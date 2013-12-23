/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Portions Copyright (C) 2005-2013 Team XBMC
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "TestUtils.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"

#include <iostream>

#ifdef TARGET_WINDOWS
#include <windows.h>
#else
#include <cstdlib>
#include <climits>
#include <ctime>
#endif

class CTempFile : public CFile
{
public:
  CTempFile(){};
  ~CTempFile()
  {
    Delete();
  }
  bool Create(const std::string &suffix)
  {
    char tmp[MAX_PATH];
    int fd;

    m_ptempFileDirectory = CSpecialProtocol::TranslatePath("special://temp/");
    m_ptempFilePath = m_ptempFileDirectory + "vdrtempfileXXXXXX";
    m_ptempFilePath += suffix;
    if (m_ptempFilePath.length() >= MAX_PATH)
    {
      m_ptempFilePath = "";
      return false;
    }
    strcpy(tmp, m_ptempFilePath.c_str());

#ifdef TARGET_WINDOWS
    if (!GetTempFileName(CSpecialProtocol::TranslatePath("special://temp/"),
                         "vdrtempfile", 0, tmp))
    {
      m_ptempFilePath = "";
      return false;
    }
    m_ptempFilePath = tmp;
#else
    if ((fd = mkstemps(tmp, suffix.length())) < 0)
    {
      m_ptempFilePath = "";
      return false;
    }
    close(fd);
    m_ptempFilePath = tmp;
#endif

    OpenForWrite(m_ptempFilePath.c_str(), true);
    return true;
  }
  bool Delete()
  {
    Close();
    return CFile::Delete(m_ptempFilePath);
  };
  std::string getTempFilePath() const
  {
    return m_ptempFilePath;
  }
  std::string getTempFileDirectory() const
  {
    return m_ptempFileDirectory;
  }
private:
  std::string m_ptempFilePath;
  std::string m_ptempFileDirectory;
};

CVDRTestUtils::CVDRTestUtils()
{
  probability = 0.01;
}

CVDRTestUtils &CVDRTestUtils::Instance()
{
  static CVDRTestUtils instance;
  return instance;
}

std::string CVDRTestUtils::ReferenceFilePath(std::string const& path)
{
  return CSpecialProtocol::TranslatePath("special://vdr") + path;
}

bool CVDRTestUtils::SetReferenceFileBasePath()
{
  std::string vdrPath = GetHomePath();
  if (vdrPath.empty())
  {
    std::cout << "Failed!" << std::endl;
    return false;
  }

  /* Set vdr path */
  CSpecialProtocol::SetVDRPath(vdrPath);
  CSpecialProtocol::SetHomePath(vdrPath); // TODO
  CSpecialProtocol::SetXBMCHomePath(vdrPath); // TODO

  return true;
}

CFile *CVDRTestUtils::CreateTempFile(std::string const& suffix)
{
  CTempFile *f = new CTempFile();
  if (f->Create(suffix))
    return f;
  delete f;
  return NULL;
}

bool CVDRTestUtils::DeleteTempFile(CFile *tempfile)
{
  if (!tempfile)
    return true;
  CTempFile *f = static_cast<CTempFile*>(tempfile);
  bool retval = f->Delete();
  delete f;
  return retval;
}

std::string CVDRTestUtils::TempFilePath(CFile const* const tempfile)
{
  if (!tempfile)
    return "";
  CTempFile const* const f = static_cast<CTempFile const* const>(tempfile);
  return f->getTempFilePath();
}

std::string CVDRTestUtils::TempFileDirectory(CFile const* const tempfile)
{
  if (!tempfile)
    return "";
  CTempFile const* const f = static_cast<CTempFile const* const>(tempfile);
  return f->getTempFileDirectory();
}

CFile *CVDRTestUtils::CreateCorruptedFile(std::string const& strFileName,
  std::string const& suffix)
{
  CFile inputfile, *tmpfile = CreateTempFile(suffix);
  unsigned char buf[20], tmpchar;
  unsigned int size, i;

  if (tmpfile && inputfile.Open(strFileName))
  {
    srand(time(NULL));
    while ((size = inputfile.Read(buf, sizeof(buf))) > 0)
    {
      for (i = 0; i < size; i++)
      {
        if ((rand() % RAND_MAX) < (probability * RAND_MAX))
        {
          tmpchar = buf[i];
          do
          {
            buf[i] = (rand() % 256);
          } while (buf[i] == tmpchar);
        }
      }
      if (tmpfile->Write(buf, size) < 0)
      {
        inputfile.Close();
        tmpfile->Close();
        DeleteTempFile(tmpfile);
        return NULL;
      }
    }
    inputfile.Close();
    tmpfile->Close();
    return tmpfile;
  }
  delete tmpfile;
  return NULL;
}


std::vector<std::string> &CVDRTestUtils::getTestFileFactoryReadUrls()
{
  return TestFileFactoryReadUrls;
}

