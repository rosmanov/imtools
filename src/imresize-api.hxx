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
#ifndef IMTOOLS_IMRESIZE_API_HXX
#define IMTOOLS_IMRESIZE_API_HXX

#include <string>
#include "imtools-types.hxx"
#include "Command.hxx"

namespace imtools { namespace imresize {

using imtools::Command;

/////////////////////////////////////////////////////////////////////
class ResizeCommand : public ::imtools::Command
{
  public:
    // Inherit ctors
    using Command::Command;

    ResizeCommand(const std::string& source,
        const std::string& output,
        uint_t width,
        uint_t height,
        double fx,
        double fy,
        int interpolation) noexcept;

    /// Executes the command
    virtual void run(imtools::CommandResult& result) override;

    /// \returns command-specific data serialized in a string
    virtual std::string serialize() const noexcept override;

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


/////////////////////////////////////////////////////////////////////
class ResizeCommandFactory : public ::imtools::CommandFactory
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

    using ::imtools::CommandFactory::CommandFactory;

    virtual ResizeCommand* create(const Command::Arguments& arguments) const override;

  protected:
    /// \returns numeric representation of option name for comparisions.
    virtual int getOptionCode(const std::string& o) const noexcept override;

  private:
    /*!
     * \param m Interpolation method name ("area", "linear", etc.)
     * \returns numeric representation of interpolation method name
     */
    static int _getInterpolationCode(const std::string& m) noexcept;
};

/////////////////////////////////////////////////////////////////////
}} // namespace imtools::imresize
#endif // IMTOOLS_IMRESIZE_API_HXX
// vim: et ts=2 sts=2 sw=2
