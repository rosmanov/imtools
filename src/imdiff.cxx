#include "imtools.hxx"


static const char* g_program_name;

static std::string g_out_filename;

static bool g_verbose = false;

// Minimum difference between old and new matrix elements specifying
// whether element is considered to be modified.
static int g_modification_threshold = 42;
static const int g_max_modification_threshold = 200;

static int g_thresh = 20;
// g_max_thresh has effect with cv::THRESH_BINARY and cv::THRESH_BINARY_INV only.
static const int g_max_thresh = 255;

static cv::Mat g_src_gray, g_out;
static cv::Mat diff_img, old_img, new_img;

const char *result_window_title = "Result";

const int IMTOOLS_BLUR_NONE = 0;
const int IMTOOLS_BLUR = 1;
const int IMTOOLS_BLUR_GAUSS = 2;
const int IMTOOLS_BLUR_MEDIAN = 3;

static const char* usage_template = "Usage: %s OPTIONS old_image new_image\n\n"
"Computes difference between two images of the same size.\n\n"
" -h, --help           Display this help.\n"
" -v, --verbose        Verbose mode.\n"
" -o, --output         Filename of the output image. Required.\n";

static const char* g_short_options = "hvo";

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
  // Select likely modified pixels.
  // We try to something similar to command:
  // compare old.jpg new.jpg -fuzz 25%  -compose Src -highlight-color White -lowlight-color Black diff.jpg
  diff_img = (old_img - new_img > g_modification_threshold);

  // Create matrix of the same size and type as diff_img
  g_out.create(diff_img.size(), diff_img.type());

  // Convert to greyscale
  cv::cvtColor(diff_img, g_src_gray, CV_BGR2GRAY);

  // Show in a window
  cv::namedWindow(result_window_title, CV_WINDOW_AUTOSIZE);
  cv::imshow(result_window_title, g_out);
}

static void
thresh_callback(int, void*)
{
  modification_thresh_callback(0, 0);

  // Reduce values lower than g_thresh with THRESH_TOZERO:
  // dst(x, y) = src(x, y) > g_thresh ? src(x, y) : 0
  //cv::threshold(g_src_gray, g_out, 0, 255, cv::THRESH_TOZERO);
  cv::threshold(g_src_gray, g_out, g_thresh, g_max_thresh, cv::THRESH_BINARY);


  // Show in a window
  cv::namedWindow(result_window_title, CV_WINDOW_AUTOSIZE);
  cv::imshow(result_window_title, g_out);
}

static void
blur_callback(int state, void *user_data)
{
  if (state != 1) {
    return;
  }

  thresh_callback(0, 0);

  int type = *((int*)user_data);

  switch (type) {
    case IMTOOLS_BLUR:
      cv::blur(g_out, g_out, cv::Size(3, 3));
      break;

    case IMTOOLS_BLUR_GAUSS:
      cv::GaussianBlur(g_out, g_out, cv::Size(3, 3), 10);
      break;

    case IMTOOLS_BLUR_MEDIAN:
      cv::medianBlur(g_out, g_out, 3);
      break;

    default:
      break;
  }
}

static void
diff(const std::string& filename_old, const std::string& filename_new)
{
  // Load images forcing them to be 3-channel
  // (for converting to grayscale)
  old_img = cv::imread(filename_old, 1);
  new_img = cv::imread(filename_new, 1);

  // Create window
  const char* window_title = "Source";
  cv::namedWindow(window_title, CV_WINDOW_AUTOSIZE);
  cv::imshow(window_title, new_img);

  cv::createButton("No blur", blur_callback, (void *) &IMTOOLS_BLUR_NONE, CV_RADIOBOX);
  cv::createButton("Blur", blur_callback, (void *) &IMTOOLS_BLUR, CV_RADIOBOX);
  cv::createButton("Gauss blur", blur_callback, (void *) &IMTOOLS_BLUR_GAUSS, CV_RADIOBOX);
  cv::createTrackbar(" Mod Thresh:", window_title, &g_modification_threshold, g_max_modification_threshold, modification_thresh_callback);
  cv::createTrackbar(" Threshold:", window_title, &g_thresh, g_max_thresh, thresh_callback);
  thresh_callback(0, 0);

  cv::waitKey(0);

#if 0
  if (g_verbose) {
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
#endif
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
