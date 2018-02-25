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
#include <atomic>
#include <stdlib.h>
#include <time.h>
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
  SkipListNode(KeyType k, SkipListNode<KeyType> *next, SkipListNodeType type) : next_(next), key_(k), is_deleted_(false), type_(type) {}

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
  // TODO: make this private again. Public to get rid of compilation error
  std::atomic<SkipListNode<KeyType> *> next_;

private:
  KeyType key_;

  bool is_deleted_; // Deprecated. We will not use this anymore. We will flip one of the bits in the next pointer of the node to mark it deleted. Thus failing CAS as well.
  SkipListNodeType type_; // Might not be needed. Good to have tho.
};



template <typename KeyType, typename ValueType>
class SkipListLeafNode : SkipListNode<KeyType> {
public:
  SkipListLeafNode(KeyType k, ValueType v, SkipListNode<KeyType> *next) : SkipListNode<KeyType>(k, next, SkipListNodeType::LEAF_NODE), val_(v) {}

  inline ValueType GetVal() {
    return val_;
  }

private:
  ValueType val_;
};



template <typename KeyType>
class SkipListInternalNode : SkipListNode<KeyType> {
public:
  SkipListInternalNode(KeyType k, SkipListNode<KeyType> *down, SkipListNode<KeyType> *next) : SkipListNode<KeyType>(k, next, SkipListNodeType::INTERNAL_NODE), down_(down) {}


  inline SkipListNode<KeyType> *GetDown() {
    return down_;
  }

private:
  SkipListNode<KeyType> *down_;
};



template <typename KeyType>
class SkipListDummyNode {
public:
  SkipListDummyNode(SkipListNode<KeyType> *next, SkipListDummyNode<KeyType> *down) {
    next_ = next;
    down_ = down;
  }

  SkipListNode<KeyType> *GetNext() {
    return next_;
  }

  SkipListDummyNode<KeyType> *GetDown() {
    return down_;
  }

  void SetNext(SkipListNode<KeyType> *next) {
    next_ = next;
  }

  void SetDown(SkipListDummyNode<KeyType> *down) {
    down_ = down;
  }
  // TODO: make this private again. Public to get rid of compilation error
  std::atomic<SkipListNode<KeyType> *> next_;

private:
  SkipListDummyNode<KeyType> *down_;

};



template <typename KeyType, typename ValueType, typename KeyComparator,
          typename KeyEqualityChecker, typename ValueEqualityChecker>
class SkipList {
  friend class SkipListDummyNode<KeyType>;
  friend class SkipListNode<KeyType>;


public:
  SkipList(KeyComparator key_cmp_obj, KeyEqualityChecker key_eq_check_obj) : key_cmp_obj_(key_cmp_obj), key_eq_check_obj_(key_eq_check_obj) {
    root_ = new SkipListDummyNode<KeyType>(nullptr, nullptr);
  }

