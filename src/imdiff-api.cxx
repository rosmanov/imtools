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
#include "imdiff-api.hxx"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "imtools-meta.hxx"
#include "log.hxx"
#include "exceptions.hxx"
#include "imtools.hxx"

using imtools::imdiff::DiffCommand;
using imtools::imdiff::DiffCommandFactory;
using imtools::CommandResult;
using imtools::ErrorException;
using imtools::FileWriteErrorException;

/////////////////////////////////////////////////////////////////////

void
DiffCommand::run(CommandResult& result)
{
  cv::Mat diff_img;
  cv::Mat old_img{cv::imread(m_old_image_filename, 1)};
  cv::Mat new_img{cv::imread(m_new_image_filename, 1)};

  if (old_img.size() != new_img.size()) {
    throw ErrorException("Input images have different dimensions");
  }
  if (old_img.type() != new_img.type()) {
    throw ErrorException("Input images have different types");
  }

  debug_timer_init(t1, t2);
  debug_timer_start(t1);
  imtools::diff(diff_img, old_img, new_img);
  debug_timer_end(t1, t2, diff);

  debug_log("Writing to %s", m_out_image_filename.c_str());
  if (!cv::imwrite(m_out_image_filename, diff_img, getCompressionParams())) {
    throw FileWriteErrorException(m_out_image_filename);
  }

  result.setValue("OK");
}


std::string
DiffCommand::serialize() const noexcept
{
  std::stringstream ss;

  ss << m_old_image_filename << m_new_image_filename;

  return ss.str();
}


/////////////////////////////////////////////////////////////////////

int
DiffCommandFactory::getOptionCode(const std::string& o) const noexcept
{
  Option code;

  switch (o[0]) {
    case 'o':
      if (o == "old_image") {
        code = Option::OLD_IMAGE;
      } else if (o == "out_image") {
        code = Option::OUT_IMAGE;
      } else {
        code = Option::UNKNOWN;
      }
      break;
    case 'n':
      code = o == "new_image" ? Option::NEW_IMAGE : Option::UNKNOWN;
      break;
    default:
      code = Option::UNKNOWN;
      break;
  }

  return static_cast<int>(code);
}


DiffCommand*
DiffCommandFactory::create(const Command::Arguments& arguments) const
{
  std::string old_image_filename;
  std::string new_image_filename;
  std::string out_image_filename;

  for (auto& it : arguments) {
    std::string key = it.first.data();
    Command::CValuePtr value = it.second;

    std::string str_value = value->getString();
    Option option = static_cast<Option>(getOptionCode(key));

    verbose_log("key: %s, value: %s, option: %d", key.c_str(), str_value.c_str(), option);

    switch (option) {
      case Option::OLD_IMAGE: old_image_filename = str_value; break;
      case Option::NEW_IMAGE: new_image_filename = str_value; break;
      case Option::OUT_IMAGE: out_image_filename = str_value; break;
      default: warning_log("Skipping unknown key '%s'", key.c_str()); break;
    }
  }

  return new DiffCommand(old_image_filename, new_image_filename, out_image_filename);
}

// vim: et ts=2 sts=2 sw=2
