#include "imtools.hxx"

namespace imtools {

int
file_exists(const char *filename)
{
  struct stat st;
  return (stat(filename, &st) == 0);
}

void
diff(cv::Mat& out_img, const cv::Mat& old_img, const cv::Mat& new_img, const int mod_threshold)
{
  // Select likely modified pixels.
  // We try to something similar to command:
  // compare old.jpg new.jpg -fuzz 25%  -compose Src -highlight-color White -lowlight-color Black diff.jpg
  out_img = (old_img - new_img > mod_threshold);

  // Create matrix of the same size and type as diff_img
  //out_img.create(diff_img.size(), diff_img.type());

  // Convert to grayscale
  cv::cvtColor(out_img, out_img, CV_BGR2GRAY);
}

void
blur(cv::Mat& target, const blur_type type)
{
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
}

void
threshold(cv::Mat& target, const int threshold, const int max_threshold)
{
  assert(max_threshold >= threshold);
  cv::threshold(target, target, threshold, max_threshold, cv::THRESH_BINARY);
}

void
match_template(cv::Point& match_loc, const cv::Mat& img, const cv::Mat& tpl)
{
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
}

void
patch(cv::Mat& out_mat, const cv::Mat& img_mat, const cv::Mat& tpl_mat, const int x, const int y)
{
  if (tpl_mat.empty()) {
    throw std::runtime_error(std::string("Input template is empty"));
  }

  int x2 = x + tpl_mat.cols;
  int y2 = y + tpl_mat.rows;

  if (x2 > img_mat.rows || y2 > img_mat.cols) {
    throw std::runtime_error("Template is out of bounds");
  }

  // Region of interest
  cv::Rect roi = cv::Rect(x, y, tpl_mat.cols, tpl_mat.rows);

  // Output matrix
  out_mat = img_mat.clone();

  // Extract ROI from the output image as a reference
  cv::Mat out_mat_roi = out_mat(roi);

  // Overwrite the ROI
  tpl_mat.copyTo(out_mat_roi);
}

void
bound_boxes(bound_box_vector_t& boxes, const cv::Mat& mask, int min_threshold, int max_threshold)
{
  cv::Mat mask_gray, threshold_output;
  std::vector<std::vector<cv::Point> > contours;
  std::vector<cv::Vec4i> hierarchy;

  assert(min_threshold >= 0 && min_threshold <= max_threshold);

  // Convert image to gray and blur it
  debug_log("bound_boxes - 1, channels = %d\n", mask.channels());
  if (mask.channels() >= 3) {
    cv::cvtColor(mask, mask_gray, CV_BGR2GRAY);
  } else {
    mask_gray = mask;
  }
  debug_log("bound_boxes - blur()\n");
  cv::blur(mask_gray, mask_gray, cv::Size(15, 15));

  // Detect edges
  debug_log("bound_boxes - threshold(%d, %d)\n", min_threshold, max_threshold);
  cv::threshold(mask_gray, threshold_output, min_threshold, max_threshold, cv::THRESH_BINARY);

  cv::findContours(threshold_output, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

  // Approximate contours to polygons, get bounding rects
  std::vector<std::vector<cv::Point> > contours_poly(contours.size());
  boxes.reserve(contours.size());
  debug_log("bound_boxes - reserved %ld\n", contours.size());
  for (int i = 0; i < contours.size(); ++i) {
    cv::approxPolyDP(cv::Mat(contours[i]), contours_poly[i], 3, true);
    boxes.push_back(cv::boundingRect(cv::Mat(contours_poly[i])));
    debug_log("bound_boxes - boxes[i] = %d x %d\n", boxes[i].x, boxes[i].y);
  }
  debug_log("bound_boxes - size = %ld\n", boxes.size());
}


} // namespace imtools