std::vector<std::string> &CVDRTestUtils::getTestFileFactoryWriteUrls()
{
  return TestFileFactoryWriteUrls;
}

std::string &CVDRTestUtils::getTestFileFactoryWriteInputFile()
{
  return TestFileFactoryWriteInputFile;
}

void CVDRTestUtils::setTestFileFactoryWriteInputFile(std::string const& file)
{
  TestFileFactoryWriteInputFile = file;
}

std::vector<std::string> &CVDRTestUtils::getAdvancedSettingsFiles()
{
  return AdvancedSettingsFiles;
}

std::vector<std::string> &CVDRTestUtils::getGUISettingsFiles()
{
  return GUISettingsFiles;
}

static const char usage[] =
"VDR Test Suite\n"
"Usage: vdr-test [options]\n"
"\n"
"The following options are recognized by the vdr-test program.\n"
"\n"
"  --add-testfilefactory-readurl [URL]\n"
"    Add a url to be used int the TestFileFactory read tests.\n"
"\n"
"  --add-testfilefactory-readurls [URLS]\n"
"    Add multiple urls from a ',' delimited string of urls to be used\n"
"    in the TestFileFactory read tests.\n"
"\n"
"  --add-testfilefactory-writeurl [URL]\n"
"    Add a url to be used int the TestFileFactory write tests.\n"
"\n"
"  --add-testfilefactory-writeurls [URLS]\n"
"    Add multiple urls from a ',' delimited string of urls to be used\n"
"    in the TestFileFactory write tests.\n"
"\n"
"  --set-testfilefactory-writeinputfile [FILE]\n"
"    Set the path to the input file used in the TestFileFactory write tests.\n"
"\n"
"  --add-advancedsettings-file [FILE]\n"
"    Add an advanced settings file to be loaded in test cases that use them.\n"
"\n"
"  --add-advancedsettings-files [FILES]\n"
"    Add multiple advanced settings files from a ',' delimited string of\n"
"    files to be loaded in test cases that use them.\n"
"\n"
"  --add-guisettings-file [FILE]\n"
"    Add a GUI settings file to be loaded in test cases that use them.\n"
"\n"
"  --add-guisettings-files [FILES]\n"
"    Add multiple GUI settings files from a ',' delimited string of\n"
"    files to be loaded in test cases that use them.\n"
"\n"
"  --set-probability [PROBABILITY]\n"
"    Set the probability variable used by the file corrupting functions.\n"
"    The variable should be a double type from 0.0 to 1.0. Values given\n"
"    less than 0.0 are treated as 0.0. Values greater than 1.0 are treated\n"
"    as 1.0. The default probability is 0.01.\n"
;

void CVDRTestUtils::ParseArgs(int argc, char **argv)
{
  int i;
  std::string arg;
  for (i = 1; i < argc; i++)
  {
    arg = argv[i];
    if (arg == "--add-testfilefactory-readurl")
    {
      TestFileFactoryReadUrls.push_back(argv[++i]);
    }
    else if (arg == "--add-testfilefactory-readurls")
    {
      arg = argv[++i];
      std::vector<std::string> urls;
      StringUtils::Split(arg, ",", urls);
      std::vector<std::string>::iterator it;
      for (it = urls.begin(); it < urls.end(); it++)
        TestFileFactoryReadUrls.push_back(*it);
    }
    else if (arg == "--add-testfilefactory-writeurl")
    {
      TestFileFactoryWriteUrls.push_back(argv[++i]);
    }
    else if (arg == "--add-testfilefactory-writeurls")
    {
      arg = argv[++i];
      std::vector<std::string> urls;
      StringUtils::Split(arg, ",", urls);
      std::vector<std::string>::iterator it;
      for (it = urls.begin(); it < urls.end(); it++)
        TestFileFactoryWriteUrls.push_back(*it);
    }
    else if (arg == "--set-testfilefactory-writeinputfile")
    {
      TestFileFactoryWriteInputFile = argv[++i];
    }
    else if (arg == "--add-advancedsettings-file")
    {
      AdvancedSettingsFiles.push_back(argv[++i]);
    }
    else if (arg == "--add-advancedsettings-files")
    {
      arg = argv[++i];
      std::vector<std::string> urls;
      StringUtils::Split(arg, ",", urls);
      std::vector<std::string>::iterator it;
      for (it = urls.begin(); it < urls.end(); it++)
        AdvancedSettingsFiles.push_back(*it);
    }
    else if (arg == "--add-guisettings-file")
    {
      GUISettingsFiles.push_back(argv[++i]);
    }
    else if (arg == "--add-guisettings-files")
    {
      arg = argv[++i];
      std::vector<std::string> urls;
      StringUtils::Split(arg, ",", urls);
      std::vector<std::string>::iterator it;
      for (it = urls.begin(); it < urls.end(); it++)
        GUISettingsFiles.push_back(*it);
    }
    else if (arg == "--set-probability")
    {
      probability = atof(argv[++i]);
      if (probability < 0.0)
        probability = 0.0;
      else if (probability > 1.0)
        probability = 1.0;
    }
    else
    {
      std::cerr << usage;
      exit(EXIT_FAILURE);
    }
  }
}

