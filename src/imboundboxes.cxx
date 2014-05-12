#include "imtools.hxx"

cv::Mat src, src_gray;
int thresh = 0;
int max_thresh = 255;
cv::RNG rng(12345);

void thresh_callback(int, void*);

int
main(int argc, char **argv)
{
  try {
    // Load source img and convert it to gray
    const char *filename = argv[1];
    if (!imtools::file_exists(filename)) {
      std::cerr << "Error: file doesn't exist: " << filename << std::endl;
      return 1;
    }
    // Load image, force 3 channels
    src = cv::imread(filename, 1);
    if (src.empty()) {
      std::cerr << "Error: image is empty" << std::endl;
      return 1;
    }

    // Convert image to gray and blur it
    cv::cvtColor(src, src_gray, CV_BGR2GRAY);
    cv::blur(src_gray, src_gray, cv::Size(15, 15));

    // Create window
    const char* window_title = "Source";
    cv::namedWindow(window_title, CV_WINDOW_AUTOSIZE);
    cv::imshow(window_title, src);

    cv::createTrackbar(" Threshold:", "Source", &thresh, max_thresh, thresh_callback);
    thresh_callback(0, 0);

    cv::waitKey(0);
  } catch (cv::Exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}

void
thresh_callback(int, void*)
{
  cv::Mat threshold_output;
  std::vector<std::vector<cv::Point> > contours;
  std::vector<cv::Vec4i> hierarchy;

  // Detect edges using threshold
  cv::threshold(src_gray, threshold_output, thresh, 255, cv::THRESH_BINARY);
  // Find contours
  cv::findContours(threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

  // Approximate contours to polygons, get bounding rects and circles
  std::vector<std::vector<cv::Point> > contours_poly(contours.size());
  std::vector<cv::Rect> boundRect(contours.size());

  for (int i = 0; i < contours.size(); ++i) {
    cv::approxPolyDP(cv::Mat(contours[i]), contours_poly[i], 3, true);
    boundRect[i] = cv::boundingRect(cv::Mat(contours_poly[i]));
  }

  // Draw polygonal contour + bonding rects + circles
  cv::Mat drawing = cv::Mat::zeros(threshold_output.size(), CV_8UC3);
  for (int i = 0; i < contours.size(); ++i) {
    cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
    cv::drawContours(drawing, contours_poly, i, color, 1, 8, std::vector<cv::Vec4i>(), 0, cv::Point());
    cv::rectangle(drawing, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0);
  }

  // Show in a window
  cv::namedWindow("Contours", CV_WINDOW_AUTOSIZE);
  cv::imshow("Contours", drawing);
}

