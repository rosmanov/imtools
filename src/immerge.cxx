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
"Obviously, all images have to be identical.\n\n"
"The tool can be useful to update a logo or some common element on a set of \"similar\" images.\n\n"
"OPTIONS:\n"
"  -h, --help           Display this help.\n"
"  -v, --verbose        Verbose mode.\n"
"  -n, --new-image      New image.\n"
"  -o, --old-image      Old image.\n";

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
  cv::Mat old_img, new_img, diff_img;
  std::string merged_filename;

  old_img = cv::imread(g_old_image_filename);
  new_img = cv::imread(g_new_image_filename);

  // Select unequal pixels
  diff_img = (old_img != new_img);

  g_dst_images.push_back(g_old_image_filename);

  for (images_vector_t::iterator it = g_dst_images.begin(); it != g_dst_images.end(); ++it) {
    old_img = cv::imread(*it);
    if (old_img.empty()) {
      std::cerr << "Warning: skipping empty image: " << *it << std::endl;
      continue;
    }

    new_img.copyTo(old_img, diff_img);

    merged_filename = "merged_" + g_old_image_filename;
    if (g_verbose) {
      std::cout << "* Writing to " << merged_filename << std::endl;
    }
    cv::imwrite(merged_filename, old_img);
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
    while (optind < argc) {
      const char *filename = argv[optind++];
      if (!imtools::file_exists(filename)) {
        fprintf(stderr, "Error: image %s doesn't exist.\n", filename);
        break;
      }
      g_dst_images.push_back(filename);
    }
  }

  try {
    merge();
  } catch (cv::Exception& e) {
    fprintf(stderr, "Error: %s\n", e.what());
    return 1;
  }

  return 0;
}
