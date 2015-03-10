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
#include "imresize-api.hxx"

using namespace imtools::imresize;

typedef ::imtools::Command::element_vector_t element_vector_t;
typedef ::imtools::Command::element_t element_t;


ResizeCommand::ResizeCommand(const std::string& source,
    const std::string& output,
    uint_t width,
    uint_t height,
    double fx,
    double fy,
    int interpolation)
: m_source(source),
  m_output(output),
  m_width(width),
  m_height(height),
  m_fx(fx),
  m_fy(fy),
  m_interpolation(interpolation)
{
}


ResizeCommand::ResizeCommand(const element_vector_t& elements)
{
  for (auto& it : elements) {
    std::string value = it.second.data();
    std::string key = it.first.data();
    Option option = static_cast<Option>(getOptionCode(key));

    verbose_log("key: %s, value: %s, option: %d\n", key.c_str(), value.c_str(), option);

    switch (option) {
      case Option::SOURCE:        m_source        = value;            break;
      case Option::OUTPUT:        m_output        = value;            break;
      case Option::WIDTH:         m_width         = std::stoi(value); break;
      case Option::HEIGHT:        m_height        = std::stoi(value); break;
      case Option::FX:            m_fx            = stod(value);      break;
      case Option::FY:            m_fy            = stod(value);      break;
      case Option::INTERPOLATION: m_interpolation = stoi(value);      break;
      default: warning_log("Skipping unknown key '%s'\n", key.c_str()); break;
    }
  }
}


/// Returns numeric representation of option name for comparisions.
int
ResizeCommand::getOptionCode(const std::string& o) const
{
  Option code;

  switch (o[0]) {
    case 's':
      code = o == "source" ? Option::SOURCE : Option::UNKNOWN;
      break;
    case 'o':
      code = o == "output" ? Option::OUTPUT : Option::UNKNOWN;
      break;
    case 'w':
      code = o == "width" ? Option::WIDTH : Option::UNKNOWN;
      break;
    case 'h':
      code = o == "height" ? Option::HEIGHT : Option::UNKNOWN;
      break;
    case 'i':
      code = o == "interpolation" ? Option::INTERPOLATION : Option::UNKNOWN;
      break;
    case 'f':
      code = o == "fx" ? Option::FX
        : (o == "fy" ? Option::FY : Option::UNKNOWN);
      break;
    default:
      code = Option::UNKNOWN;
      break;
  }

  return static_cast<int>(code);
}


void
ResizeCommand::run() const
{
  cv::Mat source;
  cv::Mat output;

  source = cv::imread(m_source, 1);
  if (source.empty()) {
    throw ErrorException("Source image '%s' is empty", m_source.c_str());
  }

  if (m_width > 0 && m_height > 0) {
    cv::resize(source, output, cv::Size(m_width, m_height), m_fx, m_fy, m_interpolation);
  } else if (m_fx > 0 && m_fy > 0) {
    cv::resize(source, output, cv::Size(), m_fx, m_fy, m_interpolation);
  } else {
    throw ErrorException("Expected positive number pairs of whether width/height, or fx/fy."
        "None provided. Nothing to do.");
  }

  if (!cv::imwrite(m_output, output, getCompressionParams())) {
    throw FileWriteErrorException(m_output);
  }
}

// vim: et ts=2 sts=2 sw=2
