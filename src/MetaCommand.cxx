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
#include "MetaCommand.hxx"
#include "imtools-meta.hxx"
#include "log.hxx"
#include "exceptions.hxx"

using imtools::MetaCommand;
using imtools::MetaCommandFactory;
using imtools::CommandResult;
using imtools::ErrorException;

/////////////////////////////////////////////////////////////////////

void
MetaCommand::run(CommandResult& result)
{
  switch (m_subcommand) {
    case SubCommand::VERSION:
      result.setValue(IMTOOLS_FULL_NAME);
      break;
    case SubCommand::COPYRIGHT:
      result.setValue(IMTOOLS_COPYRIGHT);
      break;
    case SubCommand::FEATURES:
      result.setValue(IMTOOLS_FEATURES);
      break;
    case SubCommand::ALL:
      result.setValue(std::string(IMTOOLS_FULL_NAME "\n"
            IMTOOLS_COPYRIGHT "\n"
            IMTOOLS_FEATURES));
      break;
    case SubCommand::UNKNOWN:
    default:
      throw ErrorException("Unknown command %s", m_subcommand);
  }
}


std::string
MetaCommand::_getName(const MetaCommand::SubCommand& code) const
{
  switch (code) {
    case SubCommand::VERSION: return "version";
    case SubCommand::COPYRIGHT: return "copyright";
    case SubCommand::FEATURES: return "features";
    case SubCommand::ALL: return "all";
    case SubCommand::UNKNOWN:
    default:
      return "unknown";
  }
}


std::string
MetaCommand::serialize() const noexcept
{
  return _getName(m_subcommand);
}


MetaCommand::SubCommand
MetaCommand::getSubCommandCode(const std::string& name) noexcept
{
  SubCommand code;

  switch (name[0]) {
    case 'v':
      code = name == "version" ? SubCommand::VERSION : SubCommand::UNKNOWN;
      break;
    case 'f':
      code = name == "features" ? SubCommand::FEATURES : SubCommand::UNKNOWN;
      break;
    case 'c':
      code = name == "copyright" ? SubCommand::COPYRIGHT : SubCommand::UNKNOWN;
      break;
    case 'a':
      code = name == "all" ? SubCommand::ALL : SubCommand::UNKNOWN;
      break;
    default:
      code = SubCommand::UNKNOWN;
      break;
  }

  return code;
}


/////////////////////////////////////////////////////////////////////

int
MetaCommandFactory::getOptionCode(const std::string& o) const noexcept
{
  Option code;

  switch (o[0]) {
    case 's':
      code = o == "subcommand" ? Option::SUBCOMMAND : Option::UNKNOWN;
      break;
    default:
      code = Option::UNKNOWN;
      break;
  }

  return static_cast<int>(code);
}


MetaCommand*
MetaCommandFactory::create(const Command::Arguments& arguments) const
{
  MetaCommand::SubCommand subcommand = MetaCommand::SubCommand::UNKNOWN;

  for (auto& it : arguments) {
    std::string key = it.first.data();
    Command::CValuePtr value = it.second;

    std::string str_value = value->getString();
    Option option = static_cast<Option>(getOptionCode(key));

    verbose_log("key: %s, value: %s, option: %d\n", key.c_str(), str_value.c_str(), option);

    switch (option) {
      case Option::SUBCOMMAND: subcommand = MetaCommand::getSubCommandCode(str_value); break;
      default: warning_log("Skipping unknown key '%s'\n", key.c_str()); break;
    }
  }

  return new MetaCommand(subcommand);
}

// vim: et ts=2 sts=2 sw=2
