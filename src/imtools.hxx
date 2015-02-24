/* \file
 *
 * \copyright Copyright © 2014,2015  Ruslan Osmanov <rrosmanov@gmail.com>
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
#include <sstream> // istringstream

#include "threads.hxx"
#include "log.hxx"
#include "exceptions.hxx"


#define IMTOOLS_VERSION "1.2.1"

#ifdef IMTOOLS_DEBUG
# define IMTOOLS_BUILD_TYPE "debug"
#else
# define IMTOOLS_BUILD_TYPE "release"
#endif

#ifdef IMTOOLS_THREADS
# ifdef USE_OPENMP
#  define IMTOOLS_THREADS_BACKEND "OpenMP"
# else
#  define IMTOOLS_THREADS_BACKEND "Boost"
# endif
# define IMTOOLS_SUFFIX "threaded" "(" IMTOOLS_THREADS_BACKEND ")"
#else
# define IMTOOLS_SUFFIX "non-threaded"
#endif

#define IMTOOLS_FULL_NAME "ImTools " IMTOOLS_VERSION \
  "-" IMTOOLS_BUILD_TYPE " " IMTOOLS_SUFFIX
#define IMTOOLS_COPYRIGHT "Copyright (C) 2014,2015 - Ruslan Osmanov <rrosmanov@gmail.com>"

#define save_int_opt_arg(__arg, ...)                        \
{                                                           \
  (__arg) = imtools::get_opt_arg<int>(optarg, __VA_ARGS__); \
}

#define save_double_opt_arg(__arg, ...)                        \
{                                                              \
  (__arg) = imtools::get_opt_arg<double>(optarg, __VA_ARGS__); \
}



namespace imtools {

typedef unsigned int uint_t;
typedef cv::Rect bound_box_t;
typedef std::vector<bound_box_t> bound_box_vector_t;
typedef std::vector<std::string> images_vector_t;

/// Minimum area of a bounding box to be considered "big enough" in square pixels
/// Bounding boxes having smaller area will be merged together by means of morphological operations.
const int MIN_BOUND_BOX_AREA = 2800;

/// Verbose mode for CLI output:
/// - 0 - off
/// - 1 - verbose
/// - 2 - more verbose
extern uint_t verbose;


enum blur_type {
  BLUR_NONE   = 0,
  BLUR        = 1,
  BLUR_GAUSS  = 2,
  BLUR_MEDIAN = 3
};

enum threshold_type {
  /// Default modification threshold (in percents).
  //THRESHOLD_MOD = 25,
  /// Default minimum noise suppression threshold.
  THRESHOLD_MIN = 20,
  /// Default maximum noise suppression threshold.
  THRESHOLD_MAX = 255
};


inline bool
file_exists(const char* filename)
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

template <class T> T
get_opt_arg(const string& optarg, const char* format, ...)
{
  T result;
  std::istringstream is(optarg);

  if (is >> result) {
    return result;
  }

  if (!format) {
    throw ErrorException("Invalid error format in %s", __func__ );
  }

  std::string error;
  va_list args;
  va_start(args, format);
  char message[1024];
  const int message_len = vsnprintf(message, sizeof(message), format, args);
  error = std::string(message, message_len);
  va_end(args);
  throw InvalidCliArgException(error);
}

template int get_opt_arg(const string& optarg, const char* format, ...);
template double get_opt_arg(const string& optarg, const char* format, ...);

#if 0 // unused
/// Checks if two paths are pointing to the same file.
bool equivalent_paths(const char* path1, const char* path2);
#endif

//int get_int_opt_arg(const char* const optarg, const char* format, ...);

/// Computes difference between two image matrices.
///
/// The function tries to do something similar to the command:
/// `compare old.jpg new.jpg -fuzz 25%  -compose Src -highlight-color White -lowlight-color Black diff.jpg`
/// (see ImageMagick).
/// \param result Result of the comparison; 1-channel binary image where differences have high values.
/// \param a First input matrix
/// \param b Second input matrix
void diff(cv::Mat& result , const cv::Mat& old_img, const cv::Mat& new_img);

/// Reduces noise by means of blurring the `target` image.
void blur(cv::Mat& target, const blur_type type);

/// Reduces noise by checking whether pixel values on the `target` image are
/// within `min_threshold` and `max_threshold`. Pixels having values out of
/// this range are removed.
void threshold(cv::Mat& target, const int threshold = THRESHOLD_MIN,
    const int max_threshold = THRESHOLD_MAX);

void match_template(cv::Point& match_loc, const cv::Mat& img, const cv::Mat& tpl);

/// Patch OUT_MAT at position (X, Y) with contents of TPL_MAT.
void patch(cv::Mat& out_mat, const cv::Mat& tpl_mat, const cv::Rect& roi);

/// Find bounding boxes in `mask` (can be obtained with `diff()` followed by `threshold()`)
void bound_boxes(bound_box_vector_t& boxes, const cv::Mat& mask,
    int min_threshold = THRESHOLD_MIN, int max_threshold = THRESHOLD_MAX);

/// Get average of the value computed by `get_MSSIM()`
double get_avg_MSSIM(const cv::Mat& i1, const cv::Mat& i2);

/// Computes structural similarity coefficient, i.e. similarity between i1 and i2 matrices.
/// Each item of the return value is a number between 0 and 1, where 1 is the perfect match.
cv::Scalar get_MSSIM(const cv::Mat& i1, const cv::Mat& i2);

/// Detect whether SRC is homogeneous within boundaries of RECT.
/// Enlarge RECT until it is heterogeneous, or SRC boundaries are reached.
void make_heterogeneous(cv::Rect& rect, const cv::Mat& src);

} // namespace imtools

#endif // IMTOOLS_HXX
// vim: et ts=2 sts=2 sw=2
