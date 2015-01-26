#ifndef IMTOOLS_IMMERGE_VARS_HXX
#define IMTOOLS_IMMERGE_VARS_HXX

namespace imtools { namespace immerge {
using namespace imtools;

#ifdef IMTOOLS_THREADS
bool g_thread_success = true;
unsigned g_max_threads = 4;
threads::it_lock_t g_thread_success_lock;
#endif


/// Max. number of target images
const int MAX_MERGE_TARGETS = 100;

/// Min. accepted value of the structural similarity coefficient in (double) strict mode.
/// See http://docs.opencv.org/doc/tutorials/highgui/video-input-psnr-ssim/video-input-psnr-ssim.html#image-similarity-psnr-and-ssim
const double MIN_MSSIM = 0.5;

/// Max. size of a bounding box relative to original image in percents
/// With this threshold we avoid applying really large patches, which probably
/// embrace a set of smaller bounding boxes (looks like OpenCV bug!)
/// Instead of using this threshold, we might calculate intersections of the
/// bounding boxes, then exclude those containing smaller boxes. However, we'll stick
/// with this simple approach for now.
const int MAX_BOUND_BOX_SIZE_REL = 70;

const char* g_program_name;

//int g_mod_threshold = THRESHOLD_MOD;
int g_min_threshold = THRESHOLD_MIN;
int g_max_threshold = THRESHOLD_MAX;

std::string g_old_image_filename;
std::string g_new_image_filename;
std::string g_out_dir = ".";

/// Whether non-option ARGV-elements are interpreted as `input-file output-file` pairs
bool g_pairs = false;

/// Whether to turn warnings into fatal errors
int g_strict = 0;

/// Matrix for the "old" image.
cv::Mat g_old_img;
/// Matrix for the "new" image.
cv::Mat g_new_img;
/// Matrix for a binary image representing differences between original (`g_old_img`) and
/// modified (`g_new_img`) images where modified spots have high values.
cv::Mat g_diff_img;

/// Input images.
images_vector_t g_input_images;
/// Output images.
images_vector_t g_out_images;

/// Format-specific save parameters for `cv::imwrite()`.
std::vector<int> g_compression_params;

/// Template for `printf`-like function.
const char* g_usage_template = IMTOOLS_FULL_NAME "\n\n" IMTOOLS_COPYRIGHT "\n\n"
"A tool to compute difference between two images and apply the difference\n"
"to a number of similar images by means of the OpenCV library.\n"
"Usage: %1$s OPTIONS IMAGES\n\n"
"Calculates difference between two images specified by --old-image and --new-image;\n"
"applies the difference to IMAGES.\n"
"The tool can be useful to update a logo or some common elements on a set of \"similar\" images.\n"
"Note: the bigger difference in quality the higher min. thresholds are required.\n\n"
"IMAGES:\n"
"Arguments specifying the target image paths.\n\n"
"OPTIONS:\n"
" -h, --help                 Display this help.\n"
" -v, --verbose              Turn on verbose output. Can be used multiple times\n"
"                            to increase verbosity (e.g. -vv). Default: off.\n"
" -s, --strict               Turn some warnings into fatal errors. Can be used multiple times\n"
"                            to increase strictness. Default: off.\n"
" -n, --new-image            New image. Required.\n"
" -o, --old-image            Old image. Required.\n"
" -d, --out-dir              Output directory. Default: current directory.\n"
" -p, --pairs                Interpret IMAGES as a list of input and output file pairs.\n"
"                            If present, -d (--out-dir) has no effect.\n"
#if 0
" -m, --mod-threshold        Modification threshold in %%. Default: %d.\n"
#endif
" -L, --min-threshold        Min. noise suppression threshold. Default: %2$d.\n"
" -H, --max-threshold        Max. noise suppression threshold. Default: %3$d.\n"
#ifdef IMTOOLS_THREADS
" -T, --max-threads          Max. number of concurrent threads. Default: %4$d.\n"
#endif
"\nEXAMPLES:\n\n"
"To apply changes between old.png and new.png to copies of old1.png and old2.png (out1.png and out2.png):\n"
"%1$s -o old.png -n new.png -p old1.png out1.png old2.png out2.png\n\n"
"To apply changes between old.png and new.png to old2.png (old2.png will be overwritten):\n"
"%1$s -o old.png -n new.png old2.png\n";

const char *g_short_options = "hvsn:o:d::pm::L::H::"
#ifdef IMTOOLS_THREADS
  "T::"
#endif
  ;

/// CLI arguments.
const struct option g_long_options[] = {
  {"help",          no_argument,       NULL, 'h'},
  {"verbose",       no_argument,       NULL, 'v'},
  {"strict",        no_argument,       NULL, 's'},
  {"new-image",     required_argument, NULL, 'n'},
  {"old-image",     required_argument, NULL, 'o'},
  {"out-dir",       optional_argument, NULL, 'd'},
  {"pairs",         no_argument,       NULL, 'p'},
#if 0
  {"mod-threshold", optional_argument, NULL, 'm'},
#endif
  {"min-threshold", optional_argument, NULL, 'L'},
  {"max-threshold", optional_argument, NULL, 'H'},
#ifdef IMTOOLS_THREADS
  {"max-threads",   optional_argument, NULL, 'T'},
#endif
  {0,               0,                 0,    0}
};

}} // namespace imtools::immerge
#endif // IMTOOLS_IMMERGE_VARS_HXX
