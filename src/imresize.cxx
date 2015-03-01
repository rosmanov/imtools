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

using namespace imtools::imresize;


/// Outputs help message to stdout or stderr depending on `is_error`.
static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr, g_usage_template, g_program_name);
}


bool run()
{
  cv::Mat source;
  cv::Mat output;

  source = cv::imread(g_source_image_filename, 1);
  if (source.empty()) {
    throw ErrorException("Source image is empty");
  }

  if (g_width > 0 && g_height > 0) {
    cv::resize(source, output, cv::Size(g_width, g_height), g_fx, g_fy, g_interpolation);
  } else if (g_fx > 0 && g_fy > 0) {
    cv::resize(source, output, cv::Size(), g_fx, g_fy, g_interpolation);
  } else {
    throw ErrorException("Expected positive number pairs of whether width/height, or fx/fy."
        "None provided. Nothing to do.");
  }

  if (!cv::imwrite(g_output_image_filename, output, g_compression_params)) {
    throw FileWriteErrorException(g_output_image_filename);
  }

  return true;
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

  debug_log("Source image: %s\n",      g_source_image_filename.c_str());
  debug_log("Output image: %s\n",      g_output_image_filename.c_str());
  debug_log("Thumbnail size: %ux%u\n", g_width, g_height);

  try {
    g_compression_params.reserve(4);
    g_compression_params.push_back(CV_IMWRITE_PNG_STRATEGY);
    g_compression_params.push_back(cv::IMWRITE_PNG_STRATEGY_FILTERED);
    g_compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
    g_compression_params.push_back(9/* 0 -none,  9 - full */);

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
