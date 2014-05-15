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

namespace imtools {

typedef std::vector<cv::Rect> bound_box_vector_t;

const cv::Size __kernel_3x3(3, 3);

enum blur_type {
  IMTOOLS_BLUR_NONE   = 0,
  IMTOOLS_BLUR        = 1,
  IMTOOLS_BLUR_GAUSS  = 2,
  IMTOOLS_BLUR_MEDIAN = 3
};

int file_exists(const char *filename);

// Computes difference between old_img and new_img. The matrix values lower than mod_threshold are
// cut down to zeros. Result is stored in out_img.
void diff(cv::Mat& out_img, const cv::Mat& old_img, const cv::Mat& new_img, const int mod_threshold = 42);

void blur(cv::Mat& target, const blur_type type);

void threshold(cv::Mat& target, const int threshold = 20, const int max_threshold = 255);

void match_template(cv::Point& match_loc, const cv::Mat& img, const cv::Mat& tpl);

void patch(cv::Mat& out_mat, const cv::Mat& img_mat, const cv::Mat& tpl_mat, const int x, const int y);

// Find bounding boxes in mask (can be obtained with diff() + threshold())
void bound_boxes(bound_box_vector_t& boxes, const cv::Mat& mask);

} // namespace imtools

#endif // IMTOOLS_HXX
