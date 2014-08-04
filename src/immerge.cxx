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
#include <thread> // std::thread::hardware_concurrency()

using namespace imtools;
using std::string;


static inline unsigned
max_threads()
{
  return std::thread::hardware_concurrency();
}


static void
init()
{
  debug_log0("init()\n");

  // Setup compression parameters

  g_compression_params.push_back(CV_IMWRITE_PNG_STRATEGY);
  g_compression_params.push_back(cv::IMWRITE_PNG_STRATEGY_FILTERED);
  g_compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
  g_compression_params.push_back(9/* 0 -none,  9 - full */);
#if 0
  // We don't force specific JPEG quality because the format is lossy.
  g_compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
  g_compression_params.push_back(90);
#endif
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
      max_threads(),
      g_program_name);
}


/// Loads images specified by the CLI arguments into memory
static void
load_images(const int argc, char** argv)
{
  if (optind < argc) {
    if (g_pairs) {
      // ARGV is a list of input and output files: infile outfile infile2 outfile2 ...

      for (int i = 0; optind < argc;) {
        const char* filename = argv[optind++];

        if (i++ % 2 == 0) {
          if (i > MAX_MERGE_TARGETS) {
            strict_log(g_strict, "max. number of targets exceeded: %d. "
                "Skipping the rest.\n", MAX_MERGE_TARGETS);
            break;
          }

          // Input file
          if (!file_exists(filename)) {
            strict_log(g_strict, "image %s doesn't exist.\n", filename);
            break;
          }
          g_dst_images.push_back(filename);
        } else {
          // Output file
          g_out_images.push_back(filename);
        }
      }

      // Check if the number of input files matches the number of output files

      if (g_dst_images.size() && g_dst_images.size() != g_out_images.size()) {
        strict_log(g_strict, "%s file have no pair! Skipping.\n", g_dst_images.back().c_str());
        g_dst_images.pop_back();
      }
    } else { // ARGV is a list of output files
      for (int i = 0; optind < argc; ++i) {
        if (i >= MAX_MERGE_TARGETS) {
          strict_log(g_strict, "max. number of targets exceeded: %d. "
              "Skipping the rest.\n", MAX_MERGE_TARGETS);
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
    //usage(true);
    exit(1);
  }

  if (g_old_image_filename.length() == 0 || g_new_image_filename.length() == 0) {
    strict_log(g_strict, "expected non-empty image paths for comparison.\n");
    //usage(true);
    exit(1);
  }

  // Load the two images which will specify the modificatoin to be applied to
  // each of g_dst_images; force 3 channels

  g_old_img = cv::imread(g_old_image_filename, 1);
  g_new_img = cv::imread(g_new_image_filename, 1);

  if (g_old_img.size() != g_new_img.size()) {
    throw ErrorException("Input images have different dimensions");
  }
  if (g_old_img.type() != g_new_img.type()) {
    throw ErrorException("Input images have different types");
  }
}


/// Thread routine applying a patch indicated by bounding box to the output matrix.
/// @note Is called directly, if there is no threads support.
static bool
apply_bounding_box(box_arg_t* pbarg)
{
  bool         success       = true;
  Mat          old_tpl_img;
  Mat          new_tpl_img;
  Point        match_loc, match_loc_new;
  Rect         roi, roi_new;
  bound_box_t *box;
  double       avg_mssim;
  double       avg_mssim_new;

  assert(pbarg && pbarg->box && pbarg->old_img && pbarg->out_img);
  box = pbarg->box;
  Rect homo_box(*box);

  debug_log("apply_bounding_box: %dx%d @ %d;%d\n",
      box->width, box->height, box->x, box->y);

  try {
    // The more the box area is heterogeneouson g_old_img, the more chances
    // to match this location on the image being patched.
    // However, make_heterogeneous() *modifies* the box! So we'll have to restore its size
    // before patching.
    make_heterogeneous(/**box*/homo_box, g_old_img);

    old_tpl_img = Mat(g_old_img, homo_box);
    new_tpl_img = Mat(g_new_img, *box);

    // Find likely location of an area similar to old_tpl_img on the image being processed now
    match_template(match_loc, *pbarg->old_img, old_tpl_img);
    // Some patches may already be applied. We'll try to detect if it's so.
    match_template(match_loc_new, *pbarg->old_img, new_tpl_img);

    if (homo_box != *box) {
      assert(box->x >= homo_box.x && box->y >= homo_box.y);

      roi = cv::Rect(match_loc.x + (box->x - homo_box.x),
          match_loc.y + (box->y - homo_box.y),
          box->width, box->height);

    } else {
      roi = cv::Rect(match_loc.x, match_loc.y, old_tpl_img.cols, old_tpl_img.rows);
    }
    roi_new = cv::Rect(match_loc_new.x, match_loc_new.y, new_tpl_img.cols, new_tpl_img.rows);

    // Calculate average similarity

    avg_mssim     = get_avg_MSSIM(new_tpl_img, Mat(*pbarg->out_img, roi));
    avg_mssim_new = get_avg_MSSIM(new_tpl_img, Mat(*pbarg->out_img, roi_new));

    if (avg_mssim_new < avg_mssim) {
      patch(*pbarg->out_img, new_tpl_img, roi);
    } else {
      patch(*pbarg->out_img, new_tpl_img, roi_new);
    }
  } catch (ErrorException& e) {
    success = false;
    log::push_error(e.what());
  } catch (...) {
    success = false;
    log::push_error("Caught unknown exception!\n");
  }

  return success;
}


/// Checks if box is suspiciously large (sometimes opencv can produce bounding boxes
/// embracing smaller boxes, or even entire image!). Looks like OpenCV bug. It
/// shouldn't return nested rectangles!
static inline bool
_is_huge_bound_box(const bound_box_t& box, const Mat& out_img)
{
  double box_rel_size = box.area() * 100 / out_img.size().area();
  bool result = (box_rel_size > MAX_BOUND_BOX_SIZE_REL);
  if (result) {
    warning_log("Bounding box is too large: %dx%d (%f%%)\n",
        box.width, box.height, box_rel_size);
  }
  return result;
}


/// Applies all patches to the target image
///
/// FILENAME - target filename.
/// DIFF_IMG - binary image with information about differences between
/// original and modified images, where modified spots have high values.
/// OUT_FILENAME - optional output filename. If is NULL, result is
/// written to the output directory.
static bool
process_image(string& filename, cv::Mat& diff_img, string* out_filename)
{
  bool success = true;
  int n_boxes;
  Mat old_img, out_img, old_tpl_img, new_tpl_img;
  string merged_filename;
  bound_box_vector_t boxes;

  verbose_log2("Processing target: %s\n", filename.c_str());

  // Load next target forcing 3 channels

  old_img = cv::imread(filename, 1);
  if (old_img.empty()) {
    throw ErrorException("empty image skipped: " + filename);
  }

  // Prepare matrix for current target image

  out_img = old_img.clone();

  // Generate rectangles bounding clusters of white (changed) pixels on DIFF_IMG

  bound_boxes(boxes, diff_img, g_min_threshold, g_max_threshold);
  n_boxes = boxes.size();

  // Apply the bounding boxes

  for (int i = 0; i < n_boxes; i++) {
    debug_log("box[%d]: %dx%d\n", i, boxes[i].x, boxes[i].y);

    if (_is_huge_bound_box(boxes[i], out_img)) {
      continue;
    }

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

  if (!success) {
    log::warn_all();
    error_log("%s: failed to process, skipping\n", filename.c_str());
    return success;
  }

  // Save merged matrix on filesystem

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
static void
process_image_thread_func(image_process_arg_t* ipa)
{
  try {
    if (process_image(*ipa->filename, *ipa->diff_img, ipa->out_filename))
      return;
  } catch (ErrorException& e) {
    warning_log("%s\n", e.what());
  }

  boost::mutex::scoped_lock lock(g_thread_success_mutex);
  g_thread_success = false;
}
#endif


static bool
run()
{
  bool success = true;

  Point match_loc;
  Mat out_img, diff_img, old_tpl_img, new_tpl_img;
  int i = 0;


  // Compute difference between g_old_img and g_new_img

  debug_timer_init(t1, t2);
  debug_timer_start(t1);
  diff(diff_img, g_old_img, g_new_img, g_mod_threshold);
  debug_timer_end(t1, t2, diff);


#ifdef IMTOOLS_THREADS
  int n_images = g_dst_images.size();
  auto img_proc_args = new image_process_arg_t[n_images];

  assert(g_out_images.size() == g_dst_images.size());

  pool thread_pool(g_max_threads);
  boost::mutex process_images_mutex;
  for (images_vector_t::iterator it = g_dst_images.begin();
      it != g_dst_images.end();
      ++it, ++i)
  {
    boost::mutex::scoped_lock lock(process_images_mutex);

    img_proc_args[i].diff_img = &diff_img;
    img_proc_args[i].filename = &*it;
    img_proc_args[i].out_filename = g_pairs ? &g_out_images[i] : NULL;

    thread_pool.schedule(boost::bind(process_image_thread_func, &img_proc_args[i]));
  }

  assert(n_images == i);

  // Wait for threads to finish

  thread_pool.wait();
  success = g_thread_success;

  delete [] img_proc_args;
#else // no threads
  for (images_vector_t::iterator it = g_dst_images.begin();
      it != g_dst_images.end();
      ++it, ++i)
  {
    try {
      if (!process_image(*it, diff_img, (g_pairs ? &g_out_images[i] : NULL)))
        success = false;
    } catch (ErrorException& e) {
      warning_log("%s\n", e.what());
      success = false;
    }
  }
#endif // IMTOOLS_THREADS

  debug_timer_end(t1, t2, run);

  return (success);
}


int main(int argc, char **argv)
{
  int next_option, exit_code = 0;

  g_program_name = argv[0];

#ifdef IMTOOLS_THREADS
  g_max_threads = max_threads();
#endif

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

#ifdef IMTOOLS_THREADS
        case 'T':
          {
            unsigned max_thread_num = max_threads();

            save_int_opt_arg(g_max_threads, "Invalid max threads\n");

            if (g_max_threads > max_thread_num) {
              throw InvalidCliArgException("Cannot set max threads limit to %d. "
                  "Maximum allowed value is %u", g_max_threads, max_thread_num);
            }
          }
          break;
#endif

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
          exit(2);

        default:
          error_log("getopt returned character code 0%o\n", next_option);
          usage(true);
          exit(2);
      }
    } while (next_option != -1);
  } catch (imtools::InvalidCliArgException& e) {
    error_log("%s\n", e.what());
    //usage(true);
    exit(2);
  }

  debug_log("out-dir: %s\n",       g_out_dir.c_str());
  debug_log("mod-threshold: %d\n", g_mod_threshold);
  debug_log("min-threshold: %d\n", g_min_threshold);
  debug_log("max-threshold: %d\n", g_max_threshold);
  debug_log("max-threads: %d\n",   g_max_threads);

  try {
    init();

    debug_timer_init(t1, t2);
    debug_timer_start(t1);
    load_images(argc, argv);
    debug_timer_end(t1, t2, load_images());

    if (!run()) {
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

  return exit_code;
}

// vim: et ts=2 sts=2 sw=2
