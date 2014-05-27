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

namespace imtools {

int verbose = 0;

void
diff(cv::Mat& out_img, const cv::Mat& old_img, const cv::Mat& new_img, const int mod_threshold)
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
blur(cv::Mat& target, const blur_type type)
{
  debug_timer_init(t1, t2);
  debug_timer_start(t1);

  switch (type) {
    case BLUR_NONE:
      break;

    case BLUR:
      cv::blur(target, target, cv::Size(3, 3));
      break;

    case BLUR_GAUSS:
      cv::GaussianBlur(target, target, cv::Size(3, 3), 10);
      break;

    case BLUR_MEDIAN:
      cv::medianBlur(target, target, 9);
      break;

    default:
      throw std::runtime_error(std::string("Invalid blur type"));
  }

  debug_timer_end(t1, t2, imtools::blur);
}


void
threshold(cv::Mat& target, const int threshold, const int max_threshold)
{
  debug_timer_init(t1, t2);
  debug_timer_start(t1);

  assert(max_threshold >= threshold);

  cv::threshold(target, target, threshold, max_threshold, cv::THRESH_BINARY);

  debug_timer_end(t1, t2, imtools::threshold);
}


void
match_template(cv::Point& match_loc, const cv::Mat& img, const cv::Mat& tpl)
{
  debug_timer_init(t1, t2);
  debug_timer_start(t1);

  cv::Mat result;
  int match_method = CV_TM_SQDIFF;

  int result_cols = img.cols - tpl.cols + 1;
  int result_rows = img.rows - tpl.rows + 1;
  result.create(result_cols, result_rows, CV_32FC1);

  cv::matchTemplate(img, tpl, result, match_method);
  cv::normalize(result, result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());

  // Localize the best match with minMaxLoc

  double min_val, max_val;
  cv::Point min_loc, max_loc;
  cv::minMaxLoc(result, &min_val, &max_val, &min_loc, &max_loc, cv::Mat());

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
patch(cv::Mat& out_mat, const cv::Mat& img_mat, const cv::Mat& tpl_mat, const cv::Rect& roi)
{
  debug_log("imtools::patch(), x: %d, y: %d\n", roi.x, roi.y);
  debug_timer_init(t1, t2);
  debug_timer_start(t1);

  if (out_mat.empty()) {
    throw std::runtime_error(std::string("Output matrix is empty"));
  }
  if (tpl_mat.empty()) {
    throw std::runtime_error(std::string("Input template matrix is empty"));
  }

  assert(!img_mat.empty());
  //assert(out_mat.rows >= img_mat.rows && out_mat.cols >= img_mat.cols);
  assert(out_mat.type() == img_mat.type());
  assert(img_mat.rows >= tpl_mat.rows && img_mat.cols >= tpl_mat.cols);

  if ((roi.x + tpl_mat.cols) > out_mat.cols
      || (roi.y + tpl_mat.rows) > out_mat.rows)
  {
    throw TemplateOutOfBoundsException(tpl_mat, out_mat, roi);
  }

  // Region of interest
  //cv::Rect roi = cv::Rect(pt.x, pt.y, tpl_mat.cols, tpl_mat.rows);

  // Output matrix should be prepared already.
  // out_mat = img_mat.clone();

  // Extract ROI from the output image as a reference
  cv::Mat out_mat_roi = out_mat(roi);

  // Overwrite the ROI
  tpl_mat.copyTo(out_mat_roi);

  debug_timer_end(t1, t2, imtools::patch);
}


void
bound_boxes(bound_box_vector_t& boxes, const cv::Mat& mask, int min_threshold, int max_threshold)
{
  debug_timer_init(t1, t2);
  debug_timer_start(t1);

  cv::Mat mask_gray, threshold_output;
  std::vector<std::vector<cv::Point> > contours;
  std::vector<cv::Vec4i> hierarchy;

  assert(min_threshold >= 0 && min_threshold <= max_threshold);

  // Convert image to gray and blur it

  if (mask.channels() >= 3) {
    debug_log("bound_boxes: cvtColor() to grayscale, channels = %d\n", mask.channels());
    cv::cvtColor(mask, mask_gray, CV_BGR2GRAY);
  } else {
    mask_gray = mask;
  }
  debug_log0("bound_boxes: blur() w/ 15x15 kernel\n");
  cv::blur(mask_gray, mask_gray, cv::Size(15, 15));

  // Detect edges
  debug_log("bound_boxes: threshold(%d, %d)\n", min_threshold, max_threshold);
  cv::threshold(mask_gray, threshold_output, min_threshold, max_threshold, cv::THRESH_BINARY);

  cv::findContours(threshold_output, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

  // Approximate contours to polygons, get bounding rects
  std::vector<std::vector<cv::Point> > contours_poly(contours.size());
  boxes.reserve(contours.size());
  debug_log("bound_boxes - reserved %ld\n", contours.size());
  for (size_t i = 0; i < contours.size(); ++i) {
    cv::approxPolyDP(cv::Mat(contours[i]), contours_poly[i], 3, true);
    boxes.push_back(cv::boundingRect(cv::Mat(contours_poly[i])));
    debug_log("bound_boxes - boxes[i] = %dx%d\n", boxes[i].x, boxes[i].y);
  }
  debug_log("bound_boxes - size = %ld\n", boxes.size());

  debug_timer_end(t1, t2, imtools::bound_boxes);
}


/// Computes structural similarity coefficient.
/// The code is borrowed from
/// http://docs.opencv.org/doc/tutorials/highgui/video-input-psnr-ssim/video-input-psnr-ssim.html#image-similarity-psnr-and-ssim
cv::Scalar get_MSSIM(const cv::Mat& i1, const cv::Mat& i2)
{
  const double C1 = 6.5025, C2 = 58.5225;
  int d           = CV_32F;

  cv::Mat I1, I2;
  i1.convertTo(I1, d); // cannot calculate on one byte large values
  i2.convertTo(I2, d);

  cv::Mat I2_2  = I2.mul(I2); // I2^2
  cv::Mat I1_2  = I1.mul(I1); // I1^2
  cv::Mat I1_I2 = I1.mul(I2); // I1 * I2


  cv::Mat mu1, mu2;
  cv::GaussianBlur(I1, mu1, cv::Size(11, 11), 1.5);
  cv::GaussianBlur(I2, mu2, cv::Size(11, 11), 1.5);

  cv::Mat mu1_2   = mu1.mul(mu1);
  cv::Mat mu2_2   = mu2.mul(mu2);
  cv::Mat mu1_mu2 = mu1.mul(mu2);

  cv::Mat sigma1_2, sigma2_2, sigma12;

  cv::GaussianBlur(I1_2, sigma1_2, cv::Size(11, 11), 1.5);
  sigma1_2 -= mu1_2;

  cv::GaussianBlur(I2_2, sigma2_2, cv::Size(11, 11), 1.5);
  sigma2_2 -= mu2_2;

  cv::GaussianBlur(I1_I2, sigma12, cv::Size(11, 11), 1.5);
  sigma12 -= mu1_mu2;

  cv::Mat t1, t2, t3;

  t1 = 2 * mu1_mu2 + C1;
  t2 = 2 * sigma12 + C2;
  t3 = t1.mul(t2);  // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))

  t1 = mu1_2 + mu2_2 + C1;
  t2 = sigma1_2 + sigma2_2 + C2;
  t1 = t1.mul(t2); // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))

  cv::Mat ssim_map;
  cv::divide(t3, t1, ssim_map); // ssim_map =  t3./t1;

  cv::Scalar mssim = cv::mean(ssim_map); // mssim = average of ssim map
  return mssim;
}

} // namespace imtools
// vim: et ts=2 sts=2 sw=2
