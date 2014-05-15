#include "imtools.hxx"

using namespace std;
using namespace cv;

static const char* g_program_name;

static const char* g_usage_template = "Usage: %s <original_image> <template_image>\n\n"
"Outputs top left vertice coordinates of a rectangle within <original_image> which best matches <template_image>.\n\n"
"<original_image> - Some full fledged image\n"
"<template_image> - Some modified part of <original_image>\n";


static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr, g_usage_template, g_program_name);
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
      fprintf(stderr, "File %s doesn't exist\n", argv[i]);
      usage(true);
      return 1;
    }
  }

  Mat img, tpl, result;
  cv::Point match_loc;

  img = imread(argv[1], 1);
  tpl = imread(argv[2], 1);

  imtools::match_template(match_loc, img, tpl);

  printf("%d %d\n", match_loc.x, match_loc.y);

  return 0;
}
