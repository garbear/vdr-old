/*
 *      Copyright (C) 2013-2014 Garrett Brown
 *      Copyright (C) 2013-2014 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
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

#include "utils/IntervalTree.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace std;

namespace VDR
{

typedef std::vector<Interval<std::string> > IntervalVector;

/* 0123456789012345678901234567890123456789
 * |---------1---------|
 *               |-------2-------|
 *               |--3--|
 *   |----4----|
 *               |------5-----|
 */
Interval<string> interval1("Interval1", 0, 20);
Interval<string> interval2("Interval2", 14, 30);
Interval<string> interval3("Interval3", 14, 20);
Interval<string> interval4("Interval4", 2, 12);
Interval<string> interval5("Interval5", 14, 27);

TEST(IntervalTree, Insert)
{
  IntervalTree<string> intervalTree;
  intervalTree.Insert(interval1);
  intervalTree.Insert(interval2);
  intervalTree.Insert(interval3);
  intervalTree.Insert(interval4);
  intervalTree.Insert(interval5);

  EXPECT_EQ(5, intervalTree.Size());
}

TEST(IntervalTree, Erase)
{
  IntervalTree<string> intervalTree;
  intervalTree.Insert(interval1);
  intervalTree.Insert(interval2);
  intervalTree.Insert(interval3);
  intervalTree.Insert(interval4);
  intervalTree.Insert(interval5);

  intervalTree.Erase(interval1);
  intervalTree.Erase(interval3);
  intervalTree.Erase(interval2);
  intervalTree.Erase(interval5);

  EXPECT_EQ(1, intervalTree.Size());
}

TEST(IntervalTree, GetIntersectingPoint)
{
  IntervalTree<string> intervalTree;
  intervalTree.Insert(interval1);
  intervalTree.Insert(interval2);
  intervalTree.Insert(interval3);
  intervalTree.Insert(interval4);
  intervalTree.Insert(interval5);

  IntervalVector intervals;
  intervalTree.GetIntersecting(17, intervals);
  EXPECT_EQ(4, intervals.size());
  EXPECT_TRUE(std::find(intervals.begin(), intervals.end(), interval1) != intervals.end());
  EXPECT_TRUE(std::find(intervals.begin(), intervals.end(), interval2) != intervals.end());
  EXPECT_TRUE(std::find(intervals.begin(), intervals.end(), interval3) != intervals.end());
  EXPECT_TRUE(std::find(intervals.begin(), intervals.end(), interval4) == intervals.end());
  EXPECT_TRUE(std::find(intervals.begin(), intervals.end(), interval5) != intervals.end());
}

TEST(IntervalTree, GetIntersectingInterval)
{
  IntervalTree<string> intervalTree;
  intervalTree.Insert(interval1);
  intervalTree.Insert(interval2);
  intervalTree.Insert(interval3);
  intervalTree.Insert(interval4);
  intervalTree.Insert(interval5);

  IntervalVector intervals;
  intervalTree.GetIntersecting(interval3, intervals);
  EXPECT_EQ(4, intervals.size());
  EXPECT_TRUE(std::find(intervals.begin(), intervals.end(), interval1) != intervals.end());
  EXPECT_TRUE(std::find(intervals.begin(), intervals.end(), interval2) != intervals.end());
  EXPECT_TRUE(std::find(intervals.begin(), intervals.end(), interval3) != intervals.end());
  EXPECT_TRUE(std::find(intervals.begin(), intervals.end(), interval4) == intervals.end());
  EXPECT_TRUE(std::find(intervals.begin(), intervals.end(), interval5) != intervals.end());
}

}
