//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// skiplist_index.cpp
//
// Identification: src/index/skiplist_index.cpp
//
// Copyright (c) 2015-17, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include "index/skiplist_index.h"

#include "common/logger.h"
#include "index/index_key.h"
#include "index/scan_optimizer.h"
#include "statistics/stats_aggregator.h"
#include "storage/tuple.h"

namespace peloton {
namespace index {

SKIPLIST_TEMPLATE_ARGUMENTS
SKIPLIST_INDEX_TYPE::SkipListIndex(IndexMetadata *metadata)
    :  // Base class
      Index{metadata},
      // Key "less than" relation comparator
      comparator{},
      // Key equality checker
      equals{} {
  // TODO: Add your implementation here
  return;
}

SKIPLIST_TEMPLATE_ARGUMENTS
SKIPLIST_INDEX_TYPE::~SkipListIndex() {}

/*
 * InsertEntry() - insert a key-value pair into the map
 *
 * If the key value pair already exists in the map, just return false
 */
SKIPLIST_TEMPLATE_ARGUMENTS
bool SKIPLIST_INDEX_TYPE::InsertEntry(
    const storage::Tuple *key,
    ItemPointer *value) {

  std::vector<std::pair<SkipListNode<KeyType> *, SkipListDummyNode<KeyType> *>> result;
  if (container.FindPrevious(key, nullptr, result)) {
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
      new_node = reinterpret_cast<SkipListNode <KeyType> *>(new SkipListLeafNode<KeyType, ValueType>(key, val, nullptr));
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
      // Randomization leading to addition of new levels
      new_node = reinterpret_cast<SkipListNode <KeyType> *>(new SkipListInternalNode<KeyType>(key, down, nullptr));
      root = container.CreateNewLevel(root);
      if (root->GetNext() == nullptr) {
        InsertFirstNodeAtLevel(key, new_node, root);
      } else {
        std::vector<std::pair<SkipListNode<KeyType> *, SkipListDummyNode<KeyType> *>> result;
        container.FindPrevious(key, root, result);
        InsertNodeAtLevel(result[0].first, key, new_node, root);
      }
    }
  }

