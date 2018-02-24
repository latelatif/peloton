//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// skiplist.cpp
//
// Identification: src/index/skiplist.cpp
//
// Copyright (c) 2015-17, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "index/skiplist.h"
#include <vector>

namespace peloton {
namespace index {

/*
 * FindPrevious() - Finds the previous node or the node of insertion for a given key
 *
 * Starts from the root_ dummy node and goes top-bottom finding the node with key less
 * than the key in the argument. If the second argument dummy node is not null, we only
 * want to know the previous node at that level. If it is nullptr, we find previous nodes
 * at all levels. Result stores the previous pointer along with the dummy node for that level
 * respectively. If the key is present in the skiplist, we return true. Otherwise, false
 */

template <typename KeyType, typename ValueType, typename KeyComparator,
      typename KeyEqualityChecker, typename ValueEqualityChecker>
bool SkipList::FindPrevious(KeyType key, SkipListDummyNode<KeyType> *dummy,
                               std::vector<std::pair<SkipListNode<KeyType> *, SkipListDummyNode<KeyType> *>> &result) {

  SkipListNode<KeyType> *head = root_->GetNext(), *prev = nullptr;
  SkipListDummyNode<KeyType> *root_cpy = root_;

  while (root_cpy != dummy) {
    while (head) {
      if (head->IsDeleted()) {
        // Skip the deleted nodes
        head = head->GetNext();
      }
      if (KeyComparator(key, head->GetKey()) >= 0) {
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
      head = prev->GetDown();
    }
    root_cpy = root_cpy->GetDown();
    prev = nullptr;
  }
  if (prev && KeyEqualityChecker(key, prev->GetKey())) {
    return true;
  }
  if (dummy) {
    result.push_back(std::make_pair(prev, root_cpy));
  }
  return false;
};


/*
 * CreateNewLevel() - create a new level for the skip list
 *
 * Returns a pointer to the new root with (AT LEAST) one extra level than sent via the root argument.
 * If another thread already has updated the new root_ by adding one or more levels, we traverse from
 * the updated root_ to find the dummy pointer for the level just above that represented by the argument
 * root
 */

template <typename KeyType, typename ValueType, typename KeyComparator,
    typename KeyEqualityChecker, typename ValueEqualityChecker>
SkipListDummyNode<KeyType> *SkipList::CreateNewLevel(SkipListDummyNode<KeyType> *root) {
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
};

}  // namespace index
}  // namespace peloton
