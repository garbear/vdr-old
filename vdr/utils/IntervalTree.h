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
#pragma once

#include <algorithm>
#include <assert.h>
#include <map>
#include <set>
#include <stddef.h>
#include <time.h>
#include <vector>

/*
 * Centered interval tree algorithm from wikipedia
 */
namespace VDR
{

/*!
 * A closed interval with x_low <= x_high.
 */
template <class T, typename K = int>
class Interval
{
public:
  Interval(T value, K x_low, K x_high);

  bool Contains(K x) const { return x_low <= x && x <= x_high; }

  T value;
  K x_low;
  K x_high;
};

template <class T, typename K = int>
class IntervalNode
{
public:
  typedef Interval<T, K>     IntervalType;
  typedef IntervalNode<T, K> IntervalNodeType;

  IntervalNode(const IntervalType& interval);
  IntervalNode(const IntervalNodeType& other);

  // A node has a center point and contains all the intervals that overlap it
  K x_center;

  // The node's intervals are stored in two lists sorted by increasing starting
  // time and decreasing finishing time
  struct CompareStart
  {
    bool operator()(const IntervalType& lhs, const IntervalType& rhs) { return lhs.x_low < rhs.x_low; }
  };

  struct CompareFinish
  {
    bool operator()(const IntervalType& lhs, const IntervalType& rhs) { return lhs.x_high > rhs.x_high; }
  };

  typedef std::multiset<IntervalType, CompareStart>  IntervalSetByStart;
  typedef std::multiset<IntervalType, CompareFinish> IntervalSetByFinish;

  IntervalSetByStart  S_centerByStart;
  IntervalSetByFinish S_centerByFinish;

  // All intervals that lie completely to the left of x_center
  IntervalNodeType* S_left;

  // All intervals that lie completely to the right of x_center
  IntervalNodeType* S_right;

  void SwapIntervals(IntervalNodeType& other);
};

template <class T, typename K = int>
class IntervalTree
{
public:
  typedef Interval<T, K>            IntervalType;
  typedef std::vector<IntervalType> IntervalVector;

  IntervalTree(void);
  IntervalTree(const IntervalTree<T, K>& other);
  ~IntervalTree(void);

  void Insert(const IntervalType& interval);
  void Erase(const IntervalType& interval);
  void EraseAll(const T& value);
  void Clear(void);

  size_t Size(void) const;
  bool Empty(void) const { return m_root != NULL; }

  bool GetIntersecting(K x, IntervalVector& intervals) const;
  bool GetIntersecting(const IntervalType& interval, IntervalVector& intervals) const;

  K GetMin(void) const;
  K GetMax(void) const;

private:
  typedef IntervalNode<T, K> IntervalNodeType;

  static void InsertRecursive(const IntervalType& interval, IntervalNodeType*& node);
  static void EraseRecursive(const IntervalType& interval, IntervalNodeType*& node);
  static void EraseNodeRecursive(IntervalNodeType*& node);
  static void EraseAllRecursive(const T& value, IntervalNodeType*& node);
  static void ClearRecursive(IntervalNodeType*& node);

  static size_t SizeRecursive(const IntervalNodeType* node);

  static void GetIntersectingRecursive(K x, IntervalVector& intervals, const IntervalNodeType* node);
  static void GetIntersectingRecursive(const IntervalType& interval, IntervalVector& intervals, const IntervalNodeType* node);
  static void GetIntersectingButNotEnclosingRecursive(const IntervalType& interval, IntervalVector& intervals, const IntervalNodeType* node);

  K GetMinRecursive(const IntervalNodeType& node) const;
  K GetMaxRecursive(const IntervalNodeType& node) const;