  return true;
}


SKIPLIST_TEMPLATE_ARGUMENTS
void SKIPLIST_INDEX_TYPE::InsertFirstNodeAtLevel(KeyType key, SkipListNode<KeyType> *new_node, SkipListDummyNode<KeyType> *dummy) {
  assert(new_node && dummy);
  SkipListNode<KeyType> *head = dummy->GetNext();
  new_node->SetNext(head);

  while (!dummy->next_.compare_exchange_weak(head, new_node)) {
    std::vector<std::pair<SkipListNode<KeyType> *, SkipListDummyNode<KeyType> *>> result;
    container.FindNewPrevious(key, dummy, result);
    if (result[0].first != nullptr) {
      InsertNodeAtLevel(result[0].first, key, new_node, dummy);
      return;
    }
    head = dummy->GetNext();
    new_node->SetNext(head);
  }
}

SKIPLIST_TEMPLATE_ARGUMENTS
void SKIPLIST_INDEX_TYPE::InsertNodeAtLevel(SkipListNode<KeyType> *prev, KeyType key, SkipListNode<KeyType> *new_node, SkipListDummyNode<KeyType> *dummy) {
  assert(prev && new_node && dummy);
  SkipListNode<KeyType> *next = prev->GetNext();

  while (!prev->next_.compare_exchange_weak(next, new_node)) {
    std::vector<std::pair<SkipListNode<KeyType> *, SkipListDummyNode<KeyType> *>> result;
    container.FindNewPrevious(key, dummy, result);
    if (result[0].first == nullptr) {
      InsertFirstNodeAtLevel(key, new_node, dummy);
      return;
    }
    next = prev->GetNext();
    new_node->SetNext(next);
  }
}


/*
 * DeleteEntry() - Removes a key-value pair
 *
 * If the key-value pair does not exists yet in the map return false
 */
SKIPLIST_TEMPLATE_ARGUMENTS
bool SKIPLIST_INDEX_TYPE::DeleteEntry(
    UNUSED_ATTRIBUTE const storage::Tuple *key,
    UNUSED_ATTRIBUTE ItemPointer *value) {
  bool ret = false;
  // TODO: Add your implementation here
  return ret;
}

SKIPLIST_TEMPLATE_ARGUMENTS
bool SKIPLIST_INDEX_TYPE::CondInsertEntry(
    UNUSED_ATTRIBUTE const storage::Tuple *key,
    UNUSED_ATTRIBUTE ItemPointer *value,
    UNUSED_ATTRIBUTE std::function<bool(const void *)> predicate) {
  bool ret = false;
  // TODO: Add your implementation here
  return ret;
}

/*
 * Scan() - Scans a range inside the index using index scan optimizer
 *
 */
SKIPLIST_TEMPLATE_ARGUMENTS
void SKIPLIST_INDEX_TYPE::Scan(
    UNUSED_ATTRIBUTE const std::vector<type::Value> &value_list,
    UNUSED_ATTRIBUTE const std::vector<oid_t> &tuple_column_id_list,
    UNUSED_ATTRIBUTE const std::vector<ExpressionType> &expr_list,
    UNUSED_ATTRIBUTE ScanDirectionType scan_direction,
    UNUSED_ATTRIBUTE std::vector<ValueType> &result,
    UNUSED_ATTRIBUTE const ConjunctionScanPredicate *csp_p) {
  // TODO: Add your implementation here
  return;
}

/*
 * ScanLimit() - Scan the index with predicate and limit/offset
 *
 */
SKIPLIST_TEMPLATE_ARGUMENTS
void SKIPLIST_INDEX_TYPE::ScanLimit(
    UNUSED_ATTRIBUTE const std::vector<type::Value> &value_list,
    UNUSED_ATTRIBUTE const std::vector<oid_t> &tuple_column_id_list,
    UNUSED_ATTRIBUTE const std::vector<ExpressionType> &expr_list,
    UNUSED_ATTRIBUTE ScanDirectionType scan_direction,
    UNUSED_ATTRIBUTE std::vector<ValueType> &result,
    UNUSED_ATTRIBUTE const ConjunctionScanPredicate *csp_p,
    UNUSED_ATTRIBUTE uint64_t limit, UNUSED_ATTRIBUTE uint64_t offset) {
  // TODO: Add your implementation here
  return;
}

SKIPLIST_TEMPLATE_ARGUMENTS
void SKIPLIST_INDEX_TYPE::ScanAllKeys(
    UNUSED_ATTRIBUTE std::vector<ValueType> &result) {
  // TODO: Add your implementation here
  return;
}

SKIPLIST_TEMPLATE_ARGUMENTS
void SKIPLIST_INDEX_TYPE::ScanKey(
    UNUSED_ATTRIBUTE const storage::Tuple *key,
    UNUSED_ATTRIBUTE std::vector<ValueType> &result) {
  // TODO: Add your implementation here
  return;
}

SKIPLIST_TEMPLATE_ARGUMENTS
std::string SKIPLIST_INDEX_TYPE::GetTypeName() const { return "SkipList"; }

// IMPORTANT: Make sure you don't exceed CompactIntegerKey_MAX_SLOTS

template class SkipListIndex<
    CompactIntsKey<1>, ItemPointer *, CompactIntsComparator<1>,
    CompactIntsEqualityChecker<1>, ItemPointerComparator>;
template class SkipListIndex<
    CompactIntsKey<2>, ItemPointer *, CompactIntsComparator<2>,
    CompactIntsEqualityChecker<2>, ItemPointerComparator>;
template class SkipListIndex<
    CompactIntsKey<3>, ItemPointer *, CompactIntsComparator<3>,
    CompactIntsEqualityChecker<3>, ItemPointerComparator>;
template class SkipListIndex<
    CompactIntsKey<4>, ItemPointer *, CompactIntsComparator<4>,
    CompactIntsEqualityChecker<4>, ItemPointerComparator>;

// Generic key
template class SkipListIndex<GenericKey<4>, ItemPointer *,
                             FastGenericComparator<4>,
                             GenericEqualityChecker<4>, ItemPointerComparator>;
template class SkipListIndex<GenericKey<8>, ItemPointer *,
                             FastGenericComparator<8>,
                             GenericEqualityChecker<8>, ItemPointerComparator>;
template class SkipListIndex<GenericKey<16>, ItemPointer *,
                             FastGenericComparator<16>,
                             GenericEqualityChecker<16>, ItemPointerComparator>;
template class SkipListIndex<GenericKey<64>, ItemPointer *,
                             FastGenericComparator<64>,
                             GenericEqualityChecker<64>, ItemPointerComparator>;
template class SkipListIndex<
    GenericKey<256>, ItemPointer *, FastGenericComparator<256>,
    GenericEqualityChecker<256>, ItemPointerComparator>;

// Tuple key
template class SkipListIndex<TupleKey, ItemPointer *, TupleKeyComparator,
                             TupleKeyEqualityChecker, ItemPointerComparator>;

}  // namespace index
}  // namespace peloton
