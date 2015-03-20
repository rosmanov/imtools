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
#pragma once
#ifndef IMTOOLS_IMMERGE_HXX
#define IMTOOLS_IMMERGE_HXX
#include <getopt.h>
#include "immerge-api.hxx"
#include "imtools.hxx"

namespace imtools { namespace immerge {
/////////////////////////////////////////////////////////////////////

#ifdef IMTOOLS_THREADS
unsigned g_max_threads = 4;
#endif

const char* g_program_name;

int g_min_threshold = THRESHOLD_MIN;
int g_max_threshold = THRESHOLD_MAX;

std::string g_old_image_filename;
std::string g_new_image_filename;
std::string g_out_dir = ".";

/// Whether non-option ARGV-elements are interpreted as `input-file output-file` pairs
bool g_pairs = false;

/// Whether to turn warnings into fatal errors
int g_strict = 0;

/// Input images.
ImageArray g_input_images;
/// Output images.
ImageArray g_out_images;


/////////////////////////////////////////////////////////////////////
/// Template for `printf`-like function.
const char* g_usage_template = IMTOOLS_FULL_NAME "\n\n" IMTOOLS_COPYRIGHT "\n\n"
"A tool to compute difference between two images and apply the difference\n"
"to a number of similar images by means of the OpenCV library.\n"
"Usage: %1$s OPTIONS IMAGES\n\n"
"Calculates difference between two images specified by --old-image and --new-image;\n"
"applies the difference to IMAGES.\n"
"The tool can be useful to update a logo or some common elements on a set of \"similar\" images.\n"
"Note: the bigger difference in quality the higher min. thresholds are required.\n\n"
"IMAGES:\n"
"Arguments specifying the target image paths.\n\n"
"OPTIONS:\n"
" -h, --help                 Display this help.\n"
" -V, --version              Print version\n"
" -v, --verbose              Turn on verbose output. Can be used multiple times\n"
"                            to increase verbosity (e.g. -vv). Default: off.\n"
" -s, --strict               Turn some warnings into fatal errors. Can be used multiple times\n"
"                            to increase strictness. Default: off.\n"
" -n, --new-image            New image. Required.\n"
" -o, --old-image            Old image. Required.\n"
" -d, --out-dir              Output directory. Default: current directory.\n"
" -p, --pairs                Interpret IMAGES as a list of input and output file pairs.\n"
"                            If present, -d (--out-dir) has no effect.\n"
#if 0
" -m, --mod-threshold        Modification threshold in %%. Default: %d.\n"
#endif
" -L, --min-threshold        Min. noise suppression threshold. Default: %2$d.\n"
" -H, --max-threshold        Max. noise suppression threshold. Default: %3$d.\n"
#ifdef IMTOOLS_THREADS
" -T, --max-threads          Max. number of concurrent threads. Default: %4$d.\n"
#endif
"\nEXAMPLES:\n\n"
"To apply changes between old.png and new.png to copies of old1.png and old2.png (out1.png and out2.png):\n"
"%1$s -o old.png -n new.png -p old1.png out1.png old2.png out2.png\n\n"
"To apply changes between old.png and new.png to old2.png (old2.png will be overwritten):\n"
"%1$s -o old.png -n new.png old2.png\n";


/////////////////////////////////////////////////////////////////////
// CLI arguments.

const char *g_short_options = "hvVsn:o:d:pm:L:H:"
#ifdef IMTOOLS_THREADS
  "T:"
#endif
  ;
const struct option g_long_options[] = {
  {"help",          no_argument,       NULL, 'h'},
  {"verbose",       no_argument,       NULL, 'v'},
  {"version",       no_argument,       NULL, 'V'},
  {"strict",        no_argument,       NULL, 's'},
  {"new-image",     required_argument, NULL, 'n'},
  {"old-image",     required_argument, NULL, 'o'},
  {"out-dir",       required_argument, NULL, 'd'},
  {"pairs",         no_argument,       NULL, 'p'},
#if 0
  {"mod-threshold", required_argument, NULL, 'm'},
#endif
  {"min-threshold", required_argument, NULL, 'L'},
  {"max-threshold", required_argument, NULL, 'H'},
#ifdef IMTOOLS_THREADS
  {"max-threads",   required_argument, NULL, 'T'},
#endif
  {0,               0,                 0,    0}
};

/////////////////////////////////////////////////////////////////////
}} // namespace imtools::immerge
#endif // IMTOOLS_IMMERGE_HXX
// vim: et ts=2 sts=2 sw=2
