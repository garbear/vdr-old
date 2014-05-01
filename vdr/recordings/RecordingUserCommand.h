#pragma once

#include "RecordingConfig.h"
#include <stddef.h>
#include <string>

namespace VDR
{
class cRecordingUserCommand
{
public:
  virtual ~cRecordingUserCommand(void) {}
  static cRecordingUserCommand& Get(void);

  void SetCommand(const std::string& strCommand);
  void InvokeCommand(const std::string& strState, const std::string& strRecordingFileName, const std::string& strSourceFileName = "");

private:
  cRecordingUserCommand(void) {}
  std::string m_strCommand;
};
}
