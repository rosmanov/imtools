/* Copyright © 2014,2015 - Ruslan Osmanov <rrosmanov@gmail.com>
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
#pragma once
#ifndef IMTOOLS_IMMERGE_API_HXX
#define IMTOOLS_IMMERGE_API_HXX
#include <string>
#include <opencv2/highgui/highgui.hpp>
#include "imtools-types.hxx"
#include "Command.hxx"

namespace imtools { namespace immerge {

using imtools::Command;

/////////////////////////////////////////////////////////////////////
class MergeCommand : public ::imtools::Command
{
  public:
    // Inherit ctors
    using Command::Command;

    MergeCommand(
        const imtools::ImageArray& input_images,
        const imtools::ImageArray& out_images,
        const std::string& old_image_filename,
        const std::string& new_image_filename,
        int strict,
        int min_threshold,
        int max_threshold,
        unsigned max_threads_num) noexcept;

    /// Executes the command
    virtual void run(imtools::CommandResult& result) override;
    /// \returns command-specific data serialized in a string
    virtual std::string serialize() const noexcept override;

  public:
    /// Max. number of target images
    static const int MAX_MERGE_TARGETS = 100;

    /*! Min. accepted value of the structural similarity coefficient in (double) strict mode.
     * See http://docs.opencv.org/doc/tutorials/highgui/video-input-psnr-ssim/video-input-psnr-ssim.html#image-similarity-psnr-and-ssim
     */
    const double MIN_MSSIM = 0.5f;

    /*! Max. size of a bounding box relative to original image in percents
     * With this threshold we avoid applying really large patches, which probably
     * embrace a set of smaller bounding boxes (looks like OpenCV bug!)
     * Instead of using this threshold, we might calculate intersections of the
     * bounding boxes, then exclude those containing smaller boxes. However, we'll stick
     * with this simple approach for now.
     */
    static const int MAX_BOUND_BOX_SIZE_REL = 70;

  protected:
    imtools::ImageArray m_input_images;
    imtools::ImageArray m_out_images;
    std::string m_old_image_filename;
    std::string m_new_image_filename;
    /*! Turn some warnings into fatal errors. Can be used multiple times
     * to increase strictness.*/
    int m_strict = 0;
    int m_min_threshold;
    int m_max_threshold;

  private:
    bool _processImage(const std::string& in_filename, const std::string& out_filename);

    /*! Applies a patch using a bounding box.
     *
     * \param box Specifies patch area on a canvas of the size of `g_old_img` matrix (of size equal to the size of `g_new_img` matrix).
     * \param in_img Input image which will be patched.
     * \param out_img Output image previously cloned from `in_img`.
     * \param patched_boxes Patches which have been applied on `out_img`
     */
    bool _processPatch(const BoundBox& box, const cv::Mat& in_img, cv::Mat& out_img, BoundBoxVector& patched_boxes);

    /*! Checks if box is suspiciously large
     *
     * Sometimes OpenCV can produce bounding boxes embracing smaller boxes, or even
     * entire image! Looks like OpenCV bug. It shouldn't return nested rectangles!
     */
    static bool _isHugeBoundBox(const BoundBox& box, const cv::Mat& out_img);

  private:
    /// Matrix for the "old" image.
    cv::Mat m_old_img;
    /// Matrix for the "new" image.
    cv::Mat m_new_img;
    /// Matrix for a binary image representing differences between original (`g_old_img`) and
    /// modified (`g_new_img`) images where modified spots have high values.
    cv::Mat m_diff_img;
    /// Maximum number of parallel threads
    unsigned m_max_threads = 4;
};


/////////////////////////////////////////////////////////////////////
class MergeCommandFactory : public ::imtools::CommandFactory
{
  public:
    enum class Option : int {
      UNKNOWN,
      STRICT,
      NEW_IMAGE,
      OLD_IMAGE,
      MIN_THRESHOLD,
      MAX_THRESHOLD,
      INPUT_IMAGES,
      OUTPUT_IMAGES
    };

    using ::imtools::CommandFactory::CommandFactory;

    virtual MergeCommand* create(const Command::Arguments& elements) const override;

  protected:
    /*! \param o option name
     * \returns numeric representation of option name for comparisions. */
    virtual int getOptionCode(const std::string& o) const noexcept override;
};


/////////////////////////////////////////////////////////////////////
}} // namespace imtools:immerge
#endif // IMTOOLS_IMMERGE_API_HXX
// vim: et ts=2 sts=2 sw=2
