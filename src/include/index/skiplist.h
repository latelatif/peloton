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

#pragma once

#include <iostream>

namespace peloton {
namespace index {

/*
 * SKIPLIST_TEMPLATE_ARGUMENTS - Save some key strokes
 */
#define SKIPLIST_TEMPLATE_ARGUMENTS                                       \
  template <typename KeyType, typename ValueType, typename KeyComparator, \
            typename KeyEqualityChecker, typename ValueEqualityChecker>
#define SKIPLISTNODE_TYPE SkipListNode<KeyType>
#define SKIPLISTLEAFNODE_TYPE SkipListLeafNode<KeyType, ValueType>
#define SKIPLISTHEADNODE_TYPE SkipListHeadNode<KeyType>

template <typename KeyType>
class SkipListNode {
public:
  SkipListNode(KeyType key, SkipListNode<KeyType> *right, 
               SkipListNode<KeyType> *down) : key_(key), right_(right),
               down_(down){}

  inline KeyType GetKey() {
    return key_;
  }

  inline SkipListNode<KeyType> *GetRight() {
    return right_;
  }

  inline void SetRight(SkipListNode<KeyType> *right) {
    right_ = right;
  }

private:
  KeyType key_;
  SkipListNode<KeyType> *right_;
  SkipListNode<KeyType> *down_;
  SkipListNode<KeyType> *tower_root_;
  SkipListNode<KeyType> *back_link_;
};

template <typename KeyType, typename ValueType>
class SkipListLeafNode : SkipListNode<KeyType> {
public:
  SkipListLeafNode(KeyType key, SkipListNode<KeyType> *right,
                   SkipListNode<KeyType> *down, ValueType value) : SkipListNode<KeyType>(key, right, down), value_(value) {}

  inline ValueType GetValue() {
    return value_;
  }

private:
  ValueType value_;
};

template <typename KeyType>
class SkipListHeadNode : SkipListNode<KeyType> {
public:
  SkipListHeadNode(KeyType key, SkipListNode<KeyType> *right,
                   SkipListNode<KeyType> *down, SkipListHeadNode<KeyType> *up) : SkipListNode<KeyType>(key, right, down),
    up_(up) {}

private:
  SkipListHeadNode<KeyType> *up_;
};

template <typename KeyType, typename ValueType, typename KeyComparator,
          typename KeyEqualityChecker, typename ValueEqualityChecker>
class SkipList {
public:
  enum class StatusType {
    INVALID = 0,
    to_be_named1 = 1,
    to_be_named2 = 2,
  };
private:

  using NodeNodePair = std::pair<SKIPLISTNODE_TYPE *, SKIPLISTNODE_TYPE *>;
  using NodeLevelPair = std::pair<SKIPLISTNODE_TYPE *, int>;
  using NodeStatusResultTuple = std::tuple<SKIPLISTNODE_TYPE *, StatusType, bool>;

public:


  SkipList(KeyComparator key_cmp_obj, KeyEqualityChecker key_eq_check_obj) : key_cmp_obj_(key_cmp_obj), key_eq_check_obj_(key_eq_check_obj) {}

    SKIPLISTLEAFNODE_TYPE *Search(const KeyType &key) {
    NodeNodePair node_node = SearchToLevel(key, 1);
    SKIPLISTLEAFNODE_TYPE *curr_node = reinterpret_cast<SKIPLISTLEAFNODE_TYPE *>(node_node.first);
    if (key_eq_check_obj_(curr_node->key, key)) {
      return curr_node;
    } else {
      return nullptr;
    }
  }

  NodeNodePair SearchToLevel(const KeyType &key, int level) {
    NodeLevelPair node_level = FindStart(level);
    SKIPLISTNODE_TYPE *curr_node = node_level.first;
    NodeNodePair node_node = nullptr;
    int curr_level = node_level.second;
    while (curr_level > level) { // search down to level v + 1
      node_node = SearchRight(key, curr_node);
      curr_node = node_node.first;
      curr_node = curr_node->down;
      curr_level--;
    }
    node_node = SearchRight(key, curr_node);
    return node_node;
  }

  NodeLevelPair FindStart(int level) {
    SKIPLISTHEADNODE_TYPE *curr_node = reinterpret_cast<SKIPLISTHEADNODE_TYPE *>(head_);
    int curr_level = 1;
    while ((curr_node->up->right != nullptr) || (curr_level < level)) {
      curr_node = curr_node->up;
      curr_level++;
    }
    return make_pair(curr_node, curr_level);
  }

//  NodeNodePair SearchRight(UNUSED_ATTRIBUTE const KeyType &key, UNUSED_ATTRIBUTE SKIPLISTNODE_TYPE *curr_node) {
//    // To be implemented
//  }
//
//  NodeStatusResultTuple TryFlagNode(UNUSED_ATTRIBUTE SKIPLISTNODE_TYPE *prev_node,
//                                    UNUSED_ATTRIBUTE SKIPLISTNODE_TYPE *target_node) {
//    // To be implemented
//  }
//
//  NodeNodePair InsertNode(UNUSED_ATTRIBUTE SKIPLISTNODE_TYPE *newNode, UNUSED_ATTRIBUTE SKIPLISTNODE_TYPE *prev_node,
//                          UNUSED_ATTRIBUTE SKIPLISTNODE_TYPE *next_node) {
//    // To be implemented
//  }
//
//  SKIPLISTNODE_TYPE *DeleteNode(UNUSED_ATTRIBUTE SKIPLISTNODE_TYPE *prev_node, UNUSED_ATTRIBUTE SKIPLISTNODE_TYPE *del_node) {
//    // To be implemented
//  }
//
//  void HelpMarked(UNUSED_ATTRIBUTE SKIPLISTNODE_TYPE *prev_node, UNUSED_ATTRIBUTE SKIPLISTNODE_TYPE *del_node) {
//    // To be implemented
//  }
//
//  void HelpFlagged(UNUSED_ATTRIBUTE SKIPLISTNODE_TYPE *prev_node, UNUSED_ATTRIBUTE SKIPLISTNODE_TYPE *del_node) {
//    // To be implemented
//  }
//
//  void TryMark(UNUSED_ATTRIBUTE SKIPLISTNODE_TYPE *del_node) {
//    // To be implemented
//  }

  inline SKIPLISTNODE_TYPE *GetHead() {
    return head_;
  } 

private:

  SKIPLISTNODE_TYPE *head_;
  const KeyComparator key_cmp_obj_;
  const KeyEqualityChecker key_eq_check_obj_;
};

}  // namespace index
}  // namespace peloton