std::string CVDRTestUtils::getNewLineCharacters() const
{
#ifdef TARGET_WINDOWS
  return "\r\n";
#else
  return "\n";
#endif
}

std::string CVDRTestUtils::GetHomePath(const std::string strEnvVariable /* = VDR_HOME_ENV_VARIABLE */)
{
  //std::string strPath = CEnvironment::getenv(strEnvVariable); // TODO
  std::string strPath;

  // TODO
  /*
#ifdef TARGET_WINDOWS
  if (strPath.find("..") != std::string::npos)
  {
    //expand potential relative path to full path
    CStdStringW strPathW;
    g_charsetConverter.utf8ToW(strPath, strPathW, false);
    CWIN32Util::AddExtraLongPathPrefix(strPathW);
    const unsigned int bufSize = GetFullPathNameW(strPathW, 0, NULL, NULL);
    if (bufSize != 0)
    {
      wchar_t * buf = new wchar_t[bufSize];
      if (GetFullPathNameW(strPathW, bufSize, buf, NULL) <= bufSize-1)
      {
        std::wstring expandedPathW(buf);
        CWIN32Util::RemoveExtraLongPathPrefix(expandedPathW);
        g_charsetConverter.wToUTF8(expandedPathW, strPath);
      }

      delete [] buf;
    }
  }
#endif
  */

  if (strPath.empty())
  {
    // TODO
    /*
#if defined(TARGET_DARWIN)
    int      result = -1;
    char     given_path[2*MAXPATHLEN];
    uint32_t path_size =2*MAXPATHLEN;

    result = GetDarwinExecutablePath(given_path, &path_size);
    if (result == 0)
    {
      // Move backwards to last /.
      for (int n=strlen(given_path)-1; given_path[n] != '/'; n--)
        given_path[n] = '\0';

      #if defined(TARGET_DARWIN_IOS)
        strcat(given_path, "/VDRData/VDRHome/");
      #else
        // Assume local path inside application bundle.
        strcat(given_path, "../Resources/VDR/");
      #endif

      // Convert to real path.
      char real_path[2*MAXPATHLEN];
      if (realpath(given_path, real_path) != NULL)
        return real_path;
    }
#endif
    */

    std::string strHomePath = GetExecutablePath();
    size_t last_sep = strHomePath.find_last_of(PATH_SEPARATOR_CHAR);
    if (last_sep != std::string::npos)
      strPath = strHomePath.substr(0, last_sep);
    else
      strPath = strHomePath;
  }

#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
  /* Change strPath accordingly when target is VDR_HOME and when INSTALL_PATH
   * and BIN_INSTALL_PATH differ
   */
  std::string installPath = INSTALL_PATH;
  std::string binInstallPath = BIN_INSTALL_PATH;
  if (!strEnvVariable.compare("VDR_HOME") && installPath.compare(binInstallPath))
  {
    int pos = strPath.length() - binInstallPath.length();
    std::string tmp = strPath;
    tmp.erase(0, pos);
    if (!tmp.compare(binInstallPath))
    {
      strPath.erase(pos, strPath.length());
      strPath.append(installPath);
    }
  }
#endif

  return strPath;
}

std::string CVDRTestUtils::GetExecutablePath()
{
  std::string strExecutablePath;

#ifdef TARGET_WINDOWS

  static const size_t bufSize = MAX_PATH * 2;
  wchar_t* buf = new wchar_t[bufSize];
  buf[0] = 0;
  ::GetModuleFileNameW(0, buf, bufSize);
  buf[bufSize-1] = 0;
  g_charsetConverter.wToUTF8(buf, strExecutablePath);
  delete[] buf;

#elif defined(TARGET_DARWIN)

  char     given_path[2*MAXPATHLEN];
  uint32_t path_size =2*MAXPATHLEN;

  GetDarwinExecutablePath(given_path, &path_size);
  strExecutablePath = given_path;

#elif defined(TARGET_FREEBSD)

  char buf[PATH_MAX];
  size_t buflen;
  int mib[4];

  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = getpid();

  buflen = sizeof(buf) - 1;
  if (sysctl(mib, 4, buf, &buflen, NULL, 0) < 0)
    strExecutablePath.clear(); // TODO: This seems wrong
  else
    strExecutablePath = buf;

#else

  /* Get our PID and build the name of the link in /proc */
  pid_t pid = getpid();
  char linkname[64]; /* /proc/<pid>/exe */
  snprintf(linkname, sizeof(linkname), "/proc/%i/exe", pid);

  /* Now read the symbolic link */
  char buf[PATH_MAX + 1];
  buf[0] = 0;

  int ret = readlink(linkname, buf, sizeof(buf) - 1);
  if (ret != -1)
    buf[ret] = 0;

  strExecutablePath = buf;

#endif

  return strExecutablePath;
}
