#ifndef IMTOOLS_IMMERGE_VARS_HXX
#define IMTOOLS_IMMERGE_VARS_HXX

using std::string;

namespace imtools
{

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

int g_mod_threshold       = THRESHOLD_MOD;
int g_min_threshold       = THRESHOLD_MIN;
int g_max_threshold       = THRESHOLD_MAX;
int g_min_boxes_threshold = THRESHOLD_BOXES_MIN;
int g_max_boxes_threshold = THRESHOLD_BOXES_MAX;

string g_old_image_filename;
string g_new_image_filename;
string g_out_dir = ".";

// Whether non-option ARGV-elements are interpreted as `input-file output-file` pairs
bool g_pairs = false;

// Whether to turn warnings into fatal errors
int g_strict = 0;

// Matrices for old and new images
cv::Mat g_old_img;
cv::Mat g_new_img;


#ifdef IMTOOLS_THREADS
pthread_mutex_t g_work_mutex;
pthread_mutex_t g_process_images_mutex;
pthread_attr_t g_pta;
#endif // if threads

// Destination images
images_vector_t g_dst_images;
images_vector_t g_out_images;

std::vector<int> g_compression_params;

const char* usage_template = IMTOOLS_FULL_NAME "\n\n" IMTOOLS_COPYRIGHT "\n\n"
"A tool to compute difference between two images and apply the difference\n"
"to a number of similar images by means of the OpenCV library.\n"
"Usage: %s OPTIONS IMAGES\n\n"
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
"                            to increase strictness. Double value (-ss) will enable extra\n"
"                            structural similarity checks (MSSIM) for each patch. Default: off.\n"
" -n, --new-image            New image. Required.\n"
" -o, --old-image            Old image. Required.\n"
" -d, --out-dir              Output directory. Default: current directory.\n"
" -p, --pairs                Interpret IMAGES as a list of input and output file pairs.\n"
"                            If present, -d (--out-dir) has no effect.\n"
" -m, --mod-threshold        Modification threshold. Default: %d\n"
" -L, --min-threshold        Min. noise suppression threshold. Default: %d\n"
" -H, --max-threshold        Max. noise suppression threshold. Default: %d\n"
" -b, --boxes-min-threshold  Min. threshold for bound boxes. Default: %d\n"
" -B, --boxes-max-threshold  Max. threshold for bound boxes. Default: %d\n\n"
"EXAMPLE:\n"
"%s -o old.png -n new.png -p old1.png out1.png old2.png out2.png\n";

const char *g_short_options = "hvsn:o:d::pm::L::H::b::B::";

const struct option g_long_options[] = {
  {"help",                no_argument,       NULL, 'h'},
  {"verbose",             no_argument,       NULL, 'v'},
  {"strict",              no_argument,       NULL, 's'},
  {"new-image",           required_argument, NULL, 'n'},
  {"old-image",           required_argument, NULL, 'o'},
  {"out-dir",             optional_argument, NULL, 'd'},
  {"pairs",               no_argument,       NULL, 'p'},
  {"mod-threshold",       optional_argument, NULL, 'm'},
  {"min-threshold",       optional_argument, NULL, 'L'},
  {"max-threshold",       optional_argument, NULL, 'H'},
  {"boxes-min-threshold", optional_argument, NULL, 'b'},
  {"boxes-max-threshold", optional_argument, NULL, 'B'},
  {0,                     0,                 0,    0}
};

} // namespace imtools
#endif // IMTOOLS_IMMERGE_VARS_HXX
