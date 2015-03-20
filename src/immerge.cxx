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

#include <sys/stat.h>
#include <opencv2/highgui/highgui.hpp>

#include "imtools-meta.hxx"
#include "imtools-types.hxx"

using imtools::uint_t;
using imtools::file_exists;
using imtools::ErrorException;
using imtools::InvalidCliArgException;
using namespace imtools::immerge;

#ifdef IMTOOLS_THREADS
using imtools::threads::max_threads;
#endif // IMTOOLS_THREADS


/// Outputs help message to stdout or stderr depending on `is_error`.
static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr, g_usage_template,
      g_program_name,
      imtools::Threshold::THRESHOLD_MIN,
      imtools::Threshold::THRESHOLD_MAX
#ifdef IMTOOLS_THREADS
      ,max_threads()
#endif
      );
}


/*! Loads images specified by the CLI arguments into memory
*
* \param argc The number of elements in `argv`
* \param argv Depending on the value of the `g_pairs` global variable `argv` is interpreted as one of the following:
* - `g_pairs = true`: list of input and output files: `infile outfile infile2 outfile2 ...`
* - `g_pairs = false`: list of output files: `outfile1`
* \throws ErrorException
*/
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
        if (i > MergeCommand::MAX_MERGE_TARGETS) {
          strict_log(g_strict, "max. number of targets exceeded: %d. "
              "Skipping the rest.\n", MergeCommand::MAX_MERGE_TARGETS);
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
      if (i >= MergeCommand::MAX_MERGE_TARGETS) {
        strict_log(g_strict, "max. number of targets exceeded: %d. "
            "Skipping the rest.\n", MergeCommand::MAX_MERGE_TARGETS);
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
          imtools::verbose++;
          break;

        case 'p':
          g_pairs = true;
          break;

        case 's':
          g_strict++;
          break;

        case 'V':
          imtools::print_version();
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
  debug_log("pairs: %d\n",           (int) g_pairs);
  debug_log("strict: %d\n",          (int) g_strict);
  debug_log("min-threshold: %d\n",   g_min_threshold);
  debug_log("max-threshold: %d\n",   g_max_threshold);
#ifdef IMTOOLS_THREADS
  debug_log("max-threads: %d\n",     g_max_threads);
#endif

  try {
    debug_timer_init(t1, t2);
    debug_timer_start(t1);
    load_images(argc, argv);
    debug_timer_end(t1, t2, load_images());

    debug_log("input_images size: %ld\n", g_input_images.size());
    debug_log("out_images size: %ld\n",   g_out_images.size());
    debug_log("new image filename: %s\n", g_new_image_filename.c_str());
    debug_log("old image filename: %s\n", g_old_image_filename.c_str());

    MergeCommand cmd(g_input_images,
        g_out_images,
        g_old_image_filename,
        g_new_image_filename,
        g_out_dir,
        g_strict,
        g_min_threshold,
        g_max_threshold,
        g_max_threads);
    imtools::CommandResult result;
    cmd.run(result);
    if (!result) {
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

  return (exit_code);
}

// vim: et ts=2 sts=2 sw=2
