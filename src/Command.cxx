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

#include <string>
#include <utility> // for std::initializer_list()
#include <opencv2/highgui/highgui.hpp>

#include "imtools-types.hxx"
#include "Command.hxx"

using imtools::Command;

/////////////////////////////////////////////////////////////////////

Command::CompressionParams Command::s_compression_params(std::initializer_list<int> {
    CV_IMWRITE_PNG_STRATEGY, cv::IMWRITE_PNG_STRATEGY_FILTERED,
    CV_IMWRITE_PNG_COMPRESSION, 9, // 0 - none, 9 - full
    CV_IMWRITE_JPEG_QUALITY, 90});

/////////////////////////////////////////////////////////////////////

Command::Type
Command::getType(const std::string& c)
{
  Command::Type code;

  switch (c[0]) {
    case 'r':
      code = c == "resize" ? Command::Type::RESIZE : Command::Type::UNKNOWN;
      break;
    case 'm':
      if (c == "meta") {
        code = Command::Type::META;
      } else if (c == "merge") {
        code = Command::Type::MERGE;
      } else {
        code = Command::Type::UNKNOWN;
      }
      break;
#ifdef IMTOOLS_EXTRA
    case 'd':
      code = c == "diff" ? Command::Type::DIFF : Command::Type::UNKNOWN;
      break;
#endif
    default:
      code = Command::Type::UNKNOWN;
      break;
  }

  return code;
}

// vim: et ts=2 sts=2 sw=2
