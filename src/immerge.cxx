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

#include "imtools.hxx"

/// Max. number of target images
#define IMTOOLS_MAX_MERGE_TARGETS 100

static const char* g_program_name;

static int g_mod_threshold       = imtools::THRESHOLD_MOD;
static int g_min_threshold       = imtools::THRESHOLD_MIN;
static int g_max_threshold       = imtools::THRESHOLD_MAX;
static int g_min_boxes_threshold = imtools::THRESHOLD_BOXES_MIN;
static int g_max_boxes_threshold = imtools::THRESHOLD_BOXES_MAX;

static std::string g_old_image_filename, g_new_image_filename, g_out_dir = ".";

// Matrices for old and new images
static cv::Mat g_old_img, g_new_img;

// Destination images
typedef std::vector<std::string> images_vector_t;
static images_vector_t g_dst_images;

static const char* usage_template =
"\n\nCopyright (C) 2014 - Ruslan Osmanov <rrosmanov@gmail.com>\n\n"
"A tool to compute difference between two images and apply the difference\n"
"to a number of similar images by means of the OpenCV library.\n\n"
"Usage: %s OPTIONS IMAGES\n\n"
"Calculates difference between two images specified by --old-image and --new-image;\n"
"applies the difference to IMAGES.\n"
"The tool can be useful to update a logo or some common elements on a set of \"similar\" images.\n\n"
"\n\n"
"Note: the bigger difference in quality the higher min. thresholds are required.\n\n"
"IMAGES:\n"
"Arguments specifying the target image paths.\n\n"
"OPTIONS:\n"
" -h, --help                 Display this help.\n"
" -v, --verbose              Turn on verbose output. Default: off.\n"
" -n, --new-image            New image. Required.\n"
" -o, --old-image            Old image. Required.\n"
" -d, --out-dir              Output directory. Default: current directory.\n"
" -m, --mod-threshold        Modification threshold. Default: %d\n"
" -L, --min-threshold        Min. noise suppression threshold. Default: %d\n"
" -H, --max-threshold        Max. noise suppression threshold. Default: %d\n"
" -b, --boxes-min-threshold  Min. threshold for bound boxes. Default: %d\n"
" -B, --boxes-max-threshold  Max. threshold for bound boxes. Default: %d\n\n"
"EXAMPLE:\n"
"%s -o old.png -n new.png -d/tmp old1.png old2.png\n";

static const char *short_options = "hvn:o:d::m::L::H::b::B::";

static const struct option long_options[] = {
  {"help",                no_argument,       NULL, 'h'},
  {"verbose",             no_argument,       NULL, 'v'},
  {"new-image",           required_argument, NULL, 'n'},
  {"old-image",           required_argument, NULL, 'o'},
  {"out-dir",             optional_argument, NULL, 'd'},
  {"mod-threshold",       optional_argument, NULL, 'm'},
  {"min-threshold",       optional_argument, NULL, 'L'},
  {"max-threshold",       optional_argument, NULL, 'H'},
  {"boxes-min-threshold", optional_argument, NULL, 'b'},
  {"boxes-max-threshold", optional_argument, NULL, 'B'},
  {0,                     0,                 0,    0}
};


/// Outputs help text to stdout or stderr depending on IS_ERROR, then exit()'s with
/// the status corresponding to the error state.

static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr, usage_template,
      g_program_name,
      imtools::THRESHOLD_MOD,
      imtools::THRESHOLD_MIN,
      imtools::THRESHOLD_MAX,
      imtools::THRESHOLD_BOXES_MIN,
      imtools::THRESHOLD_BOXES_MAX,
      g_program_name);
  exit(is_error ? 1 : 0);
}


/// Loads images into memory

