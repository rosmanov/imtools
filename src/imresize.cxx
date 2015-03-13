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

#include "imresize.hxx"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace imtools::imresize;

typedef ::imtools::imresize::Command Command;


/// Outputs help message to stdout or stderr depending on `is_error`.
static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr, g_usage_template, g_program_name);
}


int
main(int argc, char **argv)
{
  int next_option;
  int exit_code   = 0;

  g_program_name = argv[0];

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

        case 's':
          if (!file_exists(optarg)) {
            throw InvalidCliArgException("File '%s' doesn't exist", optarg);
          }
          g_source_image_filename = optarg;
          break;

        case 'o':
          g_output_image_filename = optarg;
          break;

        case 'W':
          save_uint_opt_arg(g_width, "Invalid width\n");
          break;

        case 'H':
          save_uint_opt_arg(g_height, "Invalid height\n");
          break;

        case 'I':
          {
            std::string s_interpolation = optarg;
            if      (s_interpolation == "nearest")  g_interpolation = cv::INTER_NEAREST;
            else if (s_interpolation == "linear")   g_interpolation = cv::INTER_LINEAR;
            else if (s_interpolation == "area")     g_interpolation = cv::INTER_AREA;
            else if (s_interpolation == "cubic")    g_interpolation = cv::INTER_CUBIC;
            else if (s_interpolation == "lanczos4") g_interpolation = cv::INTER_LANCZOS4;
            else {
              throw InvalidCliArgException("Invalid interpolation: '%s'", optarg);
            }
          }
          break;

        case 'X':
          save_double_opt_arg(g_fx, "Invalid scale factor for horizontal axis");
          break;

        case 'Y':
          save_double_opt_arg(g_fy, "Invalid scale factor for vertical axis");
          break;

        case 'v':
          verbose++;
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

  if (!g_interpolation) {
    g_interpolation = cv::INTER_LINEAR;
  }

  debug_log("Source image: %s\n",      g_source_image_filename.c_str());
  debug_log("Output image: %s\n",      g_output_image_filename.c_str());
  debug_log("Thumbnail size: %ux%u\n", g_width, g_height);

  try {
    ResizeCommand cmd(g_source_image_filename, g_output_image_filename,
        g_width, g_height, g_fx, g_fy, g_interpolation);
    cmd.run();
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
