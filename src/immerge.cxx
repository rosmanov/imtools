/**
 * \file A tool to compute difference between two images and apply the difference
 * to a number of similar images by means of the OpenCV library.
 *
 * \copyright Copyright © 2014,2015 Ruslan Osmanov <rrosmanov@gmail.com>
 *
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

#include "immerge.hxx"

using namespace imtools::immerge;
#ifdef IMTOOLS_THREADS
using imtools::max_threads;
#endif // IMTOOLS_THREADS


/// Outputs help message to stdout or stderr depending on `is_error`.
static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr, g_usage_template,
      g_program_name,
      //THRESHOLD_MOD,
      THRESHOLD_MIN,
      THRESHOLD_MAX
#ifdef IMTOOLS_THREADS
      ,max_threads()
#endif
      );
}


/// Loads images specified by the CLI arguments into memory
///
/// \param argc The number of elements in `argv`
/// \param argv Depending on the value of the `g_pairs` global variable `argv` is interpreted as one of the following:
/// - `g_pairs = true`: list of input and output files: `infile outfile infile2 outfile2 ...`
/// - `g_pairs = false`: list of output files: `outfile1`
/// \throws ErrorException
static void
load_images(const int argc, char** argv)
{
  if (optind >= argc) {
    strict_log(g_strict, "Target image(s) expected. "
        "You don't need this tool just to replace one image with another ;)\n");
    exit(1);
  }

  if (g_pairs) {
    // `argv` is a list of input and output files:
    // `infile outfile infile2 outfile2 ...`

    for (uint_t i = 0; optind < argc;) {
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
        g_input_images.push_back(filename);
      } else {
        // Output file
        g_out_images.push_back(filename);
      }
    }

    // Check if the number of input files matches the number of output files

    if (g_input_images.size() && g_input_images.size() != g_out_images.size()) {
      strict_log(g_strict, "%s file have no pair! Skipping.\n", g_input_images.back().c_str());
      g_input_images.pop_back();
    }
  } else { // `argv` is a list of output files
    for (uint_t i = 0; optind < argc; ++i) {
      if (i >= MAX_MERGE_TARGETS) {
        strict_log(g_strict, "max. number of targets exceeded: %d. "
            "Skipping the rest.\n", MAX_MERGE_TARGETS);
        break;
      }

      const char* const filename = argv[optind++];
      if (!file_exists(filename)) {
        strict_log(g_strict, "image %s doesn't exist.\n", filename);
        break;
      }
      g_input_images.push_back(filename);
    }

    // Input images should exactly match output images in this mode.
    g_out_images = g_input_images;
  }

  if (g_old_image_filename.length() == 0 || g_new_image_filename.length() == 0) {
    strict_log(g_strict, "expected non-empty image paths for comparison.\n");
    exit(1);
  }

  // Load the two images which will specify the modificatoin to be applied to
  // each of g_input_images; force 3 channels

  g_old_img = cv::imread(g_old_image_filename, 1);
  g_new_img = cv::imread(g_new_image_filename, 1);

  if (g_old_img.size() != g_new_img.size()) {
    throw ErrorException("Input images have different dimensions");
  }
  if (g_old_img.type() != g_new_img.type()) {
    throw ErrorException("Input images have different types");
  }
}


/*! Applies a patch using a bounding box.
 *
 * \param box Specifies patch area on a canvas of the size of `g_old_img` matrix (of size equal to the size of `g_new_img` matrix).
 * \param in_img Input image which will be patched.
 * \param out_img Output image previously cloned from `in_img`.
 * \param patched_boxes Patches which have been applied on `out_img`
 */
