#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <getopt.h>
#include <vector>


static const char* g_program_name;

// Coordinates
static int g_x, g_y;

static bool g_verbose = false;

// Template image filename
static std::string g_tpl_filename;

// Destination images
typedef std::vector<std::string> images_vector_t;
static images_vector_t g_dst_images;


static const char* usage_template = "Usage: %s OPTIONS <image_1>[, <image_2>[, ...]]\n\n"
"Applies image template to <image_1>, <image_2> etc.\n\n"
"OPTIONS:\n"
"  -h, --help           Display this help.\n"
"  -v, --verbose        Verbose mode.\n"
"  -x                   X coordinate.\n"
"  -y                   Y coordinate.\n"
"  -t, --template       Template image.\n";

static const char *short_options = "hvx:y:t:";

static const struct option long_options[] = {
  {"help",     no_argument,       NULL, 'h'},
  {"verbose",  no_argument,       NULL, 'v'},
  {"template", required_argument, NULL, 't'},
  {0,          0,                 0,    0}
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
patch()
{
  cv::Mat tpl_mat = cv::imread(g_tpl_filename);
  if (tpl_mat.empty()) {
    fprintf(stderr, "Error: Failed to read template %s\n", g_tpl_filename.c_str());
    usage(true);
  }
  int x2 = g_x + tpl_mat.cols, y2 = g_y + tpl_mat.cols;
  if (g_verbose) {
    printf("* Template rect. coords: %d %d %d %d\n", g_x, g_y, x2, y2);
  }

  for (images_vector_t::iterator it = g_dst_images.begin(); it != g_dst_images.end(); ++it) {
    if (g_verbose) {
      printf("* Patching '%s'...\n", it->c_str());
    }

    cv::Mat img_mat = cv::imread(*it);
    if (x2 > img_mat.rows || y2 > img_mat.cols) {
      fprintf(stderr, "Warning: template is out of bounds %dx%d, skipping!\n", img_mat.cols, img_mat.rows);
      img_mat.release();
      continue;
    }

    // Region of interest (ROI)
    cv::Rect roi = cv::Rect(g_x, g_y, tpl_mat.cols, tpl_mat.rows);

    // Output matrix
    cv::Mat out_mat = img_mat.clone();

    // Extract ROI from the output image as a reference
    cv::Mat out_mat_roi = out_mat(roi);

    // Overwrite the ROI
    tpl_mat.copyTo(out_mat_roi);

    std::string out_filename = "out_" + g_tpl_filename;
    if (g_verbose) {
      printf("* Writing to %s\n", out_filename.c_str());
    }
    cv::imwrite(out_filename, out_mat);
  }
}

int main(int argc, char **argv)
{
  int next_option;

  g_program_name = argv[0];

  do {
    next_option = getopt_long(argc, argv, short_options,
        long_options, NULL);

    switch (next_option) {
     case 'h':
        usage(false);

     case 'x':
        {
          char *optarg_end;
          g_x = strtol(optarg, &optarg_end, 10);
          if (*optarg_end != '\0') {
            fprintf(stderr, "Non-digit provided for x option\n");
            usage(true);
          }
          break;
        }

      case 'y':
        {
          char *optarg_end;
          g_y = strtol(optarg, &optarg_end, 10);
          if (*optarg_end != '\0') {
            fprintf(stderr, "Non-digit provided for y option\n");
            usage(true);
          }
          break;
        }

      case 't':
        g_tpl_filename = optarg;
        if (!file_exists(g_tpl_filename.c_str())) {
          fprintf(stderr, "File %s doesn't exist", g_tpl_filename.c_str());
          usage(true);
        }
        break;

      case 'v':
        g_verbose = true;
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

  if (optind < argc) {
    while (optind < argc) {
      const char *filename = argv[optind++];
      if (!file_exists(filename)) {
        fprintf(stderr, "Error: image %s doesn't exist.\n", filename);
        break;
      }
      g_dst_images.push_back(filename);
    }
  } else {
    fprintf(stderr, "No valid destination images are specified.\n");
    usage(true);
  }

  try {
    patch();
  } catch (cv::Exception& e) {
    fprintf(stderr, "Error: %s\n", e.what());
    return 1;
  }

  return 0;
}