static inline void
load_images(const int argc, char** argv)
{
  // Load target images

  if (optind < argc) {
    for (int i = 1; optind < argc; ++i) {
      if (i > IMTOOLS_MAX_MERGE_TARGETS) {
        warning_log("max. number of targets exceeded: %d. Skipping the rest.\n", IMTOOLS_MAX_MERGE_TARGETS);
        break;
      }

      const char *filename = argv[optind++];
      if (!imtools::file_exists(filename)) {
        error_log("image %s doesn't exist.\n", filename);
        break;
      }
      g_dst_images.push_back(filename);
    }
  } else {
    error_log("Target image(s) expected. "
        "You don't need this tool just to replace one image with another ;)");
    usage(true);
  }

  if (g_old_image_filename.length() == 0 || g_new_image_filename.length() == 0) {
    error_log("expected non-empty image paths for comparison.\n");
    usage(true);
  }

  // Load the two images which will specify the modificatoin to be applied to each of g_dst_images;
  // force 3 channels

  g_old_img = cv::imread(g_old_image_filename, 1);
  g_new_img = cv::imread(g_new_image_filename, 1);

  if (g_old_img.size() != g_new_img.size()) {
    error_log("Input images have different dimensions.\n");
    usage(true);
  }
  if (g_old_img.type() != g_new_img.type()) {
    error_log("Input images have different types.\n");
    usage(true);
  }
}


/// Thread routine) applying a bounding box to the output matrix.
/// @note Is called directly, if there is no threads support.
/// XXX mutex locking

static void*
apply_bounding_box(void* arg)
{
  imtools::patch_box_arg_t *pbarg;
  cv::Mat                   old_tpl_img;
  cv::Mat                   new_tpl_img;
  cv::Point                 match_loc;

  pbarg = (imtools::patch_box_arg_t *) arg;
  assert(pbarg && pbarg->box && pbarg->old_img && pbarg->out_img);

  // Get a part of original image which had been changed
  // using the next bounding box as a mask

  old_tpl_img = cv::Mat(g_old_img, *pbarg->box);

  // Find location on OLD_IMG which is similar to OLD_TPL_IMG

  imtools::match_template(match_loc, *pbarg->old_img, old_tpl_img);

  // Now get modified part corresponding to current box

  new_tpl_img = cv::Mat(g_new_img, *pbarg->box);

  // Copy NEW_TPL_IMG over the mask of the target matrix OLD_IMG
  // at location (match_loc.x, match_loc.y).
  // The result will be stored in OUT_IMG.

  try {
    imtools::patch(*pbarg->out_img, g_new_img, new_tpl_img, match_loc.x, match_loc.y);
  } catch (imtools::template_out_of_bounds_exception& e) {
    warning_log("%s, skipping!\n", e.what());
  }
}


static inline void
merge()
{
  debug_timer_init(t1, t2);
  debug_timer_start(t1);

  cv::Point match_loc;
  cv::Mat old_img, out_img, diff_img, old_tpl_img, new_tpl_img;
  std::string merged_filename;

  // Compute difference between g_old_img and g_new_img

  imtools::diff(diff_img, g_old_img, g_new_img, g_mod_threshold);
  debug_timer_end(t1, t2, imtools::diff);

  // Suppress noise

  debug_timer_start(t1);
  imtools::threshold(diff_img, g_min_threshold, g_max_threshold);
  debug_timer_end(t1, t2, imtools::threshold);

  // Patch the target images

  for (images_vector_t::iterator it = g_dst_images.begin(); it != g_dst_images.end(); ++it) {
    verbose_log("Processing target: %s\n", it->c_str());

    // Load next target forcing 3 channels

    old_img = cv::imread(*it, 1);
    if (old_img.empty()) {
      warning_log("skipping empty image: %s\n", it->c_str());
      continue;
    }

    // Prepare matrix for current target image

    out_img = old_img.clone();

    // Generate rectangles bounding each cluster of changed pixels within DIFF_IMG.

    imtools::bound_box_vector_t boxes;
    imtools::bound_boxes(boxes, diff_img);
    debug_log("boxes.size() = %ld\n", boxes.size());

    // Apply the bounding boxes

    int n_boxes = boxes.size();
#ifdef IMTOOLS_THREADS
    pthread_t thread_ids[n_boxes];

    for (int i = 0; i < n_boxes; i++) {
      debug_log("box[%d]: %dx%d\n", i, boxes[i].x, boxes[i].y);

      imtools::patch_box_arg_t pbarg;
      pbarg.box = &boxes[i];
      pbarg.old_img = &old_img;
      pbarg.out_img = &out_img;

      pthread_create(&thread_ids[i], NULL, apply_bounding_box, &pbarg);
    }

    // Wait until threads are finished

    for (int i = 0; i < n_boxes; i++) {
      debug_log("joining thread #%ld\n", thread_ids[i]);
      pthread_join(thread_ids[i], NULL);
    }
#else // no threads
    for (int i = 0; i < n_boxes; i++) {
      debug_log("box[%d]: %dx%d\n", i, boxes[i].x, boxes[i].y);

      imtools::patch_box_arg_t pbarg;
      pbarg.box = &boxes[i];
      pbarg.old_img = &old_img;
      pbarg.out_img = &out_img;

      apply_bounding_box(&pbarg);
    }
#endif // IMTOOLS_THREADS

    // Save merged matrix to filesystem

    merged_filename = g_out_dir + "/" + *it;
    verbose_log("Writing to %s\n", merged_filename.c_str());
    cv::imwrite(merged_filename, out_img);
  }

  debug_timer_end(t1, t2, imtools::merge);
}


