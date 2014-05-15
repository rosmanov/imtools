#ifndef IMTOOLS_HXX
#define IMTOOLS_HXX

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <stdio.h>
#include <getopt.h>

#include <sys/stat.h>
#include <stdlib.h>
#include <iostream>

#include <stdexcept>
#include <stdint.h>

#include <cassert>
#include <cerrno>


#ifdef IMTOOLS_DEBUG
# define debug_log( ... ) printf( "debug: " __VA_ARGS__ )
#else
# define debug_log( ... )
#endif

#define save_int_opt_arg( __arg, ... )       \
{                                            \
  char *optarg_end;                          \
  (__arg) = strtol(optarg, &optarg_end, 10); \
  if (*optarg_end != '\0' || (__arg) < 0) {  \
    fprintf(stderr, __VA_ARGS__);            \
    usage(true);                             \
  }                                          \
}


namespace imtools {

typedef std::vector<cv::Rect> bound_box_vector_t;

enum blur_type {
  BLUR_NONE   = 0,
  BLUR        = 1,
  BLUR_GAUSS  = 2,
  BLUR_MEDIAN = 3
};

enum {
  THRESHOLD_MOD = 42,
  THRESHOLD_MIN = 20,
  THRESHOLD_MAX = 255,
  THRESHOLD_BOXES_MIN = 3,
  THRESHOLD_BOXES_MAX = 255
};

int file_exists(const char *filename);

// Computes difference between old_img and new_img. The matrix values lower than mod_threshold are
// cut down to zeros. Result (1-channel binary image) is stored in out_img.
void diff(cv::Mat& out_img, const cv::Mat& old_img, const cv::Mat& new_img,
    const int mod_threshold = THRESHOLD_MOD);

void blur(cv::Mat& target, const blur_type type);

void threshold(cv::Mat& target, const int threshold = THRESHOLD_MIN,
    const int max_threshold = THRESHOLD_MAX);

void match_template(cv::Point& match_loc, const cv::Mat& img, const cv::Mat& tpl);

void patch(cv::Mat& out_mat, const cv::Mat& img_mat, const cv::Mat& tpl_mat,
    const int x, const int y);

// Find bounding boxes in mask (can be obtained with diff() + threshold())
void bound_boxes(bound_box_vector_t& boxes, const cv::Mat& mask,
    int min_threshold = THRESHOLD_BOXES_MIN, int max_threshold = THRESHOLD_BOXES_MAX);

} // namespace imtools

#endif // IMTOOLS_HXX
