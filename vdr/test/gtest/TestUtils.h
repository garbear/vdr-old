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
#pragma once

#include <string>
#include <vector>

namespace VDR
{

class CFile;

class CVDRTestUtils
{
public:
  static CVDRTestUtils &Instance();

  /* ReferenceFilePath() is used to prepend a path with the location to the
   * vdr-test binary. It's assumed the test suite program will only be run
   * with vdr-test residing in the source tree.
   */
  std::string ReferenceFilePath(std::string const& path);

  /* Function used in creating a temporary file. It accepts a parameter
   * 'suffix' to append to the end of the tempfile path. The temporary
   * file is return as a CFile object.
   */
  CFile *CreateTempFile(std::string const& suffix);

  /* Function used to close and delete a temporary file previously created
   * using CreateTempFile().
   */
  bool DeleteTempFile(CFile *tempfile);

  /* Function to get path of a tempfile */
  std::string TempFilePath(CFile const* const tempfile);

  /* Get the containing directory of a tempfile */
  std::string TempFileDirectory(CFile const* const tempfile);

  /* Functions to get variables used in the TestFileFactory tests. */
  std::vector<std::string> &getTestFileFactoryReadUrls();

  /* Function to get variables used in the TestFileFactory tests. */
  std::vector<std::string> &getTestFileFactoryWriteUrls();

  /* Function to get the input file used in the TestFileFactory.Write tests. */
  std::string &getTestFileFactoryWriteInputFile();

  /* Function to set the input file used in the TestFileFactory.Write tests */
  void setTestFileFactoryWriteInputFile(std::string const& file);

  /* Function to get advanced settings files. */
  std::vector<std::string> &getAdvancedSettingsFiles();

  /* Function to get GUI settings files. */
  std::vector<std::string> &getGUISettingsFiles();

  /* Function used in creating a corrupted file. The parameters are a URL
   * to the original file to be corrupted and a suffix to append to the
   * path of the newly created file. This will return a CFile
   * object which is itself a tempfile object which can be used with the
   * tempfile functions of this utility class.
   */
  CFile *CreateCorruptedFile(std::string const& strFileName, std::string const& suffix);

  /* Function to parse command line options */
  void ParseArgs(int argc, char **argv);

  /* Function to return the newline characters for this platform */
  std::string getNewLineCharacters() const;
private:
  CVDRTestUtils();
  CVDRTestUtils(CVDRTestUtils const&);
  void operator=(CVDRTestUtils const&);

  std::vector<std::string> TestFileFactoryReadUrls;
  std::vector<std::string> TestFileFactoryWriteUrls;
  std::string TestFileFactoryWriteInputFile;

  std::vector<std::string> AdvancedSettingsFiles;
  std::vector<std::string> GUISettingsFiles;

  double probability;
};

#define VDR_REF_FILE_PATH(s)          CVDRTestUtils::Instance().ReferenceFilePath(s)
#define VDR_CREATETEMPFILE(a)         CVDRTestUtils::Instance().CreateTempFile(a)
#define VDR_DELETETEMPFILE(a)         CVDRTestUtils::Instance().DeleteTempFile(a)
#define VDR_TEMPFILEPATH(a)           CVDRTestUtils::Instance().TempFilePath(a)
#define VDR_CREATECORRUPTEDFILE(a, b) CVDRTestUtils::Instance().CreateCorruptedFile(a, b)

}
