#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <getopt.h>


static const char* g_program_name;

static std::string g_out_filename;

static bool g_verbose = false;

static bool g_crop = false;

static int g_kernel_size = 3;
static int g_ratio = 3;
static const int g_low_threshold = 10;
static const int g_max_low_threshold = 100;

static const char* usage_template = "Usage: %s OPTIONS old_image new_image\n\n"
"Computes difference between two images of the same size.\n\n"
" -h, --help           Display this help.\n"
" -v, --verbose        Verbose mode.\n"
" -o, --output         Filename of the output image. Required.\n"
" -c, --crop           Crop output image up to rectangle with differences.\n";

static const char* g_short_options = "hvo:c";

static const struct option g_long_options[] = {
  {"help",    no_argument,       NULL, 'h'},
  {"verbose", no_argument,       NULL, 'v'},
  {"output",  required_argument, NULL, 'o'},
  {"crop",    required_argument, NULL, 'c'},
  {0,         0,                 0,    0}
};


static int
file_exists(const char *filename)
{
  struct stat st;
  return (stat(filename, &st) == 0);
}

static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr, usage_template, g_program_name);
  exit(is_error ? 1 : 0);
}

static void
highlightEdges(cv::Mat& img, cv::Mat& out)
{
  cv::Mat src_grey, detected_edges;

  // Create matrix of the same size and type as img
  out.create(img.size(), img.type());

  // Convert to greyscale
  cv::cvtColor(img, src_grey, CV_BGR2GRAY);

  // Reduce noise with a kernel g_kernel_size x g_kernel_size
  cv::blur(src_grey, detected_edges, cv::Size(g_kernel_size, g_kernel_size));

  // Canny detector
  cv::Canny(detected_edges, detected_edges, g_low_threshold, g_low_threshold * g_ratio, g_kernel_size);

  out = cv::Scalar::all(0);

  img.copyTo(out, detected_edges);
}

static void
crop(cv::Mat& img) {
  if (g_verbose) {
    printf("* Cropping\n");
  }

  // Some isolated pixels distorting results.
  //cv::Mat kernel = cv::Mat::ones(2, 2, CV_32F);
  //cv::erode(img, img, kernel);
  //cv::filter2D(img, img, -1, kernel, cv::Point(-1, -1), 0, cv::BORDER_DEFAULT);

  int ev;
  int min_i = img.rows - 1, max_i = 0, min_j = img.cols - 1, max_j = 0;

  // Search for edges of the modified (non-zero) area

  for (int i = 0; i < img.rows; i++) {
    for (int j = 0; j < img.cols; j++) {
      ev = img.at<int>(i, j);
      //printf("%o ", ev);

      if (ev == 0) {
        continue;
      }

      if (i < min_i) {
        min_i = i;
      }
      if (i > max_i) {
        max_i = i;
      }
      if (j < min_j) {
        min_j = j;
      }
      if (j > max_j) {
        max_j = j;
      }
    }
    //printf("\n");
  }

  if (g_verbose) {
    printf("min_i: %d, max_i: %d, min_j: %d, max_j: %d\n",
        min_i, max_i, min_j, max_j);
  }
}

static void
diff(const std::string& filename_old, const std::string& filename_new)
{
  cv::Mat diff_img, old_img, new_img;


  old_img = cv::imread(filename_old);
  new_img = cv::imread(filename_new);

  // Select unequal pixels
  diff_img = (old_img != new_img);



  cv::Mat out_img;
  highlightEdges(diff_img, out_img);

  if (g_verbose) {
    if (file_exists(g_out_filename.c_str())) {
      fprintf(stderr, "Warning: File %s will be overwritten\n", g_out_filename.c_str());
    }
    printf("* Writing to %s\n", g_out_filename.c_str());
  }

  if (g_crop) {
    crop(diff_img);
  }


  cv::imwrite(g_out_filename, out_img);
}


int main(int argc, char** argv)
{
  g_program_name = argv[0];

  // Parse CLI args

  int next_option;
  std::string filename_old, filename_new;

  do {
    next_option = getopt_long(argc, argv, g_short_options, g_long_options, NULL);

    switch (next_option) {
     case 'h':
        usage(false);

      case 'o':
        g_out_filename = optarg;
        break;

      case 'v':
        g_verbose = true;
        break;

      case 'c':
        g_crop = true;
        break;

      case -1:
        // done with options
        break;

      case '?':
        // unrecognized option
        usage(true);

      default:
        fprintf(stderr, "Error: getopt returned character code 0%o\n", next_option);
        usage(true);
    }
  } while (next_option != -1);

  if (argc - optind == 2) {
    filename_old = argv[optind++];
    filename_new = argv[optind++];
    if (g_verbose) {
      printf("* Old file: %s, new file: %s\n", filename_old.c_str(), filename_new.c_str());
    }
  } else {
    fprintf(stderr, "Error: Invalid input.\n");
    usage(true);
  }

  // Calc diff and write it to the output file

  try {
    diff(filename_old, filename_new);
  } catch (cv::Exception& e) {
    fprintf(stderr, "Error: %s\n", e.what());
    return 1;
  }

  return 0;
}
