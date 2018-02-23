//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// skiplist.h
//
// Identification: src/include/index/skiplist.h
//
// Copyright (c) 2015-17, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <vector>
#pragma once

namespace peloton {
namespace index {

/*
 * SKIPLIST_TEMPLATE_ARGUMENTS - Save some key strokes
 */
#define SKIPLIST_TEMPLATE_ARGUMENTS                                       \
  template <typename KeyType, typename ValueType, typename KeyComparator, \
            typename KeyEqualityChecker, typename ValueEqualityChecker>

enum class SkipListNodeType {
  INTERNAL_NODE,
  LEAF_NODE
};

template <typename KeyType>
class SkipListNode {
public:
  SkipListNode(KeyType k, SkipListNode<KeyType> *next, SkipListNodeType type) : key_(k), next_(next), is_deleted_(false), type_(type) {}
  inline KeyType GetKey() {
    return key_;
  }
  inline SkipListNode<KeyType> *GetNext() {
    return next_;
  }
  inline void SetNext(SkipListNode<KeyType> *next) {
    next_ = next;
  }
  inline bool IsDeleted() {
    return is_deleted_;
  }
  inline SkipListNodeType GetType() {
    return type_;
  }
private:
  KeyType key_;
  SkipListNode<KeyType> *next_;
  bool is_deleted_;
  SkipListNodeType type_;
};

template <typename KeyType, typename ValueType>
class SkipListLeafNode : SkipListNode {
public:
  SkipListLeafNode(KeyType k, ValueType v, SkipListNode<KeyType> *next) : SkipListNode(k, next, SkipListNodeType::LEAF_NODE), val_(v) {}
  inline ValueType GetVal() {
    return val_;
  }
private:
  ValueType val_;
};

template <typename KeyType>
class SkipListInternalNode : SkipListNode {
public:
  SkipListInternalNode(KeyType k, SkipListNode<KeyType> *down, SkipListNode<KeyType> *next) : SkipListNode(k, next, SkipListNodeType::INTERNAL_NODE), down_(down) {}
  inline SkipListNode<KeyType> *GetDown() {
    return down_;
  }
private:
  SkipListNode<KeyType> *down_;
};

template <typename KeyType, typename ValueType, typename KeyComparator,
          typename KeyEqualityChecker, typename ValueEqualityChecker>
class SkipList {
  // TODO: Add your declarations here
  inline std::vector<SkipListNode<KeyType> *> &GetLists() {
    return lists_;
  }
  bool Find(KeyType &k, std::vector<SkipListNode<KeyType> *> &result);
private:
  std::vector<SkipListNode<KeyType> *> lists_;
};

}  // namespace index
}  // namespace peloton
