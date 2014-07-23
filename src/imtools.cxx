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

#include "imtools.hxx"

using std::string;
using std::vector;
using cv::Mat;
using cv::Rect;
using cv::Point;
using cv::Size;

namespace imtools {

int verbose = 0;


int
get_int_opt_arg(const char* optarg, const char* format, ...)
{
  if (optarg) {
    char *optarg_end;
    int ret = strtol(optarg, &optarg_end, 10);
    if (*optarg_end != '\0' || ret < 0) {
      goto err;
    }

    return ret;
  }

err:
  if (format) {
    string error;

    va_list args;
    va_start(args, format);
    char message[1024];
    int message_len = vsnprintf(message, sizeof(message), format, args);
    error = string(message, message_len);
    va_end(args);

    throw InvalidCliArgException(error);
  }

  return -1;
}


void
diff(Mat& out_img, const Mat& old_img, const Mat& new_img, const int mod_threshold)
{
  debug_timer_init(t1, t2);
  debug_timer_start(t1);

  // Select likely modified pixels.
  // We try to something similar to command:
  // compare old.jpg new.jpg -fuzz 25%  -compose Src -highlight-color White -lowlight-color Black diff.jpg
  out_img = (old_img - new_img > mod_threshold);

  // Create matrix of the same size and type as diff_img
  //out_img.create(diff_img.size(), diff_img.type());

  // Convert to grayscale
  cv::cvtColor(out_img, out_img, CV_BGR2GRAY);

  debug_timer_end(t1, t2, imtools::diff);
}


void
blur(Mat& target, const blur_type type)
{
  debug_timer_init(t1, t2);
  debug_timer_start(t1);

  switch (type) {
    case BLUR_NONE:
      break;

    case BLUR:
      cv::blur(target, target, Size(3, 3));
      break;

    case BLUR_GAUSS:
      cv::GaussianBlur(target, target, Size(3, 3), 10);
      break;

    case BLUR_MEDIAN:
      cv::medianBlur(target, target, 9);
      break;

    default:
      throw ErrorException("Invalid blur type");
  }

  debug_timer_end(t1, t2, imtools::blur);
}


void
threshold(Mat& target, const int threshold, const int max_threshold)
{
  debug_timer_init(t1, t2);
  debug_timer_start(t1);

  assert(max_threshold >= threshold);

  cv::threshold(target, target, threshold, max_threshold, cv::THRESH_BINARY);

  debug_timer_end(t1, t2, imtools::threshold);
}


void
match_template(Point& match_loc, const Mat& img, const Mat& tpl)
{
  debug_timer_init(t1, t2);
  debug_timer_start(t1);

  Mat result;
  int match_method = CV_TM_SQDIFF;

  int result_cols = img.cols - tpl.cols + 1;
  int result_rows = img.rows - tpl.rows + 1;
  result.create(result_cols, result_rows, CV_32FC1);

  cv::matchTemplate(img, tpl, result, match_method);
  cv::normalize(result, result, 0, 1, cv::NORM_MINMAX, -1, Mat());

  // Localize the best match with minMaxLoc

  double min_val, max_val;
  Point min_loc, max_loc;
  cv::minMaxLoc(result, &min_val, &max_val, &min_loc, &max_loc, Mat());

  // For SQDIFF and SQDIFF_NORMED, the best matches are lower values. For all
  // the other methods, the higher the better
  if( match_method  == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED ) {
    match_loc = min_loc;
  } else {
    match_loc = max_loc;
  }

  debug_timer_end(t1, t2, imtools::match_template);
}


void
patch(Mat& out_mat, const Mat& tpl_mat, const Rect& roi)
{
  debug_log("imtools::patch(), x: %d, y: %d\n", roi.x, roi.y);
  debug_timer_init(t1, t2);
  debug_timer_start(t1);

  if (out_mat.empty()) {
    throw ErrorException("Output matrix is empty");
  }
  if (tpl_mat.empty()) {
    throw ErrorException("Input template matrix is empty");
  }

  if ((roi.x + tpl_mat.cols) > out_mat.cols
      || (roi.y + tpl_mat.rows) > out_mat.rows)
  {
    throw TemplateOutOfBoundsException(tpl_mat, out_mat, roi);
  }

  // Region of interest
  //Rect roi = Rect(pt.x, pt.y, tpl_mat.cols, tpl_mat.rows);

  // Extract ROI from the output image as a reference
  Mat out_mat_roi = out_mat(roi);

  // Overwrite the ROI
  tpl_mat.copyTo(out_mat_roi);

  debug_timer_end(t1, t2, imtools::patch);
}


/// Merge small rectangles into larger rectangles.
///
/// RESULT - output vector. Must be empty on input.
/// BOXES - input vector of rectangles
/// BIN_MASK - source binary image
///
/// See http://stackoverflow.com/questions/24586923/how-do-i-apply-dilation-selectively-using-opencv?noredirect=1#comment38089974_24586923
static void
_merge_small_boxes(bound_box_vector_t& result, bound_box_vector_t& boxes, const Mat& bin_mask)
{
  Mat tmp_mask = bin_mask.clone();

  // Guess that 1/4 of the boxes will be large enough
  result.reserve(boxes.size() >> 2);

  // Store big enough boxes into RESULT

  for (bound_box_vector_t::iterator it = boxes.begin(); it != boxes.end(); ++it) {
    if ((*it).area() >= MIN_BOUND_BOX_AREA) {
      result.push_back(*it);

      // Erase the box area on tmp_mask
      Mat m(tmp_mask, *it);
      m = Scalar(0);
    }
  }

  // Apply morphological closing operation, i.e. erode(dilate(src, kern), kern).
  // With this operation the small boxes should be merged.

  int morph_size = 4;
  Mat kern = cv::getStructuringElement(cv::MORPH_RECT,
      Size(2 * morph_size + 1, 2 * morph_size + 1),
      Point(morph_size, morph_size));
  cv::morphologyEx(tmp_mask, tmp_mask, cv::MORPH_CLOSE, kern, Point(-1, -1), 2);

  // Find new contours the the merged areas

  vector<vector<Point> > contours;
  vector<cv::Vec4i> hierarchy;
  cv::findContours(tmp_mask, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

  // Collect bounding boxes covering larger areas.
  // Approximate contours to polygons, get bounding rects

  vector<vector<Point> > contours_poly(contours.size());
  result.reserve(result.size() + contours.size());
  for (size_t i = 0; i < contours.size(); ++i) {
    cv::approxPolyDP(Mat(contours[i]), contours_poly[i], 1, true);
    auto m = Mat(contours_poly[i]);
    auto rect = cv::boundingRect(m);
    result.push_back(rect);
  }

  result.shrink_to_fit();
}


/// Will enlarge RECT by step pixels on each side.
/// Returns true, if at least one side had been changed, otherwise false.

static inline bool
_enlarge(Rect& rect, const Mat& boundary, const int step)
{
  int boundary_touch = 0;

  if (rect.y > step) {
    rect.y -= step;
  } else {
    rect.y = 0;
    boundary_touch++;
  }

  if (rect.x > step) {
    rect.x -= step;
  } else {
    rect.x = 0;
    boundary_touch++;
  }

  // Note, in OpenCV Rect width and height boundaries are exclusive, unlike x and y properties!
  // See http://docs.opencv.org/modules/core/doc/basic_structures.html?highlight=mat_#rect

  if (boundary.rows > rect.y + rect.height + step) {
    rect.height += step;
  } else {
    rect.height = boundary.rows;
    boundary_touch++;
  }

  if (boundary.cols > rect.x + rect.width + step) {
    rect.width += step;
  } else {
    rect.width = boundary.cols;
    boundary_touch++;
  }

  return static_cast<bool>(boundary_touch != 4);
}


void
make_heterogeneous(Rect& rect, const Mat& src)
{
  if (!src.data) {
    error_log("make_heterogeneous: source matrix is empty\n");
    return;
  }

  cv::Scalar mean, stddev;
  double ratio;
  const double min_ratio = 0.08;

  for (int step = 4; step < 1024 ; step += 4) {
    cv::meanStdDev(Mat(src, rect), mean, stddev);
    ratio = stddev[0] / mean[0];

    debug_log("make_heterogeneous: ratio = %lf mean = %lf stddev = %lf box: %dx%d @ %d;%d\n",
        ratio, mean[0], stddev[0], rect.width, rect.height, rect.x, rect.y);

    if (ratio >= min_ratio) {
      break;
    }

    if (!_enlarge(rect, src, step)) {
      debug_log("make_heterogeneous: skipping to enlarge box: %dx%d @ %d;%d\n",
          rect.width, rect.height, rect.x, rect.y);
      // SRC boundaries reached. Can't enlarge RECT anymore.
      break;
    }
  }
}


void
bound_boxes(bound_box_vector_t& result, const Mat& in_mask, int min_threshold, int max_threshold)
{
  debug_timer_init(t1, t2);
  debug_timer_start(t1);

  bound_box_vector_t boxes;
  Mat mask;
  vector<vector<Point> > contours;
  vector<cv::Vec4i> hierarchy;

  assert(min_threshold >= 0 && min_threshold <= max_threshold);

  // Convert image to grayscale

  if (mask.channels() < 3) {
    mask = in_mask.clone();
  } else {
    debug_log("bound_boxes: cvtColor() to grayscale, channels = %d\n", in_mask.channels());
    cv::cvtColor(in_mask, mask, CV_BGR2GRAY);
  }

  // Suppress noise

  debug_log("bound_boxes: threshold(%d, %d)\n", min_threshold, max_threshold);
  cv::threshold(mask, mask, min_threshold, max_threshold, cv::THRESH_BINARY);
#if 0
  cv::Canny(mask, mask, min_threshold, min_threshold * 3, 3);
#endif

  // Apply morphological closing operation, i.e. dilate, then erode (more noise suppression).

  int morph_size = 1;
  Mat kern = cv::getStructuringElement(cv::MORPH_RECT, Size(2 * morph_size + 1, 2 * morph_size + 1), Point(morph_size, morph_size));
  cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kern, Point(-1, -1), 1);

  // Detect contours of modified areas

  cv::findContours(mask, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

  // Approximate contours to polygons, get bounding boxes

  vector<vector<Point> > contours_poly(contours.size());
  boxes.reserve(contours.size());
  for (size_t i = 0; i < contours.size(); ++i) {
    cv::approxPolyDP(Mat(contours[i]), contours_poly[i], 1, true);

    auto m = Mat(contours_poly[i]);
    auto rect = cv::boundingRect(m);

    boxes.push_back(rect);
  }
  debug_log("bound_boxes number: %ld\n", boxes.size());

  _merge_small_boxes(result, boxes, mask);
  debug_log("bound_boxes number after merging small boxes: %ld\n", result.size());

  debug_timer_end(t1, t2, imtools::bound_boxes);
}


double
get_avg_MSSIM(const Mat& i1, const Mat& i2)
{
  auto mssim = get_MSSIM(i1, i2);
  return (mssim.val[0] + mssim.val[1] + mssim.val[2]) / 3;
}

/// Computes structural similarity coefficient.
/// The code is borrowed from
/// http://docs.opencv.org/doc/tutorials/highgui/video-input-psnr-ssim/video-input-psnr-ssim.html#image-similarity-psnr-and-ssim
cv::Scalar
get_MSSIM(const Mat& i1, const Mat& i2)
{
  const double C1 = 6.5025, C2 = 58.5225;
  int d           = CV_32F;

  Mat I1, I2;
  i1.convertTo(I1, d); // cannot calculate on one byte large values
  i2.convertTo(I2, d);

  Mat I2_2  = I2.mul(I2); // I2^2
  Mat I1_2  = I1.mul(I1); // I1^2
  Mat I1_I2 = I1.mul(I2); // I1 * I2


  Mat mu1, mu2;
  cv::GaussianBlur(I1, mu1, Size(11, 11), 1.5);
  cv::GaussianBlur(I2, mu2, Size(11, 11), 1.5);

  Mat mu1_2   = mu1.mul(mu1);
  Mat mu2_2   = mu2.mul(mu2);
  Mat mu1_mu2 = mu1.mul(mu2);

  Mat sigma1_2, sigma2_2, sigma12;

  cv::GaussianBlur(I1_2, sigma1_2, Size(11, 11), 1.5);
  sigma1_2 -= mu1_2;

  cv::GaussianBlur(I2_2, sigma2_2, Size(11, 11), 1.5);
  sigma2_2 -= mu2_2;

  cv::GaussianBlur(I1_I2, sigma12, Size(11, 11), 1.5);
  sigma12 -= mu1_mu2;

  Mat t1, t2, t3;

  t1 = 2 * mu1_mu2 + C1;
  t2 = 2 * sigma12 + C2;
  t3 = t1.mul(t2);  // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))

  t1 = mu1_2 + mu2_2 + C1;
  t2 = sigma1_2 + sigma2_2 + C2;
  t1 = t1.mul(t2); // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))

  Mat ssim_map;
  cv::divide(t3, t1, ssim_map); // ssim_map =  t3./t1;

  cv::Scalar mssim = cv::mean(ssim_map); // mssim = average of ssim map
  return mssim;
}

} // namespace imtools
// vim: et ts=2 sts=2 sw=2
