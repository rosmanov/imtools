/**
 * \file A tool to compute difference between two images and apply the difference
 * to a number of similar images by means of the OpenCV library.
 *
 * \copyright Copyright © 2014,2015 Ruslan Osmanov <rrosmanov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include "immerge-api.hxx"

#include <string>
#include <boost/algorithm/string/trim.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "log.hxx"
#include "exceptions.hxx"
#include "imtools.hxx"

using imtools::immerge::MergeCommand;
using imtools::immerge::MergeCommandFactory;
using imtools::CommandResult;
using imtools::ErrorException;
using imtools::FileWriteErrorException;

typedef ::imtools::Command::Arguments Arguments;
typedef ::imtools::Command::ArgumentItem ArgumentItem;
typedef ::imtools::BoundBoxVector BoundBoxVector;
typedef ::imtools::BoundBox BoundBox;

/////////////////////////////////////////////////////////////////////

MergeCommand::MergeCommand(
    const imtools::ImageArray& input_images,
    const imtools::ImageArray& out_images,
    const std::string& old_image_filename,
    const std::string& new_image_filename,
    const std::string& out_dir,
    int strict,
    int min_threshold,
    int max_threshold,
    unsigned max_threads_num) noexcept
: m_input_images(input_images),
  m_out_images(out_images),
  m_old_image_filename(old_image_filename),
  m_new_image_filename(new_image_filename),
  m_out_dir(out_dir),
  m_strict(strict),
  m_min_threshold(min_threshold),
  m_max_threshold(max_threshold),
  m_max_threads(max_threads_num)
{
}


bool
MergeCommand::_processPatch(const BoundBox& box, const cv::Mat& in_img, cv::Mat& out_img, BoundBoxVector& patched_boxes)
{
  bool      success       = true;
  cv::Mat   old_tpl_img;
  cv::Mat   new_tpl_img;
  cv::Point match_loc;
  cv::Point match_loc_new;
  cv::Rect  roi;
  cv::Rect  roi_new;
  double    avg_mssim;
  double    avg_mssim_new;

  BoundBox homo_box(box);

  debug_log("%s: %dx%d @ %d;%d\n", __func__, box.width, box.height, box.x, box.y);

  try {
    // The more the box area is heterogeneous on m_old_img, the more chances
    // to match this location on the image being patched.
    // However, imtools::make_heterogeneous() *modifies* the box! So we'll have to restore its size
    // before patching.
    imtools::make_heterogeneous(/**box*/homo_box, m_old_img);

    old_tpl_img = cv::Mat(m_old_img, homo_box);
    new_tpl_img = cv::Mat(m_new_img, box);

#if 0 && defined(IMTOOLS_DEBUG)
    {
      std::ostringstream osstr;
      osstr << "old_tpl_img_" << box.x << "@" << box.y << "_" << box.width << "x" << box.height << ".jpg";
      std::string fn = osstr.str();
      debug_log("* Writing to %s\n", fn.c_str());
      cv::imwrite(fn, old_tpl_img);
    }
    {
      std::ostringstream osstr;
      osstr << "new_tpl_img_" << homo_box.x << "@" << homo_box.y << "_" << homo_box.width << "x" << homo_box.height << ".jpg";
      std::string fn = osstr.str();
      debug_log("* Writing to %s\n", fn.c_str());
      cv::imwrite(fn, new_tpl_img);
    }
