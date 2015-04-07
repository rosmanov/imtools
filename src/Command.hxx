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

#include <cassert>
#include <iosfwd>
#include <utility> // for std::pair
#include <vector>
#include <memory>
#include <boost/algorithm/string/trim.hpp>
#include "imtools-types.hxx"


namespace imtools {

class CommandResult;

/////////////////////////////////////////////////////////////////////
/// Base class for API command classes
class Command
{
  public:
    class ArrayValue;
    class StringValue;

    ///////////////////////////////////////////////////////////////
    /// Base type for command option values
    class Value
    {
      public:
        enum class Type : int
        {
          UNDEFINED,
          STRING,
          ARRAY
        };
        typedef imtools::ImageArray ArrayType;

        inline Type getType() const noexcept { return m_type; }

#if 0
        // XXX use getArray() instead
        virtual inline operator ArrayType() const noexcept
        {
          assert(m_type == Type::ARRAY);
          auto p = dynamic_cast<ArrayValue const*>(this);
          return p ? static_cast<ArrayType>(*p) : ArrayType();
        }

        // XXX Use getString() method instead
        virtual inline operator std::string() const noexcept
        {
          assert(m_type == Type::STRING);
          auto p = dynamic_cast<StringValue const*>(this);
          return p ? static_cast<const std::string>(*p) : std::string();
        }
#endif

        virtual inline std::string getString() const noexcept
        {
          assert(m_type == Type::STRING);
          auto p = dynamic_cast<StringValue const*>(this);
          //return p ? static_cast<const std::string>(*p) : std::string();
          return p ? p->get() : std::string();
        }

        virtual inline ArrayType getArray() const noexcept
        {
          assert(m_type == Type::ARRAY);
          auto p = dynamic_cast<ArrayValue const*>(this);
          //return p ? static_cast<ArrayType>(*p) : ArrayType();
          return p ? p->get() : ArrayType();
        }

      protected:
        Value() : m_type(Type::UNDEFINED) {}

        Type m_type;
    };


    ///////////////////////////////////////////////////////////////
    /// String option value
    class StringValue : public Value
    {
      public:
        StringValue() = delete;
        StringValue(const std::string& value)
          : m_value(value) { m_type = Value::Type::STRING; }
        virtual ~StringValue() {}
#if 0
        virtual explicit inline operator std::string() const noexcept override { return m_value; }
#endif
        virtual inline std::string get() const noexcept { return m_value; }

      protected:
        std::string m_value;
    };


    ///////////////////////////////////////////////////////////////
    /// Array option value
    class ArrayValue : public Value
    {
      public:
        ArrayValue() = delete;
        ArrayValue(const std::vector<std::string>& value)
          : m_value(value) { m_type = Value::Type::ARRAY; }
        virtual ~ArrayValue() {}
#if 0
        virtual explicit inline operator Value::ArrayType() const noexcept override { return m_value; }
#endif
        virtual inline Value::ArrayType get() const noexcept { return m_value; }

      protected:
        Value::ArrayType m_value;
    };


    ///////////////////////////////////////////////////////////////
    // Types
    typedef const std::shared_ptr<const Value> CValuePtr;
    typedef std::pair<std::string, CValuePtr> ArgumentItem;
    typedef std::vector<ArgumentItem> Arguments;
    typedef std::vector<int> CompressionParams;

    /// Command type
    enum class Type
    {
      UNKNOWN,
      /// Retreives common meta information (ImTools version etc.)
      META,
      /// `imresize` command
      RESIZE,
      /// `immerge` command
      MERGE
    };

    ///////////////////////////////////////////////////////////////
    Command() : m_allow_absolute_paths(true) {}
    Command(const Command&) = delete;
    Command(const Command&&) = delete;

    /// Executes command.
    virtual void run(CommandResult& result) = 0;

    /// \returns command-specific data serialized in a string (for crypto digests)
    virtual std::string serialize() const noexcept = 0;

    /// \param c command name such as `immerge`
    /// \returns numeric representation of command name for comparisions.
    static Type getType(const std::string& c);

    static inline const CompressionParams getCompressionParams() noexcept
    {
      return s_compression_params;
    }

    virtual inline void setAllowAbsolutePaths(bool v) noexcept { m_allow_absolute_paths = v; }

    virtual inline std::string trimPath(const std::string& path) noexcept
    {
      return m_allow_absolute_paths
        ? path
        : std::string(boost::algorithm::trim_left_copy_if(path, boost::algorithm::is_any_of(PATH_DELIMS)));
    }

  protected:
    /// Format-specific save parameters for `cv::imwrite()`.
    static CompressionParams s_compression_params;
    /// Whether to allow absolute path processing
    bool m_allow_absolute_paths = true;
    const char* PATH_DELIMS = " \t\r\n/";
};


/////////////////////////////////////////////////////////////////////
class CommandFactory
{
  public:
    enum class Option : int;

    virtual ~CommandFactory() {}
    virtual Command* create(const Command::Arguments& elements) const = 0;

  protected:
    /// \param o option name
    /// \returns numeric representation of option name for comparisions.
    virtual int getOptionCode(const std::string& o) const = 0;
};


/////////////////////////////////////////////////////////////////////
// Base class for final/intermediate command result
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
