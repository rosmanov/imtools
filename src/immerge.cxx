#include <vector>
#include <iostream>
#include "imtools.hxx"

static const char* g_program_name;

static bool g_verbose = false;

static int g_mod_threshold = imtools::THRESHOLD_MOD;
static int g_min_threshold = imtools::THRESHOLD_MIN;
static int g_max_threshold = imtools::THRESHOLD_MAX;
static int g_min_boxes_threshold = imtools::THRESHOLD_BOXES_MIN;
static int g_max_boxes_threshold = imtools::THRESHOLD_BOXES_MAX;

static std::string g_old_image_filename,
  g_new_image_filename,
  g_out_dir;

static cv::Mat g_old_img, g_new_img;

// Destination images
typedef std::vector<std::string> images_vector_t;
static images_vector_t g_dst_images;

static const char* usage_template = "Usage: %s OPTIONS <image_1>[, <image_2>[, ...]]\n\n"
"Calculates difference between two images specified by --old-image and --new-image;\n"
"applies the difference to <image_1>, <image_2> etc.\n"
"The tool can be useful to update a logo or some common elements on a set of \"similar\" images.\n\n"
"OPTIONS:\n"
" -h, --help                 Display this help.\n"
" -v, --verbose              Verbose mode.\n"
" -n, --new-image            New image.\n"
" -o, --old-image            Old image.\n"
" -d, --out-dir              Output directory. Default: current directory.\n"
" -m, --mod-threshold        Modification threshold. Default: %d\n"
" -L, --min-threshold        Min. noise suppression threshold. Default: %d\n"
" -H, --max-threshold        Max. noise suppression threshold. Default: %d\n"
" -b, --boxes-min-threshold  Min. threshold for bound boxes. Default: %d\n"
" -B, --boxes-max-threshold  Max. threshold for bound boxes. Default: %d\n";

static const char *short_options = "hvn:o:dmLM";

static const struct option long_options[] = {
  {"help",                no_argument,       NULL, 'h'},
  {"verbose",             no_argument,       NULL, 'v'},
  {"new-image",           required_argument, NULL, 'n'},
  {"old-image",           required_argument, NULL, 'o'},
  {"out-dir",             required_argument, NULL, 'd'},
  {"mod-threshold",       optional_argument, NULL, 'm'},
  {"min-threshold",       optional_argument, NULL, 'L'},
  {"max-threshold",       optional_argument, NULL, 'H'},
  {"boxes-min-threshold", optional_argument, NULL, 'b'},
  {"boxes-max-threshold", optional_argument, NULL, 'B'},
  {0,                     0,                 0,    0}
};


static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr, usage_template,
      g_program_name,
      imtools::THRESHOLD_MOD,
      imtools::THRESHOLD_MIN,
      imtools::THRESHOLD_MAX,
      imtools::THRESHOLD_BOXES_MIN,
      imtools::THRESHOLD_BOXES_MAX);
  exit(is_error ? 1 : 0);
}


static void
load_images(const int argc, char** argv)
{
  // Load target images

  if (optind < argc) {
    while (optind < argc) {
      const char *filename = argv[optind++];
      if (!imtools::file_exists(filename)) {
        fprintf(stderr, "Error: image %s doesn't exist.\n", filename);
        break;
      }
      g_dst_images.push_back(filename);
    }
  } else {
    fprintf(stderr, "Error: Target image(s) expected. "
        "You don't need this tool just to replace one image with another ;)");
    usage(true);
  }

  if (g_old_image_filename.length() == 0 || g_new_image_filename.length() == 0) {
    fprintf(stderr, "Error: invalid input.\n");
    usage(true);
  }

  // Load the two images which will specify the modificatoin to be applied to each of g_dst_images;
  // force 3 channels

  g_old_img = cv::imread(g_old_image_filename, 1);
  g_new_img = cv::imread(g_new_image_filename, 1);

  if (g_old_img.size() != g_new_img.size()) {
    fprintf(stderr, "Error: Input images have different dimensions.\n");
    usage(true);
  }
  if (g_old_img.type() != g_new_img.type()) {
    fprintf(stderr, "Error: Input images have different types.\n");
    usage(true);
  }
}


