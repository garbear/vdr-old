#include "RecordingUserCommand.h"
#include "utils/Tools.h"

cRecordingUserCommand& cRecordingUserCommand::Get(void)
{
  static cRecordingUserCommand _instance;
  return _instance;
}

void cRecordingUserCommand::SetCommand(const std::string& strCommand)
{
  m_strCommand = strCommand;
}

void cRecordingUserCommand::InvokeCommand(const std::string& strState, const std::string& strRecordingFileName, const std::string& strSourceFileName /* = "" */)
{
  if (!m_strCommand.empty())
  {
    std::string cmd;
    if (!strSourceFileName.empty())
       cmd = *cString::sprintf("%s %s \"%s\" \"%s\"", m_strCommand.c_str(), strState.c_str(), *strescape(strState.c_str(), "\\\"$"), *strescape(strSourceFileName.c_str(), "\\\"$"));
    else
       cmd = *cString::sprintf("%s %s \"%s\"", m_strCommand.c_str(), strState.c_str(), *strescape(strState.c_str(), "\\\"$"));
    isyslog("executing '%s'", cmd.c_str());
    SystemExec(cmd.c_str());
  }
}
