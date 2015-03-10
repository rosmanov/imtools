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
#ifndef IMTOOLS_IMRESIZE_API_HXX
#define IMTOOLS_IMRESIZE_API_HXX
#include "Command.hxx"

namespace imtools { namespace imresize {

using namespace imtools;
using imtools::Command;

class ResizeCommand : public ::imtools::Command
{
  public:
    enum class Option : int {
      UNKNOWN,
      SOURCE,
      OUTPUT,
      WIDTH,
      HEIGHT,
      INTERPOLATION,
      FX,
      FY
    };

    ResizeCommand(const std::string& source,
        const std::string& output,
        uint_t width = 0,
        uint_t height = 0,
        double fx = 0.,
        double fy = 0.,
        int interpolation = cv::INTER_LINEAR);

    ResizeCommand(const element_vector_t& elements);

    ResizeCommand() {}
    virtual ~ResizeCommand() {}
    ResizeCommand(ResizeCommand const&) {}

    // Executes the command
    virtual void run() const;

    /// Returns numeric representation of option name for comparisions.
    virtual int getOptionCode(const std::string& o) const;

  protected:
    /// Source image path
    std::string m_source;
    /// Output image path
    std::string m_output;
    /// Output image width
    uint_t m_width;
    /// Output image height
    uint_t m_height;
    /// Scale factor along the horizontal axis.
    double m_fx;
    /// Scale factor along the vertical axis.
    double m_fy;
    /// Interpolation method
    int m_interpolation;
};


class ResizeCommandFactory : public ::imtools::CommandFactory
{
  public:
    ResizeCommandFactory() {}
    ResizeCommandFactory(ResizeCommandFactory const&) {}
    virtual ~ResizeCommandFactory() {}

    virtual ResizeCommand* create(const Command::element_vector_t& elements) const
    {
      return new ResizeCommand(elements);
    }
};


}} // namespace imtools::imresize
#endif // IMTOOLS_IMRESIZE_API_HXX
// vim: et ts=2 sts=2 sw=2