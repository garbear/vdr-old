/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/StringUtils.h"

#include "gtest/gtest.h"

#include <set>
#include <string>
#include <vector>

using namespace std;

TEST(TestStringUtils, Format)
{
  string refstr = "test 25 2.7 ff FF";

  string varstr = StringUtils::Format("%s %d %.1f %x %02X", "test", 25, 2.743f, 0x00ff, 0x00ff);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = StringUtils::Format("", "test", 25, 2.743f, 0x00ff, 0x00ff);
  EXPECT_STREQ("", varstr.c_str());
}

TEST(TestStringUtils, ToUpper)
{
  string refstr = "TEST";

  string varstr = "TeSt";
  StringUtils::ToUpper(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, ToLower)
{
  string refstr = "test";
  
  string varstr = "TeSt";
  StringUtils::ToLower(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, EqualsNoCase)
{
  string refstr = "TeSt";
  
  EXPECT_TRUE(StringUtils::EqualsNoCase(refstr, "TeSt"));
  EXPECT_TRUE(StringUtils::EqualsNoCase(refstr, "tEsT"));
}

TEST(TestStringUtils, Left)
{
  string refstr, varstr;
  string origstr = "test";
  
  refstr = "";
  varstr = StringUtils::Left(origstr, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  
  refstr = "te";
  varstr = StringUtils::Left(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  
  refstr = "test";
  varstr = StringUtils::Left(origstr, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, Mid)
{
  string refstr, varstr;
  string origstr = "test";
  
  refstr = "";
  varstr = StringUtils::Mid(origstr, 0, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  
  refstr = "te";
  varstr = StringUtils::Mid(origstr, 0, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  
  refstr = "test";
  varstr = StringUtils::Mid(origstr, 0, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  
  refstr = "st";
  varstr = StringUtils::Mid(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  
  refstr = "st";
  varstr = StringUtils::Mid(origstr, 2, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  
  refstr = "es";
  varstr = StringUtils::Mid(origstr, 1, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, Right)
{
  string refstr, varstr;
  string origstr = "test";
  
  refstr = "";
  varstr = StringUtils::Right(origstr, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  
  refstr = "st";
  varstr = StringUtils::Right(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  
  refstr = "test";
  varstr = StringUtils::Right(origstr, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, Trim)
{
  string refstr = "test test";
  
  string varstr = " test test   ";
  StringUtils::Trim(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, TrimLeft)
{
  string refstr = "test test   ";
  
  string varstr = " test test   ";
  StringUtils::TrimLeft(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, TrimRight)
{
  string refstr = " test test";
  
  string varstr = " test test   ";
  StringUtils::TrimRight(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, Replace)
{
  string refstr = "text text";
  
  string varstr = "test test";
  EXPECT_EQ(StringUtils::Replace(varstr, 's', 'x'), 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  
  EXPECT_EQ(StringUtils::Replace(varstr, 's', 'x'), 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  
  varstr = "test test";
  EXPECT_EQ(StringUtils::Replace(varstr, "s", "x"), 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  
  EXPECT_EQ(StringUtils::Replace(varstr, "s", "x"), 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, StartsWith)
{
  string refstr = "test";
  
  EXPECT_FALSE(StringUtils::StartsWithNoCase(refstr, "x"));
  
  EXPECT_TRUE(StringUtils::StartsWith(refstr, "te"));
  EXPECT_TRUE(StringUtils::StartsWith(refstr, "test"));
  EXPECT_FALSE(StringUtils::StartsWith(refstr, "Te"));
  
  EXPECT_TRUE(StringUtils::StartsWithNoCase(refstr, "Te"));
  EXPECT_TRUE(StringUtils::StartsWithNoCase(refstr, "TesT"));
}

TEST(TestStringUtils, EndsWith)
{
  string refstr = "test";
  
  EXPECT_FALSE(StringUtils::EndsWithNoCase(refstr, "x"));
  
  EXPECT_TRUE(StringUtils::EndsWith(refstr, "st"));
  EXPECT_TRUE(StringUtils::EndsWith(refstr, "test"));
  EXPECT_FALSE(StringUtils::EndsWith(refstr, "sT"));
  
  EXPECT_TRUE(StringUtils::EndsWithNoCase(refstr, "sT"));
  EXPECT_TRUE(StringUtils::EndsWithNoCase(refstr, "TesT"));
}

TEST(TestStringUtils, Join)
{
  string refstr, varstr;
  vector<string> strarray;

  strarray.push_back("a");
  strarray.push_back("b");
  strarray.push_back("c");
  strarray.push_back("de");
  strarray.push_back(",");
  strarray.push_back("fg");
  strarray.push_back(",");
  refstr = "a,b,c,de,,,fg,,";
  varstr = StringUtils::Join(strarray, ",");
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, Split)
{
  vector<string> varresults;

  int results = StringUtils::Split("g,h,ij,k,lm,,n", ",", varresults);
  EXPECT_EQ(results, 7);
  EXPECT_STREQ("g", varresults.at(0).c_str());
  EXPECT_STREQ("h", varresults.at(1).c_str());
  EXPECT_STREQ("ij", varresults.at(2).c_str());
  EXPECT_STREQ("k", varresults.at(3).c_str());
  EXPECT_STREQ("lm", varresults.at(4).c_str());
  EXPECT_STREQ("", varresults.at(5).c_str());
  EXPECT_STREQ("n", varresults.at(6).c_str());
}

TEST(TestStringUtils, FindNumber)
{
  EXPECT_EQ(3, StringUtils::FindNumber("aabcaadeaa", "aa"));
  EXPECT_EQ(1, StringUtils::FindNumber("aabcaadeaa", "b"));
}

TEST(TestStringUtils, AlphaNumericCompare)
{
  int64_t ref, var;

  ref = 0;
  var = StringUtils::AlphaNumericCompare(L"123abc", L"abc123");
  EXPECT_LT(var, ref);
}

TEST(TestStringUtils, TimeStringToSeconds)
{
  EXPECT_EQ(77455, StringUtils::TimeStringToSeconds("21:30:55"));
  EXPECT_EQ(7*60, StringUtils::TimeStringToSeconds("7 min"));
  EXPECT_EQ(7*60, StringUtils::TimeStringToSeconds("7 min\t"));
  EXPECT_EQ(154*60, StringUtils::TimeStringToSeconds("   154 min"));
  EXPECT_EQ(1*60+1, StringUtils::TimeStringToSeconds("1:01"));
  EXPECT_EQ(4*60+3, StringUtils::TimeStringToSeconds("4:03"));
  EXPECT_EQ(2*3600+4*60+3, StringUtils::TimeStringToSeconds("2:04:03"));
  EXPECT_EQ(2*3600+4*60+3, StringUtils::TimeStringToSeconds("   2:4:3"));
  EXPECT_EQ(2*3600+4*60+3, StringUtils::TimeStringToSeconds("  \t\t 02:04:03 \n "));
  EXPECT_EQ(1*3600+5*60+2, StringUtils::TimeStringToSeconds("01:05:02:04:03 \n "));
  EXPECT_EQ(0, StringUtils::TimeStringToSeconds("blah"));
  EXPECT_EQ(0, StringUtils::TimeStringToSeconds("ля-ля"));
}

TEST(TestStringUtils, RemoveCRLF)
{
  string refstr, varstr;

  refstr = "test\r\nstring\nblah blah";
  varstr = "test\r\nstring\nblah blah\n";
  StringUtils::RemoveCRLF(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, utf8_strlen)
{
  int ref, var;

  ref = 9;
  var = StringUtils::utf8_strlen("ｔｅｓｔ＿ＵＴＦ８");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, IsNaturalNumber)
{
  EXPECT_TRUE(StringUtils::IsNaturalNumber("10"));
  EXPECT_TRUE(StringUtils::IsNaturalNumber(" 10"));
  EXPECT_TRUE(StringUtils::IsNaturalNumber("0"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber(" 1 0"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber("1.0"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber("1.1"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber("0x1"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber("blah"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber("120 h"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber(" "));
  EXPECT_FALSE(StringUtils::IsNaturalNumber(""));
}

TEST(TestStringUtils, IsInteger)
{
  EXPECT_TRUE(StringUtils::IsInteger("10"));
  EXPECT_TRUE(StringUtils::IsInteger(" -10"));
  EXPECT_TRUE(StringUtils::IsInteger("0"));
  EXPECT_FALSE(StringUtils::IsInteger(" 1 0"));
  EXPECT_FALSE(StringUtils::IsInteger("1.0"));
  EXPECT_FALSE(StringUtils::IsInteger("1.1"));
  EXPECT_FALSE(StringUtils::IsInteger("0x1"));
  EXPECT_FALSE(StringUtils::IsInteger("blah"));
  EXPECT_FALSE(StringUtils::IsInteger("120 h"));
  EXPECT_FALSE(StringUtils::IsInteger(" "));
  EXPECT_FALSE(StringUtils::IsInteger(""));
}

TEST(TestStringUtils, SizeToString)
{
  string ref, var;

  ref = "2.00 GB";
  var = StringUtils::SizeToString(2147483647);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestStringUtils, Empty)
{
  EXPECT_STREQ("", StringUtils::Empty.c_str());
}

TEST(TestStringUtils, FindWords)
{
  int ref, var;

  ref = 5;
  var = StringUtils::FindWords("test string", "string");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWords("12345string", "string");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWords("apple2012", "2012");
  EXPECT_EQ(ref, var);
  ref = -1;
  var = StringUtils::FindWords("12345string", "ring");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWords("12345string", "345");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWords("apple2012", "e2012");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWords("apple2012", "12");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, FindWords_NonAscii)
{
  int ref, var;

  ref = 6;
  var = StringUtils::FindWords("我的视频", "视频");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWords("我的视频", "视");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWords("Apple ple", "ple");
  EXPECT_EQ(ref, var);
  ref = 7;
  var = StringUtils::FindWords("Äpfel.pfel", "pfel");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, FindEndBracket)
{
  int ref, var;

  ref = 11;
  var = StringUtils::FindEndBracket("atest testbb test", 'a', 'b');
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, DateStringToYYYYMMDD)
{
  int ref, var;

  ref = 20120706;
  var = StringUtils::DateStringToYYYYMMDD("2012-07-06");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, WordToDigits)
{
  string ref, var;

  ref = "8378 787464";
  var = "test string";
  StringUtils::WordToDigits(var);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestStringUtils, Paramify)
{
  const char *input = "some, very \\ odd \"string\"";
  const char *ref   = "\"some, very \\\\ odd \\\"string\\\"\"";

  string result = StringUtils::Paramify(input);
  EXPECT_STREQ(ref, result.c_str());
}