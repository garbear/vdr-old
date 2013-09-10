#pragma once

typedef struct
{
  struct termios savedTm;
  bool           HasStdin;
  bool           StartedAsRoot;
  const char *   VdrUser;
  bool           UserDump;
  int            SVDRPport;
  const char *   AudioCommand;
  const char *   VideoDirectory;
  const char *   ConfigDirectory;
  const char *   CacheDirectory;
  const char *   ResourceDirectory;
  const char *   LocaleDirectory;
  const char *   EpgDataFileName;
  bool           DisplayHelp;
  bool           DisplayVersion;
  bool           DaemonMode;
  int            SysLogTarget;
  bool           MuteAudio;
  int            WatchdogTimeout;
  const char *   Terminal;
  bool           UseKbd;
  const char *   LircDevice;
} vdr_daemon_opts;