  IntervalNodeType* m_root;
};


template <class T, typename K>
Interval<T, K>::Interval(T value, K x_low, K x_high)
: value(value),
  x_low(x_low),
  x_high(x_high)
{
}

template <class T, typename K = int>
bool operator==(const Interval<T>& lhs, const Interval<T>& rhs)
{
  return lhs.x_low  == rhs.x_low  &&
         lhs.x_high == rhs.x_high &&
         lhs.value    == rhs.value;
}

template <class T, typename K>
IntervalNode<T, K>::IntervalNode(const IntervalType& interval)
 : x_center(interval.x_low),
   S_left(NULL),
   S_right(NULL)
{
  S_centerByStart.insert(interval);
  S_centerByFinish.insert(interval);
}

template <class T, typename K>
IntervalNode<T, K>::IntervalNode(const IntervalNodeType& other)
 : x_center(other.x_center),
   S_left(other.S_left ? new IntervalNodeType(*other.S_left) : NULL),
   S_right(other.S_right ? new IntervalNodeType(*other.S_right) : NULL)
{
  S_centerByStart = other.S_centerByStart;
  S_centerByFinish = other.S_centerByFinish;
}

template <class T, typename K>
void IntervalNode<T, K>::SwapIntervals(IntervalNodeType& other)
{
  std::swap(x_center, other.x_center);
  S_centerByStart.swap(other.S_centerByStart);
  S_centerByFinish.swap(other.S_centerByFinish);
}

template <class T, typename K>
IntervalTree<T, K>::IntervalTree(void)
 : m_root(NULL)
{
}

template <class T, typename K>
IntervalTree<T, K>::IntervalTree(const IntervalTree& other)
 : m_root(other.m_root ? new IntervalNodeType(*other.m_root) : NULL)
{
}

template <class T, typename K>
IntervalTree<T, K>::~IntervalTree(void)
{
  Clear();
}

template <class T, typename K>
void IntervalTree<T, K>::Insert(const IntervalType& interval)
{
  // Validate input
  if (interval.x_low <= interval.x_high)
    InsertRecursive(interval, m_root);
}

template <class T, typename K>
void IntervalTree<T, K>::InsertRecursive(const IntervalType& interval, IntervalNodeType*& node)
{
  if (!node)
  {
    node = new IntervalNodeType(interval);
  }
  else
  {
    // Interval occurs before node->x_center
    if (interval.x_high < node->x_center)
    {
      InsertRecursive(interval, node->S_left);
    }
    // Interval contains node->x_center
    else if (interval.Contains(node->x_center))
    {
      node->S_centerByStart.insert(interval);
      node->S_centerByFinish.insert(interval);
    }
    // Interval occurs after node->x_center
    else
    {
      assert(node->x_center < interval.x_low);
      InsertRecursive(interval, node->S_right);
    }
  }
}

template <class T, typename K>
void IntervalTree<T, K>::Erase(const IntervalType& interval)
{
  EraseRecursive(interval, m_root);
}

template <class T, typename K>
void IntervalTree<T, K>::EraseRecursive(const IntervalType& interval, IntervalNodeType*& node)
{
  if (node)
  {
    // Interval occurs before node->x_center
    if (interval.x_high < node->x_center)
    {
      EraseRecursive(interval, node->S_left);
    }
    // Interval contains node->x_center
    else if (interval.Contains(node->x_center))
    {
      // Remove the interval from the sorted lists
      bool bFoundStart = false;
      for (typename IntervalNodeType::IntervalSetByStart::iterator it = node->S_centerByStart.begin();
           it != node->S_centerByStart.end(); ++it)
      {
        if (*it == interval)
        {
          bFoundStart = true;
          node->S_centerByStart.erase(it);
          break;
        }
      }

      bool bFoundFinish = false;
      for (typename IntervalNodeType::IntervalSetByFinish::iterator it = node->S_centerByFinish.begin();
           it != node->S_centerByFinish.end(); ++it)
      {
        if (*it == interval)
        {
          bFoundFinish = true;
          node->S_centerByFinish.erase(it);
          break;
        }
      }

      assert(bFoundStart == bFoundFinish);

      // If the node contains no more intervals, it may be deleted from the tree
      if (node->S_centerByStart.empty())
        EraseNodeRecursive(node);
    }
    // Interval occurs after node->x_center
    else
    {
      assert(node->x_center < interval.x_low);
      EraseRecursive(interval, node->S_right);
    }
  }
}

template <class T, typename K>
void IntervalTree<T, K>::EraseNodeRecursive(IntervalNodeType*& node)
{
  if (node)
  {
    if (!node->S_left && !node->S_right)     // Deleting a leaf (node with no children)
    {
      delete node;
      node = NULL;
    }
    else if (node->S_left && !node->S_right) // Deleting a node with one child
    {
      delete node;
      node = node->S_left;
    }
    else if (!node->S_left && node->S_right) // Deleting a node with one child
    {
      delete node;
      node = node->S_right;
    }
    else
    {
      assert(node->S_left && node->S_right); // Deleting a node with two children

      // Until proper balancing is implemented, use the child with the greatest
      // center-to-center distance.
      IntervalNodeType* successor;

      K distLeft = node->x_center - node->S_left->x_center;
      K distRight = node->S_right->x_center - node->x_center;

      const bool bUseInOrderPredecessor = (distLeft >= distRight);

      if (bUseInOrderPredecessor)
      {
        IntervalNodeType* inOrderPredecessor = node->S_left;
        while (inOrderPredecessor->S_right)
          inOrderPredecessor = inOrderPredecessor->S_right;
        successor = inOrderPredecessor;
      }
      else
      {
        IntervalNodeType* inOrderSuccessor = node->S_right;
        while (inOrderSuccessor->S_left)
          inOrderSuccessor = inOrderSuccessor->S_left;
        successor = inOrderSuccessor;
      }

      node->SwapIntervals(*successor);
      EraseNodeRecursive(successor);

      // As a result of the promotion, some nodes that were above successor will
      // become descendents of it; it is necessary to search these nodes for
      // intervals that also overlap the promoted node, and move those intervals
      // into the promoted node.
      IntervalVector intervals;
      GetIntersectingRecursive(node->x_center, intervals, bUseInOrderPredecessor ? node->S_left : node->S_right);

      for (typename IntervalVector::iterator it = intervals.begin(); it != intervals.end(); ++it)
      {
        InsertRecursive(*it, node);
        EraseRecursive(*it, bUseInOrderPredecessor ? node->S_left : node->S_right);
      }
    }
  }
}

template <class T, typename K>
void IntervalTree<T, K>::EraseAll(const T& value)
{
  EraseAllRecursive(value, m_root);
}

template <class T, typename K>
void IntervalTree<T, K>::EraseAllRecursive(const T& value, IntervalNodeType*& node)
{
  if (node)
  {
    EraseAllRecursive(value, node->S_left);
    EraseAllRecursive(value, node->S_right);

    // Remove the interval from the sorted lists
    for (typename IntervalNodeType::IntervalSetByStart::iterator it = node->S_centerByStart.begin();
         it != node->S_centerByStart.end(); ++it)
    {
      if (it->value == value)
        node->S_centerByStart.erase(it);
    }

    for (typename IntervalNodeType::IntervalSetByFinish::iterator it = node->S_centerByFinish.begin();
         it != node->S_centerByFinish.end(); ++it)
    {
      if (it->value == value)
        node->S_centerByFinish.erase(it);
    }

    // If the node contains no more intervals, it may be deleted from the tree
    if (node->S_centerByStart.empty())
      EraseNodeRecursive(node);
  }
}

template <class T, typename K>
void IntervalTree<T, K>::Clear(void)
{
  ClearRecursive(m_root);
}

template <class T, typename K>
void IntervalTree<T, K>::ClearRecursive(IntervalNodeType*& node)
{
  if (node)
  {
    // Clear left node
    ClearRecursive(node->S_left);

    // Clear this node
    node->S_centerByStart.clear();
    node->S_centerByFinish.clear();

    // Clear right node
    ClearRecursive(node->S_right);

    delete node;
    node = NULL;
  }
}

template <class T, typename K>
size_t IntervalTree<T, K>::Size(void) const
{
  return SizeRecursive(m_root);
}

template <class T, typename K>
size_t IntervalTree<T, K>::SizeRecursive(const IntervalNodeType* node)
{
  size_t count = 0;

  if (node)
  {
    // Count left intervals
    if (node->S_left)
      count += SizeRecursive(node->S_left);

    // Count center intervals
    count += node->S_centerByStart.size();

    // Count right intervals
    if (node->S_right)
      count += SizeRecursive(node->S_right);
  }

  return count;
}

template <class T, typename K>
bool IntervalTree<T, K>::GetIntersecting(K x, IntervalVector& intervals) const
{
  GetIntersectingRecursive(x, intervals, m_root);
  return !intervals.empty();
}

template <class T, typename K>
void IntervalTree<T, K>::GetIntersectingRecursive(K x, IntervalVector& intervals, const IntervalNodeType* node)
{
  if (node)
  {
    if (x < node->x_center)
    {
      GetIntersectingRecursive(x, intervals, node->S_left);

      // All intervals in S_center end after x. Find intervals in node->S_centerByStart
      // that begin before x
      for (typename IntervalNodeType::IntervalSetByStart::const_iterator it = node->S_centerByStart.begin();
           it != node->S_centerByStart.end() && it->x_low <= x; ++it)
      {
        intervals.push_back(*it);
      }
    }
    else if (x == node->x_center)
    {
      intervals.insert(intervals.end(), node->S_centerByStart.begin(), node->S_centerByStart.end());
    }
    else
    {
      assert(node->x_center < x);

      // All intervals in S_center begin before x, find intervals in
      // node->S_centerByFinish that end after x
      for (typename IntervalNodeType::IntervalSetByFinish::const_iterator it = node->S_centerByFinish.begin();
           it != node->S_centerByFinish.end() && it->x_high >= x; ++it)
      {
        intervals.push_back(*it);
      }

      GetIntersectingRecursive(x, intervals, node->S_right);
    }
  }
}

template <class T, typename K>
bool IntervalTree<T, K>::GetIntersecting(const IntervalType& interval, IntervalVector& intervals) const
{
  GetIntersectingButNotEnclosingRecursive(interval, intervals, m_root);

  // Enclosing intervals have not yet been considered, as they have no endpoint
  // inside the input interval. To find these, we pick any point in the input
  // interval and use the algorithm above to find all intervals intersecting
  // that point, taking care to avoid duplicates.
  IntervalVector enclosingIntervals;
  GetIntersectingRecursive(interval.x_low, enclosingIntervals, m_root);

  for (typename IntervalVector::const_iterator it = enclosingIntervals.begin(); it != enclosingIntervals.end(); ++it)
  {
    if (std::find(intervals.begin(), intervals.end(), *it) == intervals.end())
      intervals.push_back(*it);
  }

  return !intervals.empty();
}

template <class T, typename K>
void IntervalTree<T, K>::GetIntersectingButNotEnclosingRecursive(const IntervalType& interval, IntervalVector& intervals, const IntervalNodeType* node)
{
  if (node)
  {
    GetIntersectingButNotEnclosingRecursive(interval, intervals, node->S_left);

    for (typename IntervalNodeType::IntervalSetByStart::const_iterator it = node->S_centerByStart.begin();
        it != node->S_centerByStart.end(); ++it)
    {
      if (interval.Contains(it->x_low) ||
          interval.Contains(it->x_high))
      {
        intervals.push_back(*it);
      }
    }

    GetIntersectingButNotEnclosingRecursive(interval, intervals, node->S_right);
  }
}

template <class T, typename K>
K IntervalTree<T, K>::GetMin(void) const
{
  if (!m_root)
    return K();

  return GetMinRecursive(*m_root);
}

template <class T, typename K>
K IntervalTree<T, K>::GetMinRecursive(const IntervalNodeType& node) const
{
  if (!node.S_left)
    return node.S_centerByStart.begin()->x_low;

  return GetMinRecursive(*node.S_left);
}

template <class T, typename K>
K IntervalTree<T, K>::GetMax(void) const
{
  K x();

  if (!m_root)
    return K();

  return GetMaxRecursive(*m_root);
}

template <class T, typename K>
K IntervalTree<T, K>::GetMaxRecursive(const IntervalNodeType& node) const
{
  if (!node.S_right)
  {
    typename IntervalNodeType::IntervalSetByStart::const_iterator it = node.S_centerByFinish.end();
    --it;
    return it->x_high;
  }

  return GetMinRecursive(*node.S_right);
}

}