int main(int argc, char **argv)
{
  int next_option;

  g_program_name = argv[0];

  // Parse CLI arguments

  do {
    next_option = getopt_long(argc, argv, short_options, long_options, NULL);

    switch (next_option) {
     case 'h':
        usage(false);

      case 'n':
      case 'o':
        if (!imtools::file_exists(optarg)) {
          error_log("File %s doesn't exist", optarg);
          usage(true);
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
            error_log("invalid output directory '%s', %s.\n", optarg, strerror(errno));
            usage(true);
          }
          if (!S_ISDIR(st.st_mode)) {
            error_log("%s is not a directory.\n", optarg);
            usage(true);
          }
          g_out_dir = optarg;
          break;
        }

      case 'm':
        save_int_opt_arg(g_mod_threshold, "Invalid modification threshold\n");
        break;

      case 'L':
        save_int_opt_arg(g_min_threshold, "Invalid min threshold\n");
        break;

      case 'H':
        save_int_opt_arg(g_max_threshold, "Invalid max threshold\n");
        break;

      case 'b':
        save_int_opt_arg(g_min_boxes_threshold, "Invalid min bound boxes threshold\n");
        break;

      case 'B':
        save_int_opt_arg(g_max_boxes_threshold, "Invalid max bound boxes threshold\n");
        break;

      case 'v':
        imtools::verbose = true;
        break;

      case -1:
        // done with options
        break;

      case '?':
        // unrecognized option
        usage(true);

      default:
        error_log("getopt returned character code 0%o\n", next_option);
        usage(true);
    }
  } while (next_option != -1);

  debug_log("out-dir: %s\n", optarg);
  debug_log("mod-threshold: %d\n", g_mod_threshold);
  debug_log("min-threshold: %d\n", g_min_threshold);
  debug_log("max-threshold: %d\n", g_max_threshold);
  debug_log("min-boxes-threshold: %d\n", g_min_boxes_threshold);
  debug_log("max-boxes-threshold: %d\n", g_max_boxes_threshold);

  try {

    // Load images into memory

    debug_timer_init(t1, t2);
    debug_timer_start(t1);
    load_images(argc, argv);
    debug_timer_end(t1, t2, load_images());

    // Merge the difference into the target images

    merge();

    // Replace old image with the new one

    std::string out_filename = g_out_dir + "/" + g_old_image_filename;
    verbose_log("Replacing %s with %s\n", g_new_image_filename.c_str(), out_filename.c_str());
    cv::imwrite(out_filename, g_new_img);
  } catch (cv::Exception& e) {
    error_log("CV error: %s\n", e.what());
    return 1;
  } catch (std::exception& e) {
    error_log("%s\n", e.what());
    return 1;
  }

  return 0;
}

// vim: et ts=2 sts=2 sw=2
