#pragma once

#include "RecordingConfig.h"
#include <stddef.h>

class cRecordingUserCommand {
private:
  static const char *command;
public:
  static void SetCommand(const char *Command) { command = Command; }
  static void InvokeCommand(const char *State, const char *RecordingFileName, const char *SourceFileName = NULL);
  };

