#include <vector>
#include "imtools.hxx"

static const char* g_program_name;

// Coordinates
static int g_x, g_y;

// Template image filename
static std::string g_tpl_filename;

// Destination images
typedef std::vector<std::string> images_vector_t;
static images_vector_t g_dst_images;


static const char* usage_template = IMTOOLS_FULL_NAME "\n\n" IMTOOLS_COPYRIGHT "\n\n"
"Usage: %s OPTIONS <image_1>[, <image_2>[, ...]]\n\n"
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
    fprintf(stderr, "Error: Failed to read template %s", g_tpl_filename.c_str());
    usage(true);
  }
  int x2 = g_x + tpl_mat.cols, y2 = g_y + tpl_mat.rows;
  verbose_log("Template rect. coords: %d %d %d %d", g_x, g_y, x2, y2);

  for (images_vector_t::iterator it = g_dst_images.begin(); it != g_dst_images.end(); ++it) {
    verbose_log("Patching '%s'...", it->c_str());

    cv::Mat out_mat;
    cv::Mat img_mat = cv::imread(*it);
    cv::Rect roi = cv::Rect(g_x, g_y, tpl_mat.cols, tpl_mat.rows);

    imtools::patch(out_mat, img_mat, tpl_mat, roi);

    std::string out_filename = "out_" + g_tpl_filename;
    verbose_log("Writing to %s", out_filename.c_str());
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
            error_log("Non-digit provided for x option\n");
            usage(true);
          }
          break;
        }

      case 'y':
        {
          char *optarg_end;
          g_y = strtol(optarg, &optarg_end, 10);
          if (*optarg_end != '\0') {
            error_log("Non-digit provided for y option\n");
            usage(true);
          }
          break;
        }

      case 't':
        g_tpl_filename = optarg;
        if (!imtools::file_exists(g_tpl_filename.c_str())) {
          fprintf(stderr, "File %s doesn't exist", g_tpl_filename.c_str());
          usage(true);
        }
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
        error_log("getopt returned character code 0%o\n", next_option);
        usage(true);
    }
  } while (next_option != -1);

  if (optind < argc) {
    while (optind < argc) {
      const char *filename = argv[optind++];
      if (!imtools::file_exists(filename)) {
        error_log("image %s doesn't exist.\n", filename);
        break;
      }
      g_dst_images.push_back(filename);
    }
  } else {
    error_log("No valid destination images are specified\n");
    usage(true);
  }

  try {
    patch();
  } catch (cv::Exception& e) {
    error_log("%s\n", e.what());
    return 1;
  }

  return 0;
}
