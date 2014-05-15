#include <vector>
#include <iostream>
#include "imtools.hxx"

static const char* g_program_name;

static bool g_verbose = false;

static std::string g_old_image_filename, g_new_image_filename;

// Destination images
typedef std::vector<std::string> images_vector_t;
static images_vector_t g_dst_images;


static const char* usage_template = "Usage: %s OPTIONS [<image_1>[, <image_2>[, ...]]]\n\n"
"Calculates difference between two images specified by --old-image and --new-image;\n"
"applies the difference to <image_1>, <image_2> etc.\n"
"The tool can be useful to update a logo or some common elements on a set of \"similar\" images.\n\n"
"OPTIONS:\n"
" -h, --help           Display this help.\n"
" -v, --verbose        Verbose mode.\n"
" -n, --new-image      New image.\n"
" -o, --old-image      Old image.\n";

static const char *short_options = "hvn:o:";

static const struct option long_options[] = {
  {"help",      no_argument,       NULL, 'h'},
  {"verbose",   no_argument,       NULL, 'v'},
  {"new-image", required_argument, NULL, 'n'},
  {"old-image", required_argument, NULL, 'o'},
  {0,           0,                 0,    0}
};


static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr, usage_template, g_program_name);
  exit(is_error ? 1 : 0);
}

static void
merge()
{
  printf("merge()\n");
  cv::Point match_loc;
  cv::Mat out_img, old_img, new_img, diff_img, tpl_img;
  std::string merged_filename;

  old_img = cv::imread(g_old_image_filename, 1);
  new_img = cv::imread(g_new_image_filename, 1);

  printf("merge - 1\n");
  imtools::diff(diff_img, old_img, new_img);
  printf("merge - 2\n");
  imtools::threshold(diff_img);
  printf("merge - 3\n");

  g_dst_images.push_back(g_old_image_filename);

  for (images_vector_t::iterator it = g_dst_images.begin(); it != g_dst_images.end(); ++it) {
    if (g_verbose) {
      printf("* Image %s ...\n", it->c_str());
    }

    old_img = cv::imread(*it, 1);
    if (old_img.empty()) {
      std::cerr << "Warning: skipping empty image: " << *it << std::endl;
      continue;
    }

    imtools::bound_box_vector_t boxes;
    printf("merge - 4\n");
    imtools::bound_boxes(boxes, diff_img);
    printf("merge - 5, boxes.size() = %ld\n", boxes.size());


    for (int i = 0; i < boxes.size(); i++) {
      // Get tpl_img from original image using boxes[i] as a mask
      tpl_img = cv::Mat(new_img, boxes[i]);

      printf("match_template() - boxes[%d]: %dx%d\n", i, boxes[i].x, boxes[i].y);
      imtools::match_template(match_loc, old_img, tpl_img);

      std::ostringstream osstr;
      osstr << "rect" << i << ".jpg";
      std::string rect_filename = osstr.str();
      printf("* writing to %s\n", rect_filename.c_str());
      cv::imwrite(rect_filename, tpl_img);

      printf("patch() - %d\n", i);
      imtools::patch(out_img, old_img, tpl_img, match_loc.x, match_loc.y);

      osstr.clear();
      osstr.str("");
      osstr << "step" << i << ".jpg";
      merged_filename = osstr.str();
      std::cout << "* Writing to " << merged_filename << std::endl;
      cv::imwrite(merged_filename, out_img);
    }

    merged_filename = "merged_" + g_old_image_filename;
    if (g_verbose) {
      std::cout << "* Writing to " << merged_filename << std::endl;
    }
    cv::imwrite(merged_filename, out_img);
  }
}

int main(int argc, char **argv)
{
  int next_option;

  g_program_name = argv[0];

  do {
    next_option = getopt_long(argc, argv, short_options, long_options, NULL);

    switch (next_option) {
     case 'h':
        usage(false);

      case 'n':
      case 'o':
        if (!imtools::file_exists(optarg)) {
          fprintf(stderr, "File %s doesn't exist", optarg);
          usage(true);
        }
        if (next_option == 'n') {
          g_new_image_filename = optarg;
        } else {
          g_old_image_filename = optarg;
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
    printf("%d < %d\n", optind, argc);
    while (optind < argc) {
      const char *filename = argv[optind++];
      if (!imtools::file_exists(filename)) {
        fprintf(stderr, "Error: image %s doesn't exist.\n", filename);
        break;
      }
      g_dst_images.push_back(filename);
    }
  }

  if (g_old_image_filename.length() == 0 || g_new_image_filename.length() == 0) {
    fprintf(stderr, "Error: invalid input.\n");
    usage(true);
  }

  try {
    merge();
  } catch (cv::Exception& e) {
    fprintf(stderr, "Error: %s\n", e.what());
    return 1;
  }

  return 0;
}