static bool
process_patch(const bound_box_t& box, const cv::Mat& in_img, cv::Mat& out_img, bound_box_vector_t& patched_boxes)
{
  bool      success       = true;
  cv::Mat   old_tpl_img;
  cv::Mat   new_tpl_img;
  cv::Point match_loc;
  cv::Point match_loc_new;
  cv::Rect  roi;
  cv::Rect  roi_new;
  double    avg_mssim;
  double    avg_mssim_new;

  bound_box_t homo_box(box);

  debug_log("%s: %dx%d @ %d;%d\n", __func__, box.width, box.height, box.x, box.y);

  try {
    // The more the box area is heterogeneous on g_old_img, the more chances
    // to match this location on the image being patched.
    // However, imtools::make_heterogeneous() *modifies* the box! So we'll have to restore its size
    // before patching.
    imtools::make_heterogeneous(/**box*/homo_box, g_old_img);

    old_tpl_img = cv::Mat(g_old_img, homo_box);
    new_tpl_img = cv::Mat(g_new_img, box);

#if defined(IMTOOLS_DEBUG)
    {
      std::ostringstream osstr;
      osstr << "old_tpl_img_" << box.x << "@" << box.y << "_" << box.width << "x" << box.height << ".jpg";
      std::string fn = osstr.str();
      debug_log("* Writing to %s\n", fn.c_str());
      cv::imwrite(fn, old_tpl_img);
    }
    {
      std::ostringstream osstr;
      osstr << "new_tpl_img_" << homo_box.x << "@" << homo_box.y << "_" << homo_box.width << "x" << homo_box.height << ".jpg";
      std::string fn = osstr.str();
      debug_log("* Writing to %s\n", fn.c_str());
      cv::imwrite(fn, new_tpl_img);
    }
#endif

    // Find likely location of an area similar to old_tpl_img on the image being processed now.
    imtools::match_template(match_loc, in_img, old_tpl_img);
    // Some patches may already be applied. We'll try to detect if it's so.
    imtools::match_template(match_loc_new, in_img, new_tpl_img);

    // Assign regions of interest for old and new images. Then we'll see which of them
    // matches best.

    if (homo_box != box) {
      // The box has been modified by imtools::make_heterogeneous()
      assert(box.x >= homo_box.x && box.y >= homo_box.y);

      roi = cv::Rect(match_loc.x + (box.x - homo_box.x), match_loc.y + (box.y - homo_box.y), box.width, box.height);
      debug_log("homo_box != *box, roi = (%d, %d, %d, %d)\n", roi.x, roi.y, roi.width, roi.height);
    } else {
      roi = cv::Rect(match_loc.x, match_loc.y, old_tpl_img.cols, old_tpl_img.rows);
      debug_log("homo_box == *box, roi = (%d, %d, %d, %d)\n", roi.x, roi.y, roi.width, roi.height);
    }
    roi_new = cv::Rect(match_loc_new.x, match_loc_new.y, new_tpl_img.cols, new_tpl_img.rows);
    debug_log("roi_new = (%d, %d, %d, %d)\n", roi_new.x, roi_new.y, roi_new.width, roi_new.height);

    // Calculate average similarity

    avg_mssim     = imtools::get_avg_MSSIM(new_tpl_img, cv::Mat(out_img, roi));
    avg_mssim_new = imtools::get_avg_MSSIM(new_tpl_img, cv::Mat(out_img, roi_new));
    debug_log("avg_mssim: %f avg_mssim_new: %f\n", avg_mssim, avg_mssim_new);

#if 0
    // Check if a patched box intersects roi. If it is not, then we ought to
    // skip the checks for avg_mssim_new. There is a number of functions computing intersection of different contours, e.g.:
    // - pointPolygonTest() checks if a point is within a polygon.
    // - cv::intersectConvexConvex() returns intersection area of two convex polygons (opencv2/imgproc/imgproc.hpp)
    // However, we'll stick with a simple loop here.
    bool b_intersection = false;
    for (bound_box_vector_t::iterator it = patched_boxes.begin(); it != patched_boxes.end(); ++it) {
      // The `&` operator returns intersection of two rectangles.
      cv::Rect r = (*it) & roi;
      if (r.width) {
        debug_log0("Got intersection\n");
        b_intersection = true;
        break;
      }
    }
    if (!b_intersection || avg_mssim > avg_mssim_new /*|| avg_mssim < 0*/) {
#endif
    if (avg_mssim > avg_mssim_new /*|| avg_mssim < 0*/) {
      imtools::patch(out_img, new_tpl_img, roi);
      patched_boxes.push_back(cv::Rect(roi));
      debug_log("<<<<<<<< Using roi: avg_mssim: %f avg_mssim_new: %f ratio: %f\n",
          avg_mssim, avg_mssim_new, (avg_mssim_new / avg_mssim));
    } else {
      debug_log("<<<<<<<< Using roi_new: avg_mssim: %f avg_mssim_new: %f ratio: %f\n",
          avg_mssim, avg_mssim_new, (avg_mssim_new / avg_mssim));
      imtools::patch(out_img, new_tpl_img, roi_new);
      patched_boxes.push_back(cv::Rect(roi_new));
    }
#if 0 && defined(IMTOOLS_DEBUG)
    {
      std::ostringstream osstr;
      osstr << "patched_" << box.x << "@" << box.y << "_" << box.width << "x" << box.height << ".jpg";
      std::string filename = osstr.str();
      debug_log("* Writing to %s\n", filename.c_str());
      cv::imwrite(filename, out_img);
    }
#endif
  } catch (ErrorException& e) {
    success = false;
    log::push_error(e.what());
  } catch (...) {
    success = false;
    log::push_error("Caught unknown exception!\n");
  }

  return (success);
}


/*! Checks if box is suspiciously large
 *
 * Sometimes OpenCV can produce bounding boxes embracing smaller boxes, or even
 * entire image! Looks like OpenCV bug. It shouldn't return nested rectangles!
 */
static inline bool
_is_huge_bound_box(const bound_box_t& box, const cv::Mat& out_img)
{
  double box_rel_size = box.area() * 100 / out_img.size().area();
  bool result = (box_rel_size > MAX_BOUND_BOX_SIZE_REL);
  if (result) {
    warning_log("Bounding box is too large: %dx%d (%f%%)\n",
        box.width, box.height, box_rel_size);
  }
  return (result);
}


/*! Applies all patches on input file and saves the result to specified output file.
 * The filenames may point to the same file.
 * \param in_filename filename of input image.
 * \param out_filename filename of output image.
 */
static bool
process_image(const std::string& in_filename, const std::string& out_filename)
{
  bool               success         = true;
  uint_t             n_boxes;
  cv::Mat            in_img;
  cv::Mat            out_img;
  std::string        merged_filename;
  bound_box_vector_t boxes;
  bound_box_vector_t patched_boxes;

  // Load target image forcing 3 channels
  verbose_log2("Processing target: %s\n", in_filename.c_str());
  in_img = cv::imread(in_filename, 1);
  if (in_img.empty()) {
    throw ErrorException("empty image skipped: " + in_filename);
  }

  // Prepare matrix for the output image
  out_img = in_img.clone();

  // Generate rectangles bounding clusters of white (changed) pixels on `g_diff_img`
  imtools::bound_boxes(boxes, g_diff_img, g_min_threshold, g_max_threshold);
  n_boxes = boxes.size();
  patched_boxes.reserve(n_boxes);

  // Process the patches
  for (uint_t i = 0; i < n_boxes; i++) {
    debug_log("box[%d]: %dx%d @ %d;%d\n", i, boxes[i].width, boxes[i].height, boxes[i].x, boxes[i].y);

    if (_is_huge_bound_box(boxes[i], out_img)) {
      continue;
    }

    if (!process_patch(boxes[i], in_img, out_img, patched_boxes)) {
      success = false;
      break;
    }
  }

  if (!success) {
    log::warn_all();
    error_log("%s: failed to process, skipping\n", in_filename.c_str());
    return (success);
  }

  // Save merged matrix to filesystem
  if (g_strict && g_pairs && file_exists(out_filename)) {
    throw ErrorException("strict mode prohibits writing to existing file " + out_filename);
  }
  verbose_log2("Writing to %s\n", out_filename.c_str());
  if (!cv::imwrite(out_filename, out_img, g_compression_params)) {
    throw FileWriteErrorException(out_filename);
  }
  verbose_log("[Output] file:%s boxes:%d\n", out_filename.c_str(), n_boxes);

  return (success);
}


#ifdef IMTOOLS_THREADS_BOOST
static void
process_image_thread_func(const uint_t i)
{
  try {
    if (process_image(g_input_images[i], g_out_images[i]))
      return;
  } catch (ErrorException& e) {
    warning_log("%s\n", e.what());
  }

  IT_SCOPED_LOCK(lock, g_thread_success_lock);
  g_thread_success = false;
}
#endif // IMTOOLS_THREADS_BOOST


/// The main controller
static inline bool
run()
{
  bool      success     = true;
  uint_t    n_images    = g_input_images.size();
  uint_t    i;
  cv::Point match_loc;
  cv::Mat   out_img;

  // Compute difference between `g_old_img` and `g_new_img`
  debug_timer_init(t1, t2);
  debug_timer_start(t1);
  imtools::diff(g_diff_img, g_old_img, g_new_img);
  debug_timer_end(t1, t2, diff);

#ifdef IMTOOLS_THREADS
  uint_t num_threads = g_input_images.size() >= g_max_threads ? g_max_threads : g_input_images.size();

  assert(g_out_images.size() == g_input_images.size());

#ifdef USE_OPENMP
  IT_INIT_OPENMP(num_threads);

  bool r;
  // We'll use `&=` instead of `=` because of restrictions of `omp atomic`.
  // See http://www-01.ibm.com/support/knowledgecenter/SSXVZZ_8.0.0/com.ibm.xlcpp8l.doc/compiler/ref/ruompatm.htm%23RUOMPATM
  _Pragma("omp parallel for private(r)")
  for (i = 0; i < n_images; ++i) {
    try {
      r = process_image(g_input_images[i], g_out_images[i]);
      if (!r) {
        _Pragma("omp atomic")
        success &= false;
      }
    } catch (ErrorException& e) {
      warning_log("%s\n", e.what());
      _Pragma("omp atomic")
      success &= false;
    }
  }
#else // ! USE_OPENMP = use boost
  boost::threadpool::pool thread_pool(num_threads);
  boost::mutex process_images_mutex;

  for (i = 0; i < n_images; ++i) {
    IT_SCOPED_LOCK(lock, process_images_mutex);
    thread_pool.schedule(boost::bind(process_image_thread_func, i));
  }

  thread_pool.wait(); // Wait for threads to finish
  success = g_thread_success;
#endif // USE_OPENMP

#else // no threads
  for (i = 0; i < n_images; ++i) {
    try {
      if (!process_image(g_input_images[i], g_out_images[i]))
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


int
main(int argc, char **argv)
{
  int next_option;
  int exit_code   = 0;

  g_program_name = argv[0];

#ifdef IMTOOLS_THREADS
  g_max_threads = max_threads();
#endif

#ifdef IMTOOLS_DEBUG
  setvbuf(stdout, NULL, _IONBF, 0); // turn off buffering
#endif

  debug_log0("Parsing CLI args\n");
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

#if 0
        case 'm':
          save_int_opt_arg(g_mod_threshold, "Invalid modification threshold");
          if (g_mod_threshold > 100 || g_mod_threshold < 0) {
            throw InvalidCliArgException("Modification threshold is out of range");
          }
          break;
#endif

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

        case 'V':
          print_version();
          exit(0);

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
    exit(2);
  }

  debug_log("out-dir: %s\n",         g_out_dir.c_str());
#if 0
  debug_log("mod-threshold: %d%%\n", g_mod_threshold);
#endif
  debug_log("min-threshold: %d\n",   g_min_threshold);
  debug_log("max-threshold: %d\n",   g_max_threshold);
#ifdef IMTOOLS_THREADS
  debug_log("max-threads: %d\n",     g_max_threads);
#endif

  try {
    g_compression_params.reserve(4);
    g_compression_params.push_back(CV_IMWRITE_PNG_STRATEGY);
    g_compression_params.push_back(cv::IMWRITE_PNG_STRATEGY_FILTERED);
    g_compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
    g_compression_params.push_back(9/* 0 -none,  9 - full */);
#if 0
    // We don't force specific JPEG quality because the format is lossy.
    g_compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    g_compression_params.push_back(90);
#endif

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
