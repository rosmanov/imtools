#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "imtools.hxx"

static const char* g_program_name;

static const char* usage_template = IMTOOLS_FULL_NAME "\n\n" IMTOOLS_COPYRIGHT "\n\n"
"Usage: %s <original_image> <template_image>\n\n"
"Outputs top left vertice coordinates of a rectangle within <original_image> which best matches <template_image>.\n\n"
"<original_image> - Some full fledged image\n"
"<template_image> - Some modified part of <original_image>\n";


static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr, usage_template, g_program_name);
  exit(is_error ? 1 : 0);
}


int main(int argc, char **argv)
{
  g_program_name = argv[0];

  if (argc < 3) {
    usage(true);
    return 1;
  }

  for (int i = 1; i < 3; i++) {
    if (!imtools::file_exists(argv[i])) {
      error_log("File %s doesn't exist", argv[i]);
      usage(true);
      return 1;
    }
  }

  cv::Mat img, tpl, result;
  cv::Point match_loc;

  img = cv::imread(argv[1], 1);
  tpl = cv::imread(argv[2], 1);

  imtools::match_template(match_loc, img, tpl);

  printf("%d %d\n", match_loc.x, match_loc.y);

  return 0;
}
