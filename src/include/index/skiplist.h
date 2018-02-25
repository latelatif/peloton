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

namespace peloton {
namespace index {

/*
 * SKIPLIST_TEMPLATE_ARGUMENTS - Save some key strokes
 */
#define SKIPLIST_TEMPLATE_ARGUMENTS                                       \
  template <typename KeyType, typename ValueType, typename KeyComparator, \
            typename KeyEqualityChecker, typename ValueEqualityChecker>

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
  SkipListLeafNode(KeyType key, ValueType value, SkipListNode<KeyType> *next, 
                   SkipListNode<KeyType> *down) : key_(key), value_(value),
                   right_(right), down_(down) {}

  inline ValueType GetValue() {
    return val_;
  }

private:
  ValueType value_;
};

template <typename KeyType>
class SkipListHeadNode : SkipListNode<KeyType> {
public:
  SkipListLeafNode(KeyType key, ValueType value, SkipListNode<KeyType> *next, 
                   SkipListNode<KeyType> *down) : key_(key), value_(value),
                   right_(right), down_(down), up_(up) {}

private:
  SkipListNode<KeyType> *up_;
};

template <typename KeyType, typename ValueType, typename KeyComparator,
          typename KeyEqualityChecker, typename ValueEqualityChecker>
class SkipList {
  friend class SkipListDummyNode<KeyType>;
  friend class SkipListNode<KeyType>;

  using NodeNodePair = std::pair<SkipListNode *, SkipListNode *>;
  using NodeLevelPair = std::pair<SkipListNode *, int>;
  using NodeStatusResultTuple std::tuple<SkipListNode *, StatusType, bool>

public:
  enum class StatusType {
    INVALID = 0,
    to_be_named1 = 1,
    to_be_named2 = 2,
  };

  SkipList(KeyComparator key_cmp_obj, KeyEqualityChecker key_eq_check_obj) : key_cmp_obj_(key_cmp_obj), key_eq_check_obj_(key_eq_check_obj) {}

  SkipListLeafNode *Search(const KeyType &key) {
    NodeNodePair node_node = SearchToLevel(key, 1);
    SkipListLeafNode *curr_node = reinterpret_cast<SkipListLeafNode *>(node_node.first);
    if (key_eq_check_obj_(curr_node->key, key)) {
      return curr_node;
    } else {
      return nullptr;
    }
  }

  NodeNodePair SearchToLevel(const KeyType &key, int level) {
    NodeLevelPair node_level = FindStart(level);
    SkipListNode *curr_node = node_level.first;
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
    SkipListHeadNode curr_node = reinterpret_cast<SkipListHeadNode *>(head_)
    int curr_level = 1;
    while ((curr_node->up->right != nullptr) || (curr_level < level)) {
      curr_node = curr_node->up;
      curr_level++;
    }
    return make_pair(curr_node, curr_level);
  }

  NodeNodePair SearchRight(const KeyType &key, SkipListNode *curr_node) {
    // To be implemented
  }

  NodeStatusResultTuple TryFlagNode(SkipListNode *prev_node,
                                    SkipListNode *target_node) {
    // To be implemented
  }

  NodeNodePair InsertNode(SkipListNode *newNode, SkipListNode *prev_node,
                          SkipListNode *next_node) {
    // To be implemented
  }

  SkipListNode *DeleteNode(SkipListNode *prev_node, SkipListNode *del_node) {
    // To be implemented
  }

  void HelpMarked(SkipListNode *prev_node, SkipListNode *del_node) {
    // To be implemented
  }

  void HelpFlagged(SkipListNode *prev_node, SkipListNode *del_node) {
    // To be implemented
  }

  void TryMark(SkipListNode *del_node) {
    // To be implemented
  }

  inline SkipListNode<KeyType> *GetHead() {
    return head_;
  } 

private:
  SkipListNode<KeyType> *head_;
  const KeyComparator key_cmp_obj_;
  const KeyEqualityChecker key_eq_check_obj_;
};

}  // namespace index
}  // namespace peloton
