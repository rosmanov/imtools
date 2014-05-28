/**
 * Copyright © 2014 Ruslan Osmanov <rrosmanov@gmail.com>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/**
 * A tool to compute difference between two images and apply the difference
 * to a number of similar images by means of the OpenCV library.
 */

#include "immerge.hxx"

using namespace imtools;
using std::string;


static void
cleanup(bool is_error)
{
  verbose_log2("Cleanup...\n");

  // Release resources allocated for threading

  IT_MUTEX_DESTROY(g_process_images_mutex);
  IT_MUTEX_DESTROY(g_work_mutex);
  IT_ATTR_DESTROY(g_pta);
}


/// Outputs help text to stdout or stderr depending on IS_ERROR

static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr, usage_template,
      g_program_name,
      THRESHOLD_MOD,
      THRESHOLD_MIN,
      THRESHOLD_MAX,
      THRESHOLD_BOXES_MIN,
      THRESHOLD_BOXES_MAX,
      g_program_name);
}


/// Loads images into memory

static void
load_images(const int argc, char** argv)
{
  // Load target images

  if (optind < argc) {

    if (g_pairs) { // IMAGES is a list of input and output files
      for (int i = 0; optind < argc;) {
        const char* filename = argv[optind++];

        if (i++ % 2 == 0) {
          if (i > MAX_MERGE_TARGETS) {
            strict_log(g_strict, "max. number of targets exceeded: %d. Skipping the rest.\n", MAX_MERGE_TARGETS);
            break;
          }

          // input file
          if (!file_exists(filename)) {
            strict_log(g_strict, "image %s doesn't exist.\n", filename);
            break;
          }
          g_dst_images.push_back(filename);
        } else {
          // output file
          g_out_images.push_back(filename);
        }
      }

      // Check if the number of input files matches the number of output files

      if (g_dst_images.size() && g_dst_images.size() != g_out_images.size()) {
        strict_log(g_strict, "%s file have no pair! Skipping.\n", g_dst_images.back().c_str());
        g_dst_images.pop_back();
      }
      assert(g_dst_images.size() == g_out_images.size());
    } else { // IMAGES is a list of output files
      for (int i = 0; optind < argc; ++i) {
        if (i >= MAX_MERGE_TARGETS) {
          strict_log(g_strict, "max. number of targets exceeded: %d. Skipping the rest.\n", MAX_MERGE_TARGETS);
          break;
        }

        const char *filename = argv[optind++];
        if (!file_exists(filename)) {
          strict_log(g_strict, "image %s doesn't exist.\n", filename);
          break;
        }
        g_dst_images.push_back(filename);
      }
    }
  } else {
    strict_log(g_strict, "Target image(s) expected. "
        "You don't need this tool just to replace one image with another ;)\n");
    usage(true);
    exit(1);
  }

  if (g_old_image_filename.length() == 0 || g_new_image_filename.length() == 0) {
    strict_log(g_strict, "expected non-empty image paths for comparison.\n");
    usage(true);
    exit(1);
  }

  // Load the two images which will specify the modificatoin to be applied to each of g_dst_images;
  // force 3 channels

  g_old_img = cv::imread(g_old_image_filename, 1);
  g_new_img = cv::imread(g_new_image_filename, 1);

  if (g_old_img.size() != g_new_img.size()) {
    strict_log(g_strict, "Input images have different dimensions.\n");
    exit(1);
  }
  if (g_old_img.type() != g_new_img.type()) {
    strict_log(g_strict, "Input images have different types.\n");
    exit(1);
  }
}


/// Thread routine) applying a bounding box to the output matrix.
/// @note Is called directly, if there is no threads support.

static void*
apply_bounding_box(void* arg)
{
  bool       success = true;
  box_arg_t *pbarg;
  cv::Mat    old_tpl_img;
  cv::Mat    new_tpl_img;
  cv::Point  match_loc;

  pbarg = (box_arg_t *) arg;
  assert(pbarg && pbarg->box && pbarg->old_img && pbarg->out_img);

  try {
    // Get a part of original image which had been changed
    // using the next bounding box as a mask

    old_tpl_img = cv::Mat(g_old_img, *pbarg->box);

    // Find location on OLD_IMG which is similar to OLD_TPL_IMG

    match_template(match_loc, *pbarg->old_img, old_tpl_img);

    // Now get modified part corresponding to current box

    new_tpl_img = cv::Mat(g_new_img, *pbarg->box);

    // Region of interest

    auto roi = cv::Rect(match_loc.x, match_loc.y, new_tpl_img.cols, new_tpl_img.rows);

    if (g_strict >= 2) {
      auto mssim = get_MSSIM(new_tpl_img, (*pbarg->out_img)(roi));
      debug_log("mssim: %f %f %f\n", mssim.val[0], mssim.val[1], mssim.val[2]);
      auto avg_mssim = (mssim.val[0] + mssim.val[1] + mssim.val[2]) / 3;
      if (avg_mssim < MIN_MSSIM) {
        // Images are considered significantly different
        throw LowMssimException(avg_mssim, roi, *pbarg->filename);
      }
    }

    // Copy NEW_TPL_IMG over the mask of the target matrix OLD_IMG
    // at location (match_loc.x, match_loc.y).
    // The result will be stored in OUT_IMG.

    patch(*pbarg->out_img, g_new_img, new_tpl_img, roi);

  } catch (ErrorException& e) {
    success = false;
    log::push_error(e.what());
  } catch (...) {
    log::push_error("Caught unknown exception!\n");
    success = false;
  }

  return (void*)(success);
}


