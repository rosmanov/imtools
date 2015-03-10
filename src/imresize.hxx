/* Copyright © 2014,2015 - Ruslan Osmanov <rrosmanov@gmail.com>
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
#ifndef IMTOOLS_IMRESIZE_HXX
#define IMTOOLS_IMRESIZE_HXX
#include "imtools.hxx"
#include "imresize-api.hxx"

namespace imtools { namespace imresize {
using namespace imtools;

const char* g_program_name;

/// Source image path
std::string g_source_image_filename;
/// Output image path
std::string g_output_image_filename;
/// Output image width
uint_t g_width = 0;
/// Output image height
uint_t g_height = 0;
/// Scale factor along the horizontal axis.
double g_fx = 0.;
/// Scale factor along the vertical axis.
double g_fy = 0.;
/// Interpolation method
int g_interpolation = cv::INTER_LINEAR;


/// Template for `printf`-like function.
const char* g_usage_template = IMTOOLS_FULL_NAME "\n\n" IMTOOLS_COPYRIGHT "\n\n"
"A simple tool to make thumbnails.\n"
"Usage: %1$s OPTIONS IMAGES\n\n"
"OPTIONS:\n"
" -h, --help               Display this help.\n"
" -V, --version            Print version\n"
" -v, --verbose            Turn on verbose output. Can be used multiple times\n"
"                          to increase verbosity (e.g. -vv). Default: off.\n"
" -s, --source             Path to source image.\n"
" -o, --output             Path to output image.\n"
" -W, --width              Width of the output image.\n"
" -H, --height             Height of the output image.\n"
" -X, --fx                 Scale factor along the horizontal axis.\n"
" -Y, --fy                 Scale factor along the vertical axis.\n"
" -I, --interpolation      Interpolation method. Possible values:\n"
"    nearest  - a nearest-neighbor interpolation\n"
"    linear   - a bilinear interpolation (used by default)\n"
"    area     - resampling using pixel area relation. It may be a preferred\n"
"               method for image decimation, as it gives moire’-free results.\n"
"               But when the image is zoomed, it is similar to the `nearest` method.\n"
"    cubic    - a bicubic interpolation over 4x4 pixel neighborhood\n"
"    lanczos4 - a Lanczos interpolation over 8x8 pixel neighborhood\n"
"\nEXAMPLE:\n\n"
"The following command makes a 90x100px thumbnail from src.png and writes the result into out.png\n"
"%1$s -s src.png -o out.png -W 90 -H 100\n\n"
"To decimate the image by factor of 2 in each direction\n"
"%1$s -s src.png -o out.png --fx 0.5 --fy 0.5\n";

// CLI arguments.
const char *g_short_options = "hvVs:o:W:H:X:Y:I:";
const struct option g_long_options[] = {
  {"help",          no_argument,       NULL, 'h'},
  {"verbose",       no_argument,       NULL, 'v'},
  {"version",       no_argument,       NULL, 'V'},
  {"source",        required_argument, NULL, 's'},
  {"output",        required_argument, NULL, 'o'},
  {"width",         required_argument, NULL, 'W'},
  {"height",        required_argument, NULL, 'H'},
  {"fx",            required_argument, NULL, 'X'},
  {"fy",            required_argument, NULL, 'Y'},
  {"interpolation", required_argument, NULL, 'I'},
  {0,               0,                 0,    0}
};


}} // namespace imtools::imresize

#endif // IMTOOLS_IMRESIZE_HXX
// vim: et ts=2 sts=2 sw=2
