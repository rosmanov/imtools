#ifndef IMTOOLS_IMMERGE_VARS_HXX
#define IMTOOLS_IMMERGE_VARS_HXX

using namespace imtools;
using std::string;


/// Max. number of target images
const int IMTOOLS_MAX_MERGE_TARGETS = 100;

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
bool g_strict = false;

// Matrices for old and new images
cv::Mat g_old_img;
cv::Mat g_new_img;

#ifdef IMTOOLS_THREADS
// Whether some thread function failed
// (we don't cancel threads in order to avoid possible memory leaks etc.)
bool g_thread_failure = false;

pthread_mutex_t g_work_mutex;
pthread_mutex_t g_process_images_mutex;
// Mutex for g_thread_failure variable
pthread_mutex_t g_thread_failure_mutex;
pthread_attr_t g_pta;
#endif

// Destination images
images_vector_t g_dst_images;
images_vector_t g_out_images;
images_vector_t g_generated_files;

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
" -v, --verbose              Turn on verbose output. Default: off.\n"
" -s, --strict               Turn on warnings into errors. Default: off.\n"
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
"%s -o old.png -n new.png -d/tmp old1.png old2.png\n";

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

#endif // IMTOOLS_IMMERGE_VARS_HXX
