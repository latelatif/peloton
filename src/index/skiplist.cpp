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

template <typename KeyType, typename ValueType, typename KeyComparator,
      typename KeyEqualityChecker, typename ValueEqualityChecker>
bool SkipList::Find(KeyType &key, std::vector<SkipListNode<KeyType> *> &result) {

  bool found = false;
  size_t index = lists_.GetSize() - 1;
  SkipListNode<KeyType> *head = lists_[index];
  SkipListNode<KeyType> *prev = nullptr;
  while (head && head->GetType() == SkipListNodeType::INTERNAL_NODE) {
    prev = nullptr;
    while (head) {
      if (KeyComparator(key, head->GetKey()) > 0) {
        break;
      }
      prev = head;
      head = head->GetNext();
    }
    if (nullptr == prev) {
      head = lists_[index - 1];
    } else {
      result.push_back(prev);
      head = prev->GetDown();
    }
    --index;
  }
  prev = nullptr;
  while (head) {
    if (KeyComparator(key, head->GetKey()) > 0) {
      break;
    }
    prev = head;
    head = head->GetNext();
  }
  if (prev && KeyEqualityChecker(key, prev->GetKey())) {
    return true;
  }
  result.push_back(prev);
  return false;
}

}  // namespace index
}  // namespace peloton
