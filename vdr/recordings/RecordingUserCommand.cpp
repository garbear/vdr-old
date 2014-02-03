#include "RecordingUserCommand.h"
#include "utils/Tools.h"

const char *cRecordingUserCommand::command = NULL;

void cRecordingUserCommand::InvokeCommand(const char *State, const char *RecordingFileName, const char *SourceFileName)
{
  if (command) {
    cString cmd;
    if (SourceFileName)
       cmd = cString::sprintf("%s %s \"%s\" \"%s\"", command, State, *strescape(RecordingFileName, "\\\"$"), *strescape(SourceFileName, "\\\"$"));
    else
       cmd = cString::sprintf("%s %s \"%s\"", command, State, *strescape(RecordingFileName, "\\\"$"));
    isyslog("executing '%s'", *cmd);
    SystemExec(cmd);
  }
}