static void
merge()
{
  cv::Point match_loc;
  cv::Mat old_img, out_img, diff_img, tpl_img;
  std::string merged_filename;

  // Compute difference between g_old_img and g_new_img
  imtools::diff(diff_img, g_old_img, g_new_img, g_mod_threshold);

  // Suppress noise
  imtools::threshold(diff_img, g_min_threshold, g_max_threshold);

  for (images_vector_t::iterator it = g_dst_images.begin(); it != g_dst_images.end(); ++it) {
    debug_log("target: %s\n", it->c_str());

    old_img = cv::imread(*it, 1);
    if (old_img.empty()) {
      fprintf(stderr, "Warning: skipping empty image: %s\n", it->c_str());
      continue;
    }

    imtools::bound_box_vector_t boxes;
    imtools::bound_boxes(boxes, diff_img);
    debug_log("boxes.size() = %ld\n", boxes.size());

    for (int i = 0; i < boxes.size(); i++) {
      // Get tpl_img from original image using boxes[i] as a mask
      tpl_img = cv::Mat(g_new_img, boxes[i]);

      debug_log("match_template(), boxes[%d]: %dx%d\n", i, boxes[i].x, boxes[i].y);
      imtools::match_template(match_loc, old_img, tpl_img);

#ifdef IMTOOLS_DEBUG
      std::ostringstream osstr;
      osstr << "rect" << i << ".jpg";
      std::string rect_filename = osstr.str();
      if (g_verbose) {
        std::cout << "* Writing to " << rect_filename << std::endl;
      }
      cv::imwrite(rect_filename, tpl_img);
#endif

      debug_log("patch() - %d\n", i);
      imtools::patch(out_img, old_img, tpl_img, match_loc.x, match_loc.y);

#ifdef IMTOOLS_DEBUG
      osstr.clear();
      osstr.str("");
      osstr << "step" << i << ".jpg";
      merged_filename = osstr.str();
      if (g_verbose) {
        std::cout << "* Writing to " << merged_filename << std::endl;
      }
      cv::imwrite(merged_filename, out_img);
#endif
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

      case 'd':
        {
          struct stat st;
          if (stat(optarg, &st)) {
            fprintf(stderr, "Error: invalid output directory, %s.\n", strerror(errno));
            usage(true);
          }
          if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "Error: %s is not a directory.\n", optarg);
            usage(true);
          }
          g_out_dir = optarg;
          break;
        }

      case 'm':
        save_int_opt_arg(g_mod_threshold, "Invalid modification threshold\n");
        break;

      case 'L':
        save_int_opt_arg(g_min_threshold, "Invalid min threshold\n");
        break;

      case 'H':
        save_int_opt_arg(g_max_threshold, "Invalid max threshold\n");
        break;

      case 'b':
        save_int_opt_arg(g_min_boxes_threshold, "Invalid min bound boxes threshold\n");
        break;

      case 'B':
        save_int_opt_arg(g_max_boxes_threshold, "Invalid max bound boxes threshold\n");
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

  try {
    load_images(argc, argv);

    // Merge the difference into the target images
    merge();

    // Overwrite old image with the new one
    debug_log("overwriting %s\n", g_old_image_filename.c_str());
    cv::imwrite(g_old_image_filename, g_new_img);
  } catch (cv::Exception& e) {
    fprintf(stderr, "CV error: %s\n", e.what());
    return 1;
  } catch (std::exception& e) {
    fprintf(stderr, "Error: %s\n", e.what());
    return 1;
  }

  return 0;
}

// vim: et ts=2 sts=2 sw=2