  inline SkipListDummyNode<KeyType> *GetRoot() {
    return root_;
  }
  // NOTE: https://stackoverflow.com/questions/10632251/undefined-reference-to-template-function
  // Implementation in .h file due to linker error
  bool Insert(const KeyType &key, const ValueType &value) {
    std::vector<std::pair<SkipListNode<KeyType> *, SkipListDummyNode<KeyType> *>> result;
    if (FindPrevious(key, nullptr, result)) {
      return false;
    }

    // Not a duplicate, start insert
    int index = result.size() - 1;
    srand(time(0));
    SkipListNode<KeyType> *down = nullptr;
    bool finish = false;

    for (int i = index; i >= 0; i--) {

      // Go through the result bottom up
      SkipListNode<KeyType> *new_node;

      if (i == index) {
        // Leaf node
        new_node = reinterpret_cast<SkipListNode <KeyType> *>(new SkipListLeafNode<KeyType, ValueType>(key, value, nullptr));
        down = new_node;
      } else {
        // Internal node. randomization and break if fail
        int random_val = rand() % 2;
        if (random_val == 0) {
          finish = true;
          break;
        }
        new_node = reinterpret_cast<SkipListNode <KeyType> *>(new SkipListInternalNode<KeyType>(key, down, nullptr));
        down = new_node;
      }

      if (result[i].first == nullptr) {
        // New node has to be inserted right after the dummy
        InsertFirstNodeAtLevel(key, new_node, result[i].second);
      } else {
        // New node has to be inserted after result[i].first
        InsertNodeAtLevel(result[i].first, key, new_node, result[i].second);
      }
    }

    SkipListDummyNode<KeyType> *root = result[0].second;

    if (!finish) {
      while (rand() % 2) {
        SkipListNode<KeyType> *new_node;
        // Randomization leading to addition of new levels
        new_node = reinterpret_cast<SkipListNode <KeyType> *>(new SkipListInternalNode<KeyType>(key, down, nullptr));
        root = CreateNewLevel(root);
        if (root->GetNext() == nullptr) {
          InsertFirstNodeAtLevel(key, new_node, root);
        } else {
          std::vector<std::pair<SkipListNode<KeyType> *, SkipListDummyNode<KeyType> *>> result;
          FindPrevious(key, root, result);
          InsertNodeAtLevel(result[0].first, key, new_node, root);
        }
      }
    }

    return true;
  }
  /*
   * FindPrevious() - Finds the previous node or the node of insertion for a given key
   *
   * Starts from the root_ dummy node and goes top-bottom finding the node with key less
   * than the key in the argument. If the second argument dummy node is not null, we only
   * want to know the previous node at that level. If it is nullptr, we find previous nodes
   * at all levels. Result stores the previous pointer along with the dummy node for that level
   * respectively. If the key is present in the skiplist, we return true. Otherwise, false
   */
  bool FindPrevious(KeyType key, SkipListDummyNode<KeyType> *dummy, std::vector<std::pair<SkipListNode<KeyType> *, SkipListDummyNode<KeyType> *>> &result) {
    SkipListDummyNode<KeyType> *root_cpy = root_, *stop_dummy = nullptr;
    SkipListNode<KeyType> *head = root_cpy->GetNext();
    SkipListNode<KeyType> *prev = nullptr;
    if (nullptr != dummy) {
      stop_dummy = dummy->GetDown();
    }

    while (root_cpy != stop_dummy) {
      while (head) {
        if (head->IsDeleted()) {
          // Skip the deleted nodes
          head = head->GetNext();
        }
        if (key_cmp_obj_(head->GetKey(), key) >= 0) {
          break;
        }
        prev = head;
        head = head->GetNext();
      }
      if (nullptr == dummy) {
        result.push_back(std::make_pair(prev, root_cpy));
      }
      if (nullptr == prev) {
        head = root_cpy->GetDown()->GetNext();
      } else {
        if (prev->GetType() == SkipListNodeType::INTERNAL_NODE) {
          head = reinterpret_cast<SkipListInternalNode<KeyType> *>(prev)->GetDown();
        } else {
          head = nullptr;
        }
      }
      root_cpy = root_cpy->GetDown();
      prev = nullptr;
    }
    if (prev && key_eq_check_obj_(key, prev->GetKey())) {
      return true;
    }
    if (dummy) {
      // go through the head and find on that node
      result.push_back(std::make_pair(prev, dummy));
    }
    return false;
  }
  /*
   * CreateNewLevel() - create a new level for the skip list
   *
   * Returns a pointer to the new root with (AT LEAST) one extra level than sent via the root argument.
   * If another thread already has updated the new root_ by adding one or more levels, we traverse from
   * the updated root_ to find the dummy pointer for the level just above that represented by the argument
   * root
   */
  SkipListDummyNode<KeyType> *CreateNewLevel(SkipListDummyNode<KeyType> *root) {
    SkipListDummyNode<KeyType> *new_root = new SkipListDummyNode<KeyType>(nullptr, root);
    if (root_.compare_exchange_weak(root, new_root)) {
      return new_root;
    }
    delete new_root;
    new_root = root_;
    while (new_root->GetDown() != root) {
      new_root = new_root->GetDown();
    }
    return new_root;
  }

  void InsertFirstNodeAtLevel(KeyType key, SkipListNode<KeyType> *new_node, SkipListDummyNode<KeyType> *dummy) {
    assert(new_node && dummy);
    SkipListNode<KeyType> *head = dummy->GetNext();
    new_node->SetNext(head);

    while (!dummy->next_.compare_exchange_weak(head, new_node)) {
      std::vector<std::pair<SkipListNode<KeyType> *, SkipListDummyNode<KeyType> *>> result;
      FindPrevious(key, dummy, result);
      if (result[0].first != nullptr) {
        InsertNodeAtLevel(result[0].first, key, new_node, dummy);
        return;
      }
      head = dummy->GetNext();
      new_node->SetNext(head);
    }
  }

  void InsertNodeAtLevel(SkipListNode<KeyType> *prev, KeyType key, SkipListNode<KeyType> *new_node, SkipListDummyNode<KeyType> *dummy) {
    assert(prev && new_node && dummy);
    SkipListNode<KeyType> *next = prev->GetNext();

    while (!prev->next_.compare_exchange_weak(next, new_node)) {
      std::vector<std::pair<SkipListNode<KeyType> *, SkipListDummyNode<KeyType> *>> result;
      FindPrevious(key, dummy, result);
      if (result[0].first == nullptr) {
        InsertFirstNodeAtLevel(key, new_node, dummy);
        return;
      }
      next = prev->GetNext();
      new_node->SetNext(next);
    }
  }

private:
  // Root Dummy Node. On addition of levels, the root dummy node gets updated as the latest dummy node to be added.
  std::atomic<SkipListDummyNode<KeyType> *> root_;
  const KeyComparator key_cmp_obj_;
  const KeyEqualityChecker key_eq_check_obj_;
};

}  // namespace index
}  // namespace peloton