#endif

    // Find likely location of an area similar to old_tpl_img on the image being processed now.
    imtools::match_template(match_loc, in_img, old_tpl_img);
    // Some patches may already be applied. We'll try to detect if it's so.
    imtools::match_template(match_loc_new, in_img, new_tpl_img);

    // Assign regions of interest for old and new images. Then we'll see which of them
    // matches best.

    if (homo_box != box) {
      // The box has been modified by imtools::make_heterogeneous()
      assert(box.x >= homo_box.x && box.y >= homo_box.y);

      roi = cv::Rect(match_loc.x + (box.x - homo_box.x), match_loc.y + (box.y - homo_box.y), box.width, box.height);
      debug_log("homo_box != *box, roi = (%d, %d, %d, %d)\n", roi.x, roi.y, roi.width, roi.height);
    } else {
      roi = cv::Rect(match_loc.x, match_loc.y, old_tpl_img.cols, old_tpl_img.rows);
      debug_log("homo_box == *box, roi = (%d, %d, %d, %d)\n", roi.x, roi.y, roi.width, roi.height);
    }
    roi_new = cv::Rect(match_loc_new.x, match_loc_new.y, new_tpl_img.cols, new_tpl_img.rows);
    debug_log("roi_new = (%d, %d, %d, %d)\n", roi_new.x, roi_new.y, roi_new.width, roi_new.height);

    // Calculate average similarity

    avg_mssim     = imtools::get_avg_MSSIM(new_tpl_img, cv::Mat(out_img, roi));
    avg_mssim_new = imtools::get_avg_MSSIM(new_tpl_img, cv::Mat(out_img, roi_new));
    debug_log("avg_mssim: %f avg_mssim_new: %f\n", avg_mssim, avg_mssim_new);

#if 0
    // Check if a patched box intersects roi. If it is not, then we ought to
    // skip the checks for avg_mssim_new. There is a number of functions computing intersection of different contours, e.g.:
    // - pointPolygonTest() checks if a point is within a polygon.
    // - cv::intersectConvexConvex() returns intersection area of two convex polygons (opencv2/imgproc/imgproc.hpp)
    // However, we'll stick with a simple loop here.
    bool b_intersection = false;
    for (BoundBoxVector::iterator it = patched_boxes.begin(); it != patched_boxes.end(); ++it) {
      // The `&` operator returns intersection of two rectangles.
      cv::Rect r = (*it) & roi;
      if (r.width) {
        debug_log0("Got intersection\n");
        b_intersection = true;
        break;
      }
    }
    if (!b_intersection || avg_mssim > avg_mssim_new /*|| avg_mssim < 0*/) {
#endif
    if (avg_mssim > avg_mssim_new /*|| avg_mssim < 0*/) {
      imtools::patch(out_img, new_tpl_img, roi);
      patched_boxes.push_back(cv::Rect(roi));
      debug_log("<<<<<<<< Using roi: avg_mssim: %f avg_mssim_new: %f ratio: %f\n",
          avg_mssim, avg_mssim_new, (avg_mssim_new / avg_mssim));
    } else {
      debug_log("<<<<<<<< Using roi_new: avg_mssim: %f avg_mssim_new: %f ratio: %f\n",
          avg_mssim, avg_mssim_new, (avg_mssim_new / avg_mssim));
      imtools::patch(out_img, new_tpl_img, roi_new);
      patched_boxes.push_back(cv::Rect(roi_new));
    }
#if 0 && defined(IMTOOLS_DEBUG)
    {
      std::ostringstream osstr;
      osstr << "patched_" << box.x << "@" << box.y << "_" << box.width << "x" << box.height << ".jpg";
      std::string filename = osstr.str();
      debug_log("* Writing to %s\n", filename.c_str());
      cv::imwrite(filename, out_img);
    }
#endif
  } catch (ErrorException& e) {
    success = false;
    imtools::log::push_error(e.what());
  } catch (...) {
    success = false;
    imtools::log::push_error("Caught unknown exception!\n");
  }

  return (success);
}


bool
MergeCommand::_isHugeBoundBox(const BoundBox& box, const cv::Mat& out_img)
{
  double box_rel_size = box.area() * 100 / out_img.size().area();
  bool result = (box_rel_size > MAX_BOUND_BOX_SIZE_REL);
  if (result) {
    warning_log("Bounding box is too large: %dx%d (%f%%)\n",
        box.width, box.height, box_rel_size);
  }
  return (result);
}


