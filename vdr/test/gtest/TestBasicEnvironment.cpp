/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "TestBasicEnvironment.h"
#include "TestUtils.h"
#include "filesystem/Directory.h"
//#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "lib/platform/os.h"
//#include "powermanagement/PowerManager.h"
//#include "settings/AdvancedSettings.h"
//#include "settings/Settings.h"
//#include "Util.h"

//#include <cstdio>
#include <cstdlib>
#include <climits>
#include <string>

using namespace std;

void TestBasicEnvironment::SetUp()
{
  char *tmp;
  CFile *f;

  /* NOTE: The below is done to fix memleak warning about unitialized variable
   * in vdrutil::GlobalsSingleton<CAdvancedSettings>::getInstance().
   */
  //g_advancedSettings.Initialize(); // TODO

  if (!CVDRTestUtils::Instance().SetReferenceFileBasePath())
    SetUpError();
  CVDRTestUtils::Instance().setTestFileFactoryWriteInputFile(
    VDR_REF_FILE_PATH("vdr/filesystem/test/reffile.txt")
  );

  // For darwin set framework path - else we get assert in guisettings init below // TODO
  /*
#ifdef TARGET_DARWIN
  string frameworksPath = CUtil::GetFrameworksPath();
  CSpecialProtocol::SetVDRFrameworksPath(frameworksPath);
#endif
  */

  /* Something should be done about all the asserts in GUISettings so
   * that the initialization of these components won't be needed.
   */
  //g_powerManager.Initialize(); // TODO
  //CSettings::Get().Initialize(); // TODO

  /* Create a temporary directory and set it to be used throughout the
   * test suite run.
   */
#ifdef TARGET_WINDOWS

  TCHAR lpTempPathBuffer[MAX_PATH];
  if (!GetTempPath(MAX_PATH, lpTempPathBuffer))
    SetUpError();
  string vdrTempPath = lpTempPathBuffer;
  if (!GetTempFileName(vdrTempPath.c_str(), "vdrtempdir", 0, lpTempPathBuffer))
    SetUpError();
  DeleteFile(lpTempPathBuffer);
  if (!CreateDirectory(lpTempPathBuffer, NULL))
    SetUpError();
  CSpecialProtocol::SetTempPath(lpTempPathBuffer);

#else

  char buf[MAX_PATH];
  strcpy(buf, "/tmp/vdrtempdirXXXXXX");

  if ((tmp = mkdtemp(buf)) == NULL)
    SetUpError();
  CSpecialProtocol::SetTempPath(tmp);
  CSpecialProtocol::SetXBMCTempPath(tmp); // TODO

#endif

  /* Create and delete a tempfile to initialize the VFS (really to initialize
   * CLibcdio). This is done so that the initialization of the VFS does not
   * affect the performance results of the test cases.
   */
  /* TODO: Make the initialization of the VFS here optional so it can be
   * testable in a test case.
   */
  f = VDR_CREATETEMPFILE("");
  if (!f || !VDR_DELETETEMPFILE(f))
  {
    TearDown();
    SetUpError();
  }
}

void TestBasicEnvironment::TearDown()
{
  string vdrTempPath = CSpecialProtocol::TranslatePath("special://temp/");
  CDirectory::Remove(vdrTempPath);
}

void TestBasicEnvironment::SetUpError()
{
  fprintf(stderr, "Setup of basic environment failed.\n");
  exit(EXIT_FAILURE);
}
