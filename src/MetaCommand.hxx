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
#ifndef IMTOOLS_META_COMMAND_HXX
#define IMTOOLS_META_COMMAND_HXX

#include <string>
#include "Command.hxx"

namespace imtools {

using imtools::Command;

/////////////////////////////////////////////////////////////////////
/// Model for `meta` server command
class MetaCommand : public Command
{
  public:
    enum class SubCommand : int {
      UNKNOWN,
      VERSION,
      COPYRIGHT,
      FEATURES,
      ALL
    };

    // Inherit ctors
    using Command::Command;
    explicit MetaCommand(SubCommand subcommand) : m_subcommand(subcommand) {};

    // Executes the command
    virtual void run(CommandResult& result) override;

    /// \returns command-specific data serialized in a string
    virtual std::string serialize() const noexcept;

    /// Returns numeric representation of subcommand name for comparisions.
    static SubCommand getSubCommandCode(const std::string& name) noexcept;

  protected:
    SubCommand m_subcommand;

  private:
    std::string _getName(const MetaCommand::SubCommand& code) const;
};

/////////////////////////////////////////////////////////////////////
class MetaCommandFactory : public ::imtools::CommandFactory
{
  public:
    enum class Option : int {
      UNKNOWN,
      SUBCOMMAND
    };

    using ::imtools::CommandFactory::CommandFactory;

    virtual MetaCommand* create(const Command::Arguments& elements) const override;

  protected:
    /// Returns numeric representation of option name for comparisions.
    virtual int getOptionCode(const std::string& o) const noexcept;
};

/////////////////////////////////////////////////////////////////////
} // namespace imtools
#endif // IMTOOLS_META_COMMAND_HXX
// vim: et ts=2 sts=2 sw=2