bool
MergeCommand::_processImage(const std::string& in_filename, const std::string& out_filename)
{
  bool           success          = true;
  uint_t         n_boxes;
  cv::Mat        in_img;
  cv::Mat        out_img;
  std::string    merged_filename;
  BoundBoxVector boxes;
  BoundBoxVector patched_boxes;

  // Load target image forcing 3 channels
  verbose_log2("Processing target: %s\n", in_filename.c_str());
  in_img = cv::imread(in_filename, 1);
  if (in_img.empty()) {
    throw ErrorException("empty image skipped: " + in_filename);
  }

  // Prepare matrix for the output image
  out_img = in_img.clone();

  // Generate rectangles bounding clusters of white (changed) pixels on `m_diff_img`
  imtools::bound_boxes(boxes, m_diff_img, m_min_threshold, m_max_threshold);
  n_boxes = boxes.size();
  patched_boxes.reserve(n_boxes);

  // Process the patches
  for (uint_t i = 0; i < n_boxes; i++) {
    debug_log("box[%d]: %dx%d @ %d;%d\n", i, boxes[i].width, boxes[i].height, boxes[i].x, boxes[i].y);

    if (_isHugeBoundBox(boxes[i], out_img)) {
      continue;
    }

    if (!_processPatch(boxes[i], in_img, out_img, patched_boxes)) {
      success = false;
      break;
    }
  }

  if (!success) {
    imtools::log::warn_all();
    error_log("%s: failed to process, skipping\n", in_filename.c_str());
    return (success);
  }

  // Save merged matrix to filesystem
  if (m_strict && out_filename == in_filename && imtools::file_exists(out_filename)) {
    throw ErrorException("strict mode prohibits writing to existing file " + out_filename);
  }
  verbose_log2("Writing to %s\n", out_filename.c_str());
  if (!cv::imwrite(out_filename, out_img, getCompressionParams())) {
    throw FileWriteErrorException(out_filename);
  }
  verbose_log("[Output] file:%s boxes:%d\n", out_filename.c_str(), n_boxes);

  return (success);
}


void
MergeCommand::run(imtools::CommandResult& result)
{
  bool      success     = true;
  uint_t    n_images    = m_input_images.size();
  uint_t    i;
  cv::Point match_loc;
  cv::Mat   out_img;

  // Load the two images which will specify the modificatoin to be applied to
  // each of m_input_images; force 3 channels
  m_old_img = cv::imread(m_old_image_filename, 1);
  m_new_img = cv::imread(m_new_image_filename, 1);
  if (m_old_img.size() != m_new_img.size()) {
    throw ErrorException("Input images have different dimensions");
  }
  if (m_old_img.type() != m_new_img.type()) {
    throw ErrorException("Input images have different types");
  }

  // Compute difference between `m_old_img` and `m_new_img`
  debug_timer_init(t1, t2);
  debug_timer_start(t1);
  imtools::diff(m_diff_img, m_old_img, m_new_img);
  debug_timer_end(t1, t2, diff);

  if (m_out_images.size() != m_input_images.size()) {
    throw ErrorException("Number of input doesn't match number of output images");
  }


#ifdef IMTOOLS_THREADS
  uint_t num_threads = m_input_images.size() >= m_max_threads ? m_max_threads : m_input_images.size();

  assert(m_out_images.size() == m_input_images.size());

  IT_INIT_OPENMP(num_threads);

  bool r;
  // We'll use `&=` instead of `=` because of restrictions of `omp atomic`.
  // See http://www-01.ibm.com/support/knowledgecenter/SSXVZZ_8.0.0/com.ibm.xlcpp8l.doc/compiler/ref/ruompatm.htm%23RUOMPATM
  _Pragma("omp parallel for private(r)")
  for (i = 0; i < n_images; ++i) {
    try {
      r = _processImage(trimPath(m_input_images[i]), trimPath(m_out_images[i]));
      if (!r) {
        _Pragma("omp atomic")
        success &= false;
      }
    } catch (ErrorException& e) {
      warning_log("%s\n", e.what());
      _Pragma("omp atomic")
      success &= false;
    }
  }

#else // no threads
  for (i = 0; i < n_images; ++i) {
    try {
      if (!_processImage(trimPath(m_input_images[i]), trimPath(m_out_images[i])))
        success = false;
    } catch (ErrorException& e) {
      warning_log("%s\n", e.what());
      success = false;
    }
  }
#endif // IMTOOLS_THREADS

  debug_timer_end(t1, t2, run);

  if (success) {
    result.setValue("OK");
  }
}


