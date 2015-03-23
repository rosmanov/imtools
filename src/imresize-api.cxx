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

#include <cmath> // for isgreater()
#include <string>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "log.hxx"
#include "exceptions.hxx"

using imtools::imresize::ResizeCommand;
using imtools::imresize::ResizeCommandFactory;
using imtools::CommandResult;


ResizeCommand::ResizeCommand(const std::string& source,
    const std::string& output,
    uint_t width,
    uint_t height,
    double fx,
    double fy,
    int interpolation) noexcept
: m_source(source),
  m_output(output),
  m_width(width),
  m_height(height),
  m_fx(fx),
  m_fy(fy),
  m_interpolation(interpolation)
{
}

int
ResizeCommandFactory::_getInterpolationCode(const std::string& m) noexcept
{
  const int DEFAULT_CODE = cv::INTER_LINEAR;
  int code;

  switch (m[0]) {
    case 'n':
      code = m == "nearest" ? cv::INTER_NEAREST : DEFAULT_CODE;
      break;
    case 'l':
      if (m == "linear")
        code = cv::INTER_LINEAR;
      else if (m == "lanczos4")
        code = cv::INTER_LANCZOS4;
      else
        code = DEFAULT_CODE;
      break;
    case 'a':
      code = m == "area" ? cv::INTER_AREA : DEFAULT_CODE;
      break;
    case 'c':
      code = m == "cubic" ? cv::INTER_CUBIC : DEFAULT_CODE;
      break;
  }

  return code;
}


/// Returns numeric representation of option name for comparisions.
int
ResizeCommandFactory::getOptionCode(const std::string& o) const noexcept
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
ResizeCommand::run(CommandResult& result)
{
  std::string source_filename(trimPath(m_source));
  std::string output_filename(trimPath(m_output));

  cv::Mat source(cv::imread(source_filename, 1));
  if (source.empty()) {
    throw ErrorException("Source image '%s' doesn't exist", source_filename.c_str());
  }

  cv::Mat output;
  if ((m_width > 0 && m_height > 0)
      || (std::isgreater(m_fx, 0.0) && std::isgreater(m_fy, 0.0)))
  {
    debug_log("cv::resize(s, o, size(%d, %d), %f, %f, %i)",
        m_width, m_height, m_fx, m_fy, m_interpolation);
    cv::resize(source, output, cv::Size(m_width, m_height), m_fx, m_fy, m_interpolation);
  } else {
    throw ErrorException("Expected pairs of positive numbers: "
        "whether width/height, or fx/fy. None provided.");
  }

  if (!cv::imwrite(output_filename, output, getCompressionParams())) {
    throw FileWriteErrorException(output_filename);
  }

  result.setValue("OK");
}


std::string
ResizeCommand::serialize() const noexcept
{
    std::stringstream ss;

    ss << m_source << m_output
      << m_width << m_height
      << m_fx << m_fy;

    return ss.str();
}


ResizeCommand*
ResizeCommandFactory::create(const Command::Arguments& arguments) const
{
  std::string source;
  std::string output;
  uint_t      width         = 0;
  uint_t      height        = 0;
  double      fx            = 0.0;
  double      fy            = 0.0;
  int         interpolation = cv::INTER_LINEAR;

  for (auto& it : arguments) {
    std::string key = it.first.data();
    Command::CValuePtr value = it.second;

    Option option = static_cast<Option>(getOptionCode(key));
    auto str_value = value->getString();

    verbose_log("key: %s, value: %s, option: %d", key.c_str(), str_value.c_str(), option);

    switch (option) {
      case Option::SOURCE:        source        = str_value;                       break;
      case Option::OUTPUT:        output        = str_value;                       break;
      case Option::WIDTH:         width         = std::stoi(str_value);            break;
      case Option::HEIGHT:        height        = std::stoi(str_value);            break;
      case Option::FX:            fx            = std::stod(str_value);            break;
      case Option::FY:            fy            = std::stod(str_value);            break;
      case Option::INTERPOLATION: interpolation = _getInterpolationCode(str_value); break;
      default: warning_log("Skipping unknown key '%s'", key.c_str()); break;
    }
  }

  return new ResizeCommand(source, output, width, height, fx, fy, interpolation);
}


// vim: et ts=2 sts=2 sw=2
