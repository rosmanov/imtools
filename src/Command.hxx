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
#ifndef IMTOOLS_COMMAND_HXX
#define IMTOOLS_COMMAND_HXX

#include <iosfwd>
#include <utility> // for std::pair
#include <vector>

namespace imtools {
/////////////////////////////////////////////////////////////////////
/// Base class for API command classes
class Command
{
  public:
    typedef std::pair<std::string, std::string> element_t;
    typedef std::vector<element_t> element_vector_t;
    typedef std::vector<int> compression_params_t;

    enum class Option : int;

    /// Command type
    enum class Type {
      UNKNOWN,
      RESIZE,
      MERGE
    };

    /// Executes command.
    virtual void run() const = 0;

    /// Returns numeric representation of option name for comparisions.
    virtual int getOptionCode(const std::string& o) const = 0;

    /// Returns numeric representation of command name for comparisions.
    static Type getType(const std::string& c);

    static const compression_params_t getCompressionParams()
    {
      return s_compression_params;
    }

  protected:
    /// Format-specific save parameters for `cv::imwrite()`.
    static compression_params_t s_compression_params;
};

/////////////////////////////////////////////////////////////////////
class CommandFactory
{
  public:
    virtual ~CommandFactory() {}
    virtual Command* create(const Command::element_vector_t& elements) const = 0;
};


/////////////////////////////////////////////////////////////////////
} // namespace imtools
#endif // IMTOOLS_COMMAND_HXX
// vim: et ts=2 sts=2 sw=2
