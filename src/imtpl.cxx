#include "imtools.hxx"

using namespace std;
using namespace cv;


static void
usage(const char *program)
{
  printf("Usage: %s <original_image> <template_image>\n\n"
      "Outputs top left vertice coordinates of a rectangle within <original_image> which best matches <template_image>.\n\n"
      "<original_image> - Some full fledged image\n"
      "<template_image> - Some modified part of <original_image>\n", program);
}

int main(int argc, const char *argv[])
{
  if (argc < 3) {
    usage(argv[0]);
    return 1;
  }

  for (int i = 1; i < 3; i++) {
    if (!imtools::file_exists(argv[i])) {
      fprintf(stderr, "File %s doesn't exist\n", argv[i]);
      usage(argv[0]);
      return 1;
    }
  }

  Mat img, tpl, result;
  int match_method = CV_TM_SQDIFF;

  img = imread(argv[1], 1);
  tpl = imread(argv[2], 1);

  int result_cols = img.cols - tpl.cols + 1;
  int result_rows = img.rows - tpl.rows + 1;
  result.create(result_cols, result_rows, CV_32FC1);

  matchTemplate(img, tpl, result, match_method);
  normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());

  // Localize the best match with minMaxLoc

  double minVal, maxVal;
  Point minLoc, maxLoc, matchLoc;
  minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

  // For SQDIFF and SQDIFF_NORMED, the best matches are lower values. For all
  // the other methods, the higher the better
  if( match_method  == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED ) {
    matchLoc = minLoc;
  } else {
    matchLoc = maxLoc;
  }

  printf("%d %d\n", matchLoc.x, matchLoc.y);

  return 0;
}
