/* Copyright (C) 2014  Ruslan Osmanov <rrosmanov@gmail.com>
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

#ifndef IMTOOLS_HXX
#define IMTOOLS_HXX

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <getopt.h>
#include <sys/stat.h>
#include <stdexcept>
#include <cassert>
#include <cerrno>
#include <ctime>
#include <cstdarg>
#include <unistd.h>

#include "threads.hxx"
#include "log.hxx"
#include "exceptions.hxx"

using cv::Mat;
using cv::Rect;
using cv::Scalar;
using cv::Point;


#define IMTOOLS_VERSION "1.1.0-dev"

#ifdef IMTOOLS_DEBUG
# define IMTOOLS_BUILD_TYPE "debug"
#else
# define IMTOOLS_BUILD_TYPE "release"
#endif

#ifdef IMTOOLS_THREADS
# define IMTOOLS_SUFFIX "threaded"
#else
# define IMTOOLS_SUFFIX "non-threaded"
#endif

#define IMTOOLS_FULL_NAME "ImTools " IMTOOLS_VERSION " (" IMTOOLS_BUILD_TYPE ") (" IMTOOLS_SUFFIX ")"
#define IMTOOLS_COPYRIGHT "Copyright (C) 2014 - Ruslan Osmanov <rrosmanov@gmail.com>"

#define save_int_opt_arg(__arg, ...)              \
{                                                 \
  (__arg) = get_int_opt_arg(optarg, __VA_ARGS__); \
}


namespace imtools {

/// Minimum area of a bounding box to be considered "big enough" in square pixels
/// Bounding boxes having smaller area will be merged together by means of morphological operations.
const int MIN_BOUND_BOX_AREA = 2800;

/// Verbose mode for CLI output
/// 0 - off, 1 - verbose, 2 - more verbose
extern int verbose;


typedef unsigned int uint_t;
typedef Rect bound_box_t;
typedef std::vector<bound_box_t> bound_box_vector_t;
typedef std::vector<std::string> images_vector_t;

enum blur_type {
  BLUR_NONE   = 0,
  BLUR        = 1,
  BLUR_GAUSS  = 2,
  BLUR_MEDIAN = 3
};

enum {
  THRESHOLD_MOD = 40,
  THRESHOLD_MIN = 20,
  THRESHOLD_MAX = 255
};

struct box_arg_t {
  bound_box_t *box;
  Mat         *old_img;
  Mat         *out_img;
  std::string *filename;
};

struct image_process_arg_t {
  std::string *filename;
  std::string *out_filename;
  Mat         *diff_img;
};

inline bool
file_exists(const char *filename)
{
  struct stat st;
  return (stat(filename, &st) == 0);
}

inline bool
file_exists(const std::string& filename)
{
  struct stat st;
  return (stat(filename.c_str(), &st) == 0);
}


int get_int_opt_arg(const char* optarg, const char* format, ...);

/// Computes difference between old_img and new_img. The matrix values lower
/// than mod_threshold are cut down to zeros. Result (1-channel binary image) is
/// stored in out_img.
void diff(Mat& out_img, const Mat& old_img, const Mat& new_img,
    const int mod_threshold = THRESHOLD_MOD);

/// Can be used to reduce noise
void blur(Mat& target, const blur_type type);

/// Reduces noise
void threshold(Mat& target, const int threshold = THRESHOLD_MIN,
    const int max_threshold = THRESHOLD_MAX);

void match_template(Point& match_loc, const Mat& img, const Mat& tpl);

/// Patch OUT_MAT at position (X, Y) with contents of TPL_MAT.
void patch(Mat& out_mat, const Mat& tpl_mat, const Rect& roi);

/// Find bounding boxes in mask (can be obtained with diff() + threshold())
void bound_boxes(bound_box_vector_t& boxes, const Mat& mask,
    int min_threshold = THRESHOLD_MIN, int max_threshold = THRESHOLD_MAX);

// Get average of the value computed by get_MSSIM()
double get_avg_MSSIM(const Mat& i1, const Mat& i2);

/// Computes structural similarity coefficient, i.e. similarity between i1 and i2 matrices.
/// Each item of the return value is a number between 0 and 1, where 1 is the perfect match.
Scalar get_MSSIM(const Mat& i1, const Mat& i2);

/// Detect whether SRC is homogenuous within boundaries of RECT.
/// Enlarge RECT until it is heterogeneous, or SRC boundaries are reached.
void make_heterogeneous(Rect& rect, const Mat& src);

} // namespace imtools

#endif // IMTOOLS_HXX
// vim: et ts=2 sts=2 sw=2