static bool
process_image(string& filename, cv::Mat& diff_img, string* out_filename)
{
  bool success = true;
  cv::Mat old_img, out_img, old_tpl_img, new_tpl_img;
  string merged_filename;

  verbose_log2("Processing target: %s\n", filename.c_str());

  // Load next target forcing 3 channels

  old_img = cv::imread(filename, 1);
  if (old_img.empty()) {
    throw ErrorException("empty image skipped: " + filename);
  }

  // Prepare matrix for current target image

  out_img = old_img.clone();

  // Generate rectangles bounding each cluster of changed pixels within DIFF_IMG.

  bound_box_vector_t boxes;
  bound_boxes(boxes, diff_img);

  int n_boxes = boxes.size();
  debug_log("boxes.size() = %d\n", n_boxes);

  // Apply the bounding boxes

#if defined(IMTOOLS_THREADS)
  if (n_boxes) {
    void* thread_result;
    auto pbarg = new box_arg_t[n_boxes];

    for (int i = 0; i < n_boxes; i++) {
      IT_LOCK(g_work_mutex);
      debug_log("box[%d]: %dx%d\n", i, boxes[i].x, boxes[i].y);

      pbarg[i].box = &boxes[i];
      pbarg[i].old_img = &old_img;
      pbarg[i].out_img = &out_img;
      pbarg[i].filename = &filename;

      pthread_create(&pbarg[i].thread_id, &g_pta, apply_bounding_box, &pbarg[i]);
      IT_UNLOCK(g_work_mutex);
    }

    // Wait until threads are finished

    for (int i = 0; i < n_boxes; i++) {
      debug_log("joining thread #%ld\n", pbarg[i].thread_id);
      pthread_join(pbarg[i].thread_id, &thread_result);
      if (!thread_result) {
        success = false;
        // No break. We have to wait for all the threads
      }
    }

    delete [] pbarg;
  }
#else // no threads
  for (int i = 0; i < n_boxes; i++) {
    debug_log("box[%d]: %dx%d\n", i, boxes[i].x, boxes[i].y);

    box_arg_t pbarg;
    pbarg.box = &boxes[i];
    pbarg.old_img = &old_img;
    pbarg.out_img = &out_img;
    pbarg.filename = &filename;

    if (!apply_bounding_box(&pbarg)) {
      success = false;
      break;
    }
  }
#endif // IMTOOLS_THREADS

  if (!success) {
    log::warn_all();
    error_log("%s: failed to process, skipping\n", filename.c_str());
    return success;
  }

  // Save merged matrix to filesystem

  merged_filename = out_filename
    ? out_filename->c_str()
    : (g_out_dir + "/" + basename(filename.c_str()));

  if (g_strict) {
    if (file_exists(merged_filename))
      throw ErrorException("strict mode prohibits writing to existing file " + merged_filename);
  }

  verbose_log2("Writing to %s\n", merged_filename.c_str());
  if (!cv::imwrite(merged_filename, out_img, g_compression_params)) {
    throw FileWriteErrorException(merged_filename);
  }

  verbose_log("[Output] file:%s boxes:%d\n", merged_filename.c_str(), n_boxes);

  return success;
}


#ifdef IMTOOLS_THREADS
static void*
process_image_thread_func(void* arg)
{
  image_process_arg_t* ipa = (image_process_arg_t*)arg;

  try {
    if (!process_image(*ipa->filename, *ipa->diff_img, ipa->out_filename))
      return NULL;
  } catch (ErrorException& e) {
    warning_log("%s\n", e.what());
    return NULL;
  }

  return (void*)1;
}
#endif


