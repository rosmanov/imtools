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
#ifndef IMTOOLS_DIFF_API_HXX
#define IMTOOLS_DIFF_API_HXX

#include <string>
#include "Command.hxx"

namespace imtools { namespace imdiff {

using imtools::Command;

/////////////////////////////////////////////////////////////////////
/// Model for `diff` server command
class DiffCommand : public Command
{
  public:
    // Inherit ctors
    using Command::Command;
    explicit DiffCommand(const std::string& old_image_filename,
        const std::string& new_image_filename,
        const std::string& out_image_filename) :
      m_old_image_filename{old_image_filename},
      m_new_image_filename{new_image_filename},
      m_out_image_filename{out_image_filename}
    {};

    // Executes the command
    virtual void run(CommandResult& result) override;

    /// \returns command-specific data serialized in a string
    virtual std::string serialize() const noexcept;

  protected:
    const std::string m_old_image_filename;
    const std::string m_new_image_filename;
    const std::string m_out_image_filename;
};

/////////////////////////////////////////////////////////////////////
class DiffCommandFactory : public ::imtools::CommandFactory
{
  public:
    enum class Option : int {
      UNKNOWN,
      OLD_IMAGE,
      NEW_IMAGE,
      OUT_IMAGE,
    };

    using ::imtools::CommandFactory::CommandFactory;

    virtual DiffCommand* create(const Command::Arguments& elements) const override;

  protected:
    /// Returns numeric representation of option name for comparisions.
    virtual int getOptionCode(const std::string& o) const noexcept;
};

/////////////////////////////////////////////////////////////////////
}} // namespace imtools::imdiff
#endif // IMTOOLS_DIFF_API_HXX
// vim: et ts=2 sts=2 sw=2
