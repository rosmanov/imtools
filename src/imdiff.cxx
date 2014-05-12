#include "imtools.hxx"


static const char* g_program_name;

static std::string g_out_filename;

static bool g_verbose = false;

// Minimum difference between old and new matrix elements specifying
// whether element is considered to be modified.
static const int g_modification_threshold = 42;

static const double g_thresh = 20.;
// g_max_thresh has effect with cv::THRESH_BINARY and cv::THRESH_BINARY_INV only.
static const double g_max_thresh = 250.;

static const char* usage_template = "Usage: %s OPTIONS old_image new_image\n\n"
"Computes difference between two images of the same size.\n\n"
" -h, --help           Display this help.\n"
" -v, --verbose        Verbose mode.\n"
" -o, --output         Filename of the output image. Required.\n";

static const char* g_short_options = "hvo:c";

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
diff(const std::string& filename_old, const std::string& filename_new)
{
  cv::Mat diff_img, old_img, new_img, src_gray, out;

  // Load images forcing them to be 3-channel
  // (for converting to grayscale)
  old_img = cv::imread(filename_old, 1);
  new_img = cv::imread(filename_new, 1);

  // Select likely modified pixels
  diff_img = (old_img - new_img > g_modification_threshold);

  // Create matrix of the same size and type as diff_img
  out.create(diff_img.size(), diff_img.type());

  // Convert to greyscale
  cv::cvtColor(diff_img, src_gray, CV_BGR2GRAY);

  // Reduce values lower than g_thresh with THRESH_TOZERO:
  // dst(x, y) = src(x, y) > g_thresh ? src(x, y) : 0
  //cv::threshold(src_gray, out, 0, 255, cv::THRESH_TOZERO);
  cv::threshold(src_gray, out, g_thresh, g_max_thresh, cv::THRESH_BINARY);

  // Reduce noise
  //cv::blur(out, out, cv::Size(3, 3));
  //cv::medianBlur(out, out, 3);
  //cv::GaussianBlur(out, out, cv::Size(3, 3), 10);

  if (g_verbose) {
    if (imtools::file_exists(g_out_filename.c_str())) {
      fprintf(stderr, "Warning: File %s will be overwritten\n", g_out_filename.c_str());
    }
    printf("* Writing to %s\n", g_out_filename.c_str());
  }

  cv::imwrite(g_out_filename, out);
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