std::string
MergeCommand::serialize() const noexcept
{
  std::stringstream ss;

  ss << m_old_image_filename
    << m_new_image_filename
    << m_out_dir
    //<< m_input_images.size()
    << m_out_images.size()
    << m_strict;

  return ss.str();
}


/////////////////////////////////////////////////////////////////////

int
MergeCommandFactory::getOptionCode(const std::string& o) const noexcept
{
  Option code;

  switch (o[0]) {
    case 's':
      if (o == "strict") {
        code = Option::STRICT;
      } else {
        code = Option::UNKNOWN;
      }
      break;
    case 'i':
      code = o == "input_images" ? Option::INPUT_IMAGES : Option::UNKNOWN;
      break;
    case 'o':
      if (o == "old_image") {
        code = Option::OLD_IMAGE;
      }  else if (o == "out_dir") {
        code = Option::OUT_DIR;
      }  else if (o == "output_images") {
        code = Option::OUTPUT_IMAGES;
      } else {
        code = Option::UNKNOWN;
      }
      break;
    case 'n':
      code = o == "new_image" ? Option::NEW_IMAGE : Option::UNKNOWN;
      break;
    case 'm':
      if (o == "min_threshold") {
        code = Option::MIN_THRESHOLD;
      }  else if (o == "max_threshold") {
        code = Option::MAX_THRESHOLD;
      } else {
        code = Option::UNKNOWN;
      }
      break;
    default:
      code = Option::UNKNOWN;
      break;
  }

  return static_cast<int>(code);
}


MergeCommand*
MergeCommandFactory::create(const Command::Arguments& arguments) const
{
  imtools::ImageArray input_images;
  imtools::ImageArray out_images;
  std::string         old_image_filename;
  std::string         new_image_filename;
  std::string         out_dir{"."};
  int                 strict              = 0;
  int                 min_threshold       = imtools::Threshold::THRESHOLD_MIN;
  int                 max_threshold       = imtools::Threshold::THRESHOLD_MAX;
  unsigned            max_threads_num     = imtools::threads::max_threads();

  for (auto& it : arguments) {
    std::string key = it.first.data();
    Option option = static_cast<Option>(getOptionCode(key));
    Command::CValuePtr value = it.second;

    verbose_log("key: %s, value: %s, option: %d\n",
        key.c_str(), (value->getType() == Command::Value::Type::STRING ? value->getString().c_str() : "[]"), option);

    switch (option) {
      case Option::INPUT_IMAGES:  input_images       = value->getArray();                                break;
      case Option::OUTPUT_IMAGES: out_images         = value->getArray();                                break;
      case Option::OLD_IMAGE:     old_image_filename = value->getString();                               break;
      case Option::NEW_IMAGE:     new_image_filename = value->getString();                               break;
      case Option::OUT_DIR:       out_dir            = value->getString();                               break;
      case Option::STRICT:        strict             = std::stoi(value->getString());                    break;
      case Option::MIN_THRESHOLD: min_threshold      = std::stoi(value->getString());                    break;
      case Option::MAX_THRESHOLD: max_threshold      = std::stoi(value->getString());                    break;
      case Option::UNKNOWN:
      default: warning_log("Skipping unknown key '%s'\n", key.c_str()); break;
    }
  }

  if (input_images.empty()) {
    input_images = out_images;
  } else if (out_images.empty()) {
    out_images = input_images;
  } else if (input_images.size() != out_images.size()) {
    throw ErrorException("Sizes of input and output images are not equal");
  }

#ifdef IMTOOLS_DEBUG
  printf("input_images: "); for (auto& it : input_images) { printf("%s ", it.c_str()); } printf("\n");
#endif

  return new MergeCommand(
      input_images,
      out_images,
      old_image_filename,
      new_image_filename,
      out_dir,
      strict,
      min_threshold,
      max_threshold,
      max_threads_num);
}

// vim: et ts=2 sts=2 sw=2
