
/* \file
 *
 * \copyright Copyright © 2014,2015  Ruslan Osmanov <rrosmanov@gmail.com>
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
#ifndef IMTOOLS_TYPES_HXX
#define IMTOOLS_TYPES_HXX

#include <iosfwd>
#include <vector>
#include "opencv-fwd.hxx"


namespace imtools {
/////////////////////////////////////////////////////////////////////
typedef unsigned int uint_t;
typedef cv::Rect BoundBox;
typedef std::vector<BoundBox> BoundBoxVector;
typedef std::vector<std::string> ImageArray;

enum Blur {
  BLUR_NONE   = 0,
  BLUR        = 1,
  BLUR_GAUSS  = 2,
  BLUR_MEDIAN = 3
};

enum Threshold {
  /// Default modification threshold (in percents).
  //THRESHOLD_MOD = 25,
  /// Default minimum noise suppression threshold.
  THRESHOLD_MIN = 20,
  /// Default maximum noise suppression threshold.
  THRESHOLD_MAX = 255
};

/////////////////////////////////////////////////////////////////////
} // namespace imtools
#endif // IMTOOLS_TYPES_HXX
// vim: et ts=2 sts=2 sw=2
