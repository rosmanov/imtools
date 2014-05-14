#include "imtools.hxx"

static const char* g_program_name;

static bool g_verbose = false;

static std::string g_src_filename;

static std::string g_mask_filename;

static const char* usage_template = "Usage: %s OPTIONS <mask_filename>\n\n"
"Outlines essential changes with bounding boxes. <mask_filename> is an image generated with imdiff tool.\n\n"
"OPTIONS:\n"
" -h, --help           Display this help.\n"
" -v, --verbose        Verbose mode.\n"
" -s, --source         Source image which will be used to generate parts corresponding to the bounding boxes.\n\n";

static const char* g_short_options = "hvs:";

static const struct option g_long_options[] = {
  {"help",    no_argument,       NULL, 'h'},
  {"verbose", no_argument,       NULL, 'v'},
  {"source",  required_argument, NULL, 's'},
  {0,         0,                 0,    0}
};


static void
usage(bool is_error)
{
  fprintf(is_error ? stdout : stderr, usage_template, g_program_name);
  exit(is_error ? 1 : 0);
}


static cv::Mat g_mask, g_mask_gray, g_src;
static int g_thresh = 3;
static int g_max_thresh = 255;
static cv::RNG g_rng(12345);

void thresh_callback(int, void*);

int
main(int argc, char **argv)
{
  int next_option;

  g_program_name = argv[0];

  try {
    do {
      next_option = getopt_long(argc, argv, g_short_options, g_long_options, NULL);

      switch (next_option) {
        case 'h':
          usage(false);

        case 's':
          printf("step 1 %s\n", optarg);
          g_src_filename = optarg;
          printf("step 2\n");
          if (!imtools::file_exists(g_src_filename.c_str())) {
            std::cerr << "File " << g_src_filename << " doesn't exist" << std::endl;
            usage(true);
          }
          printf("step 3\n");
          g_src = cv::imread(g_src_filename);
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
          std::cerr << "Error: getopt returned character code 0" << std::oct << next_option << std::endl;
          usage(true);
      }
    } while (next_option != -1);

    if (optind < argc) {
      g_mask_filename = argv[optind++];

      if (!imtools::file_exists(g_mask_filename.c_str())) {
        std::cerr << "Error: image " << g_mask_filename << " doesn't exist." << std::endl;
        usage(true);
      }

      // Load image, force 3 channels
      g_mask = cv::imread(g_mask_filename, 1);
      if (g_mask.empty()) {
        std::cerr << "Error: image is empty" << std::endl;
        usage(true);
      }
    } else {
      std::cerr << "No mask image is specified.\n";
      usage(true);
    }


    // Convert image to gray and blur it
    cv::cvtColor(g_mask, g_mask_gray, CV_BGR2GRAY);
    cv::blur(g_mask_gray, g_mask_gray, cv::Size(15, 15));

    // Create window
    const char* window_title = "Source";
    cv::namedWindow(window_title, CV_WINDOW_AUTOSIZE);
    cv::imshow(window_title, g_mask);

    cv::createTrackbar(" Threshold:", "Source", &g_thresh, g_max_thresh, thresh_callback);
    thresh_callback(0, 0);

    cv::waitKey(0);
  } catch (cv::Exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}

void
thresh_callback(int, void*)
{
  cv::Mat threshold_output;
  std::vector<std::vector<cv::Point> > contours;
  std::vector<cv::Vec4i> hierarchy;

  // Detect edges using threshold
  cv::threshold(g_mask_gray, threshold_output, g_thresh, 255, cv::THRESH_BINARY);

  // Find contours
  cv::findContours(threshold_output, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

  // Approximate contours to polygons, get bounding rects and circles
  std::vector<std::vector<cv::Point> > contours_poly(contours.size());
  std::vector<cv::Rect> boundRect(contours.size());

  for (int i = 0; i < contours.size(); ++i) {
    cv::approxPolyDP(cv::Mat(contours[i]), contours_poly[i], 3, true);
    boundRect[i] = cv::boundingRect(cv::Mat(contours_poly[i]));
  }

  // Draw polygonal contour + bonding rects + circles
  cv::Mat drawing = cv::Mat::zeros(threshold_output.size(), CV_8UC3);
  int square;

  for (int i = 0; i < contours.size(); ++i) {
    square = boundRect[i].width * boundRect[i].height;
    printf("Rect: x: %d y: %d width: %d height: %d square: %d\n",
        boundRect[i].x, boundRect[i].y, boundRect[i].width, boundRect[i].height, square);
    if (square < 128) {
      printf("Skipping rect.\n");
      continue;
    }

    if (!g_src.empty()) {
      std::ostringstream osstr;

      osstr << "rect" << i << ".jpg";
      std::string filename = osstr.str();
      printf("* Writing to %s\n", filename.c_str());
      cv::imwrite(filename, cv::Mat(g_src, boundRect[i]));
    }

    cv::Scalar color = cv::Scalar(g_rng.uniform(0, 255), g_rng.uniform(0, 255), g_rng.uniform(0, 255));
    cv::drawContours(drawing, contours_poly, i, color, 1, 8, std::vector<cv::Vec4i>(), 0, cv::Point());
    cv::rectangle(drawing, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0);
  }

  // Show in a window
  cv::namedWindow("Contours", CV_WINDOW_AUTOSIZE);
  cv::imshow("Contours", drawing);
}

