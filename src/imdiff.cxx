#include <getopt.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "imtools.hxx"


static const char* g_program_name;

static std::string g_out_filename;


// Minimum difference between old and new matrix elements specifying
// whether element is considered to be modified.
//static int g_modification_threshold = 40;
static const int g_max_modification_threshold = 200;

static int g_thresh = 20;
// g_max_thresh has effect with cv::THRESH_BINARY and cv::THRESH_BINARY_INV only.
static const int g_max_thresh = 255;

static cv::Mat g_out;
static cv::Mat diff_img, old_img, new_img;

const char *result_window_title = "Result";

static imtools::Blur g_Blur = imtools::BLUR_NONE;

static const char* usage_template = "Usage: %s OPTIONS old_image new_image\n\n"
"Computes difference between two images of the same size.\n\n"
" -h, --help           Display this help.\n"
" -v, --verbose        Verbose mode.\n"
" -o, --output         Filename of the output image. Required.\n";

static const char* g_short_options = "hvo:";

static const struct option g_long_options[] = {
  {"help",    no_argument,       NULL, 'h'},
  {"verbose", no_argument,       NULL, 'v'},
  {"output",  required_argument, NULL, 'o'},
  {0,         0,                 0,    0}
};


static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr, usage_template, g_program_name);
  exit(is_error ? 1 : 0);
}

static void
modification_thresh_callback(int, void*)
{
  imtools::diff(g_out, old_img, new_img);

  cv::imshow(result_window_title, g_out);
}

static void
thresh_callback(int, void*)
{
  modification_thresh_callback(0, 0);

  imtools::threshold(g_out);

  if (g_Blur != imtools::BLUR_NONE) {
    imtools::blur(g_out, g_Blur);
  }

  cv::imshow(result_window_title, g_out);
}

static void
blur_callback(int state, void* user_data)
{
  if (state != 1) {
    return;
  }

  g_Blur = (imtools::Blur)(intptr_t)user_data;

  thresh_callback(0, 0);
}

static void
show_diff(const std::string& filename_old, const std::string& filename_new)
{
  const char* window_title = "Source";

  // Load images forcing them to be 3-channel
  // (for converting to grayscale)
  old_img = cv::imread(filename_old, 1);
  new_img = cv::imread(filename_new, 1);

  cv::namedWindow(window_title, CV_WINDOW_NORMAL);

  cv::createButton("No blur",     blur_callback, (void *) imtools::BLUR_NONE,   CV_RADIOBOX);
  cv::createButton("Blur",        blur_callback, (void *) imtools::BLUR,        CV_RADIOBOX);
  cv::createButton("Gauss blur",  blur_callback, (void *) imtools::BLUR_GAUSS,  CV_RADIOBOX);
  cv::createButton("Median blur", blur_callback, (void *) imtools::BLUR_MEDIAN, CV_RADIOBOX);

  //cv::createTrackbar(" Mod Thresh:", window_title, &g_modification_threshold, g_max_modification_threshold, modification_thresh_callback);
  cv::createTrackbar(" Threshold:",  window_title, &g_thresh,                 g_max_thresh,                 thresh_callback);

  cv::imshow(window_title, new_img);

  cv::namedWindow(result_window_title, CV_WINDOW_AUTOSIZE);

  thresh_callback(0, 0);

  cv::waitKey(0);
}

int main(int argc, char** argv)
{
  g_program_name = argv[0];

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
        imtools::verbose++;
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
    verbose_log("Old file: %s, new file: %s", filename_old.c_str(), filename_new.c_str());
  } else {
    fprintf(stderr, "Error: Invalid input.\n");
    usage(true);
  }

  try {
    show_diff(filename_old, filename_new);

    if (imtools::verbose) {
      if (imtools::file_exists(g_out_filename.c_str())) {
        fprintf(stderr, "Warning: File %s will be overwritten\n", g_out_filename.c_str());
      }
      printf("* Writing to %s\n", g_out_filename.c_str());
    }
    if (g_out_filename.length() == 0) {
      fprintf(stderr, "Error: No output file specified.\n");
      usage(true);
    }
    cv::imwrite(g_out_filename, g_out);
  } catch (cv::Exception& e) {
    fprintf(stderr, "Error: %s\n", e.what());
    return 1;
  }

  return 0;
}