static bool
merge()
{
  bool success = true;

  debug_timer_init(t1, t2);
  debug_timer_start(t1);

  cv::Point match_loc;
  cv::Mat out_img, diff_img, old_tpl_img, new_tpl_img;

  // Compute difference between g_old_img and g_new_img

  diff(diff_img, g_old_img, g_new_img, g_mod_threshold);
  debug_timer_end(t1, t2, diff);

  // Suppress noise

  debug_timer_start(t1);
  threshold(diff_img, g_min_threshold, g_max_threshold);
  debug_timer_end(t1, t2, threshold);


  // Patch the target images

#if defined(IMTOOLS_THREADS)
  int n_images = g_dst_images.size();
  auto img_proc_args = new image_process_arg_t[n_images];
  int i = 0;
  void *thread_result;

  for (images_vector_t::iterator it = g_dst_images.begin(); it != g_dst_images.end(); ++it, ++i) {
    IT_LOCK(g_process_images_mutex);

    img_proc_args[i].diff_img = &diff_img;
    img_proc_args[i].filename = &*it;
    img_proc_args[i].out_filename = g_pairs ? &g_out_images[i] : NULL;

    pthread_create(&img_proc_args[i].thread_id, &g_pta, process_image_thread_func, (void*)&img_proc_args[i]);

    IT_UNLOCK(g_process_images_mutex);
  }

  assert(n_images == i);

  // Wait for threads to finish

  for (int i = 0; i < n_images; i++) {
    debug_log("(images) joining thread #%ld\n", img_proc_args[i].thread_id);
    pthread_join(img_proc_args[i].thread_id, &thread_result);
    if (!thread_result) {
      success = false;
    }
  }

  delete [] img_proc_args;
#else // no threads
  int i = 0;
  for (images_vector_t::iterator it = g_dst_images.begin(); it != g_dst_images.end(); ++it, ++i) {
    try {
      if (!process_image(*it, diff_img, (g_pairs ? &g_out_images[i] : NULL)))
        success = false;
    } catch (ErrorException& e) {
      warning_log("%s\n", e.what());
      success = false;
    }
  }
#endif // IMTOOLS_THREADS

  debug_timer_end(t1, t2, merge);

  return (success);
}


int main(int argc, char **argv)
{
  int next_option, exit_code = 0;

  g_program_name = argv[0];

#ifdef IMTOOLS_DEBUG
  setvbuf(stdout, NULL, _IONBF, 0); // turn off buffering
#endif

  // Parse CLI arguments

  try {
    do {
      next_option = getopt_long(argc, argv, g_short_options, g_long_options, NULL);

      switch (next_option) {
        case 'h':
          usage(false);
          exit(0);

        case 'n':
        case 'o':
          if (!file_exists(optarg)) {
            throw InvalidCliArgException("File %s doesn't exist", optarg);
          }
          if (next_option == 'n') {
            g_new_image_filename = optarg;
          } else {
            g_old_image_filename = optarg;
          }
          break;

        case 'd':
          {
            struct stat st;
            if (stat(optarg, &st)) {
              throw InvalidCliArgException("invalid output directory '%s', %s.", optarg, strerror(errno));
            }
            if (!S_ISDIR(st.st_mode)) {
              throw InvalidCliArgException("%s is not a directory.", optarg);
            }
            g_out_dir = optarg;
            break;
          }

        case 'm':
          save_int_opt_arg(g_mod_threshold, "Invalid modification threshold");
          break;

        case 'L':
          save_int_opt_arg(g_min_threshold, "Invalid min threshold");
          break;

        case 'H':
          save_int_opt_arg(g_max_threshold, "Invalid max threshold\n");
          break;

        case 'b':
          save_int_opt_arg(g_min_boxes_threshold, "Invalid min bound boxes threshold");
          break;

        case 'B':
          save_int_opt_arg(g_max_boxes_threshold, "Invalid max bound boxes threshold");
          break;

        case 'v':
          verbose++;
          break;

        case 'p':
          g_pairs = true;
          break;

        case 's':
          g_strict++;
          break;

        case -1:
          // done with options
          break;

        case '?':
          // unrecognized option
          usage(true);
          exit(1);

        default:
          error_log("getopt returned character code 0%o\n", next_option);
          usage(true);
          exit(1);
      }
    } while (next_option != -1);
  } catch (imtools::InvalidCliArgException& e) {
    error_log("%s\n", e.what());
    usage(true);
    exit(1);
  }


  debug_log("out-dir: %s\n", g_out_dir.c_str());
  debug_log("mod-threshold: %d\n", g_mod_threshold);
  debug_log("min-threshold: %d\n", g_min_threshold);
  debug_log("max-threshold: %d\n", g_max_threshold);
  debug_log("min-boxes-threshold: %d\n", g_min_boxes_threshold);
  debug_log("max-boxes-threshold: %d\n", g_max_boxes_threshold);

  // Setup compression parameters

  g_compression_params.push_back(CV_IMWRITE_PNG_STRATEGY);
  g_compression_params.push_back(cv::IMWRITE_PNG_STRATEGY_FILTERED);
  g_compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
  g_compression_params.push_back(9/* 0 -none,  9 - full */);
#if 0
  g_compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
  g_compression_params.push_back(90);
#endif

  // Initialize threading

  IT_ATTR_INIT(g_pta);
  IT_MUTEX_CREATE(g_work_mutex);
  IT_MUTEX_CREATE(g_process_images_mutex);

  try {
    // Load images into memory

    debug_timer_init(t1, t2);
    debug_timer_start(t1);
    load_images(argc, argv);
    debug_timer_end(t1, t2, load_images());

    // Merge the difference into the target images

    if (!merge()) {
      exit_code = 1;
    }

  } catch (ErrorException& e) {
    error_log("%s\n", e.what());
    exit_code = 1;
  } catch (cv::Exception& e) {
    error_log("CV error: %s\n", e.what());
    exit_code = 1;
  } catch (...) {
    error_log("Unknown error!!! Please file a bug.\n");
    exit_code = 1;
  }

  cleanup(exit_code);

  return exit_code;
}

// vim: et ts=2 sts=2 sw=2
