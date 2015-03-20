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
#include <memory>

//#include "exceptions.hxx"
//using imtools::ErrorException;

namespace imtools {


class CommandResult;

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
      /// Retreives common meta information (ImTools version etc.)
      META,
      /// `imresize` command
      RESIZE,
      /// `immerge` command
      MERGE
    };

    /// Executes command.
    virtual void run(CommandResult& result) const = 0;

    /// \param o option name
    /// \returns numeric representation of option name for comparisions.
    virtual int getOptionCode(const std::string& o) const = 0;

    /// \returns command-specific data serialized in a string (for crypto digests)
    virtual std::string serialize() const noexcept = 0;

    /// \param c command name such as `immerge`
    /// \returns numeric representation of command name for comparisions.
    static Type getType(const std::string& c);

    static const compression_params_t getCompressionParams() noexcept
    {
      return s_compression_params;
    }

    virtual inline void setAllowAbsolutePaths(bool v) { m_allow_absolute_paths = v; }

  protected:
    /// Format-specific save parameters for `cv::imwrite()`.
    static compression_params_t s_compression_params;
    /// Whether to allow absolute path processing
    bool m_allow_absolute_paths;

};


/////////////////////////////////////////////////////////////////////
class CommandFactory
{
  public:
    virtual ~CommandFactory() {}
    virtual Command* create(const Command::element_vector_t& elements) const = 0;
};


/////////////////////////////////////////////////////////////////////
// Base class for final/intermediate result of a command
class CommandResult
{
  public:
    CommandResult() {}
    CommandResult(const CommandResult&) = delete;
    CommandResult& operator=(const CommandResult&) = delete;

    inline void setValue(const std::string& value) noexcept { m_value = value; }
    inline std::string getValue() const noexcept { return m_value; }

    operator bool() const noexcept { return !m_value.empty(); }
    operator std::string() const noexcept { return m_value; }

  protected:
    std::string m_value;
};

/////////////////////////////////////////////////////////////////////
} // namespace imtools
#endif // IMTOOLS_COMMAND_HXX
// vim: et ts=2 sts=2 sw=2
