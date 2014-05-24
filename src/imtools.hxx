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
#include <unistd.h>

#include "threads.hxx"
#include "log.hxx"
#include "exceptions.hxx"

#define IMTOOLS_VERSION "1.0.2"

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

#define save_int_opt_arg(__arg, ...)         \
{                                            \
  char *optarg_end;                          \
  (__arg) = strtol(optarg, &optarg_end, 10); \
  if (*optarg_end != '\0' || (__arg) < 0) {  \
    error_log(__VA_ARGS__);                  \
    usage(true);                             \
  }                                          \
}


namespace imtools {

/// Verbose mode for CLI output
extern bool verbose;

typedef unsigned int uint_t;
typedef cv::Rect bound_box_t;
typedef std::vector<bound_box_t> bound_box_vector_t;
typedef std::vector<std::string> images_vector_t;

enum blur_type {
  BLUR_NONE   = 0,
  BLUR        = 1,
  BLUR_GAUSS  = 2,
  BLUR_MEDIAN = 3
};

enum {
  THRESHOLD_MOD       = 0, //42,
  THRESHOLD_MIN       = 0, //20,
  THRESHOLD_MAX       = 255,
  THRESHOLD_BOXES_MIN = 0, //3,
  THRESHOLD_BOXES_MAX = 255
};

struct box_arg_t {
  pthread_t    thread_id;
  bound_box_t *box;
  cv::Mat     *old_img;
  cv::Mat     *out_img;
};

struct image_process_arg_t {
  pthread_t    thread_id;
  std::string *filename;
  std::string *out_filename;
  cv::Mat     *diff_img;
};

inline bool
file_exists(const char *filename)
{
  struct stat st;
  return (stat(filename, &st) == 0);
}


// Computes difference between old_img and new_img. The matrix values lower than mod_threshold are
// cut down to zeros. Result (1-channel binary image) is stored in out_img.
void diff(cv::Mat& out_img, const cv::Mat& old_img, const cv::Mat& new_img,
    const int mod_threshold = THRESHOLD_MOD);

/// Can be used to reduce noise
void blur(cv::Mat& target, const blur_type type);

/// Reduces noise
void threshold(cv::Mat& target, const int threshold = THRESHOLD_MIN,
    const int max_threshold = THRESHOLD_MAX);

void match_template(cv::Point& match_loc, const cv::Mat& img, const cv::Mat& tpl);

/// Patch IMG_MAT at position (X, Y) with contents of TPL_MAT.
/// Result is stored in OUT_MAT. OUT_MAT must be of the same size and type as IMG_MAT.
void patch(cv::Mat& out_mat, const cv::Mat& img_mat, const cv::Mat& tpl_mat,
    const int x, const int y);

/// Find bounding boxes in mask (can be obtained with diff() + threshold())
void bound_boxes(bound_box_vector_t& boxes, const cv::Mat& mask,
    int min_threshold = THRESHOLD_BOXES_MIN, int max_threshold = THRESHOLD_BOXES_MAX);

} // namespace imtools

#endif // IMTOOLS_HXX
// vim: et ts=2 sts=2 sw=2
