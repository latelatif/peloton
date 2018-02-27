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


// TODO(gandeevan): might want to change the scan functions to an iterator model

#pragma once

#include <iostream>
#include <tuple>
#include "common/logger.h"
#include "index/index.h"
#include "executor/delete_executor.h"

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

#define IS_WORD_ALIGNED(x) (((long)x%4)==0)
#define MAX_LEVEL 1000

template <typename KeyType>


class SkipListNode {
public:

  /*
   * The tower in which the node belongs.
   */
  enum class TowerType {
      HEAD_TOWER = 0,
      MIDDLE_TOWER = 1,
      //TAIL_TOWER =2,
  };

  enum class TraversalMode {
      GO_DOWN_ON_LT = 0,
      GO_DOWN_ON_LEQ = 1,
  };

  SkipListNode(KeyType key, TowerType tower_type,
               SKIPLISTNODE_TYPE *succ,
               SKIPLISTNODE_TYPE *down, SKIPLISTNODE_TYPE *tower_root)
               : key_(key), tower_type_(tower_type),
                 succ_(succ),  down_(down), tower_root_(tower_root){}

  SkipListNode(TowerType tower_type,
               SKIPLISTNODE_TYPE *succ,
               SKIPLISTNODE_TYPE *down, SKIPLISTNODE_TYPE *tower_root)
               :  tower_type_(tower_type),
                  succ_(succ),  down_(down), tower_root_(tower_root){}

  inline KeyType GetKey() {
    return key_;
  }

  inline SKIPLISTNODE_TYPE *GetSucc(){
    return succ_;
  }

  inline SKIPLISTNODE_TYPE **GetSuccPtr(){
    return &succ_;
  }

  inline SKIPLISTNODE_TYPE *GetRight() {
    if(succ_== nullptr)
      return nullptr;

    return (SKIPLISTNODE_TYPE* )((long)succ_ & (~(0x3L)));
  }


  static inline SKIPLISTNODE_TYPE *DoMark(SKIPLISTNODE_TYPE *node){
    return (SKIPLISTNODE_TYPE *)((long)node | 1L);
  }

  static inline bool IsMarkedReference(SKIPLISTNODE_TYPE *node){
    return (((long)node & 1L)==1);
  }

  static inline SKIPLISTNODE_TYPE *DoFlag(SKIPLISTNODE_TYPE *node){
    return (SKIPLISTNODE_TYPE *)((long)node | 2L);
  }

  static inline bool IsFlagged(SKIPLISTNODE_TYPE *node){
    return (((long)node & 2L)==2);
  }

  inline TowerType GetTowerType(){
    return tower_type_;
  }

  inline void SetTowerType(TowerType tower_type){
    tower_type_ = tower_type;
  }

  inline SKIPLISTNODE_TYPE *GetTowerRoot(){
    return tower_root_;
  }

  inline void SetTowerRoot(SKIPLISTNODE_TYPE *tower_root){
    tower_root_ = tower_root;
  }



  inline void SetSucc(SKIPLISTNODE_TYPE *succ) {
    succ_ = succ;
  }

  inline SKIPLISTNODE_TYPE *GetBackLink(){
    return back_link_;
  }

  inline void SetBackLink(SKIPLISTNODE_TYPE *back_link){
    back_link_ = back_link;
  }

  inline SKIPLISTNODE_TYPE *GetDown(){
    return down_;
  }

  inline void SetDown(SKIPLISTNODE_TYPE *down){
    down_ = down;
  }



private:
  KeyType key_;
  TowerType tower_type_;
  SKIPLISTNODE_TYPE *succ_;
  SKIPLISTNODE_TYPE *down_;
  SKIPLISTNODE_TYPE *tower_root_;
  SKIPLISTNODE_TYPE *back_link_;


};

template <typename KeyType, typename ValueType>
class SkipListLeafNode : public SKIPLISTNODE_TYPE {
public:
  SkipListLeafNode(KeyType key, typename SKIPLISTNODE_TYPE::TowerType tower_type,
                   SKIPLISTNODE_TYPE *succ, ValueType value)
                    : SKIPLISTNODE_TYPE(key, tower_type, succ, nullptr, nullptr),
                      value_(value) {}

  inline ValueType GetValue() {
    return value_;
  }

private:
  ValueType value_;
};

template <typename KeyType>
class SkipListHeadNode : public SKIPLISTNODE_TYPE {
public:
  SkipListHeadNode(SKIPLISTNODE_TYPE *succ, SKIPLISTHEADNODE_TYPE *down,
                   SKIPLISTHEADNODE_TYPE *up, SKIPLISTNODE_TYPE *tower_root, int level)
                    : SKIPLISTNODE_TYPE (SKIPLISTNODE_TYPE::TowerType::HEAD_TOWER,
                                          succ,
                                          reinterpret_cast<SKIPLISTNODE_TYPE *>(down),
                                          tower_root),
                      level_(level), up_(up) {}

  inline SKIPLISTHEADNODE_TYPE *GetUp(){
    return up_;
  }

  inline void SetUp(SKIPLISTHEADNODE_TYPE *up){
    up_ = up;
  }

  inline int GetLevel(){
    return level_;
  }

private:
  int level_;
  SkipListHeadNode<KeyType> *up_;
};


template <typename KeyType, typename ValueType, typename KeyComparator,
          typename KeyEqualityChecker, typename ValueEqualityChecker>
class SkipList {
public:
  enum class StatusType {
    INVALID = 0,
    FLAGGED = 1,
    DELETED = 2,
    to_be_named2 = 3,
  };
private:

  using NodeNodePair = std::pair<SKIPLISTNODE_TYPE *, SKIPLISTNODE_TYPE *>;
  using NodeLevelPair = std::pair<SKIPLISTNODE_TYPE *, int>;
  using NodeStatusResultTuple = std::tuple<SKIPLISTNODE_TYPE *, StatusType, bool>;

public:

  SkipList(KeyComparator key_cmp_obj, KeyEqualityChecker key_eq_check_obj, bool is_unique) :
          key_cmp_obj_(key_cmp_obj), key_eq_check_obj_(key_eq_check_obj), is_unique_(is_unique) {
    // InitHeadTower();
    // No tower root needed or set for head tower
    root_ = new SKIPLISTHEADNODE_TYPE(nullptr, nullptr, nullptr, nullptr, 1);
  }

  void InitHeadTower(){

    int curr_level=1;

    SKIPLISTHEADNODE_TYPE *new_head_node = new SKIPLISTHEADNODE_TYPE(nullptr, nullptr, nullptr, nullptr, curr_level);
    SKIPLISTHEADNODE_TYPE  *curr_down = new_head_node;
    head_ = new_head_node;

    new_head_node->SetTowerRoot(head_);

    curr_level++;

    for(; curr_level<=MAX_LEVEL; curr_level++){
      new_head_node = new SKIPLISTHEADNODE_TYPE(nullptr, curr_down, nullptr, head_, curr_level);
      curr_down->SetUp(new_head_node);
      curr_down = new_head_node;
    }
  }

  void PrintSkipListLevel(int level, SKIPLISTNODE_TYPE *head){
    std::cout<<std::endl<<"Level "<<level<<std::endl;
    SKIPLISTNODE_TYPE *node = head->GetRight();

    while(node){
      printf("(key=%s) --> ", node->GetKey().GetInfo().c_str());
      node = node->GetRight();
    }

    std::cout<<"nullptr";

  }

  void PrintSkipList(){
    std::cout<<"SkipList: "<<std::endl;
    SKIPLISTHEADNODE_TYPE *curr_head = reinterpret_cast<SKIPLISTHEADNODE_TYPE *>(head_);

    while(curr_head!=nullptr){
      if(curr_head->GetSucc()== nullptr)
        break;
      PrintSkipListLevel(curr_head->GetLevel(), curr_head);
      curr_head = curr_head->GetUp();
    }
    std::cout<<"\n\n";
    fflush(stdout);

  }

  NodeLevelPair FindStart(int level) {
    SKIPLISTHEADNODE_TYPE *curr_node = reinterpret_cast<SKIPLISTHEADNODE_TYPE *>(head_);
    int curr_level = 1;
    while ((curr_node->GetUp()->GetSucc() != nullptr) || (curr_level < level)) {
      curr_node = curr_node->GetUp();
      curr_level++;
    }
    return std::make_pair(reinterpret_cast<SKIPLISTNODE_TYPE *>(curr_node), curr_level);
  }


  SKIPLISTLEAFNODE_TYPE *Search(const KeyType &key) {
    NodeNodePair node_node = SearchToLevel(key, 1, SKIPLISTNODE_TYPE::TraversalMode::GO_DOWN_ON_LEQ);
    SKIPLISTLEAFNODE_TYPE *curr_node = reinterpret_cast<SKIPLISTLEAFNODE_TYPE *>(node_node.first);

    if(curr_node->GetTowerType()==SKIPLISTNODE_TYPE::TowerType::HEAD_TOWER)
      return nullptr;

    if (key_eq_check_obj_(curr_node->key, key)) {
      return curr_node;
    } else {
      return nullptr;
    }
  }

  NodeNodePair SearchToLevel(const KeyType &key, int level,
    typename SKIPLISTNODE_TYPE::TraversalMode traversal_mode) {

    SKIPLISTNODE_TYPE *curr_node = reinterpret_cast<SKIPLISTNODE_TYPE *>(root_);
    // NOTE: Do not use root_->GetLevel(). There might be a new root being created concurrently
    int curr_level = reinterpret_cast<SKIPLISTHEADNODE_TYPE *>(curr_node)->GetLevel();

    NodeNodePair node_node;

    while (curr_level > level) { // search down to level v + 1
      if(traversal_mode == SKIPLISTNODE_TYPE::TraversalMode::GO_DOWN_ON_LEQ)
        node_node = SearchRightLEQ(key, curr_node);
      else if(traversal_mode == SKIPLISTNODE_TYPE::TraversalMode::GO_DOWN_ON_LT)
        node_node = SearchRightLT(key, curr_node);

      curr_node = node_node.first;
      curr_node = curr_node->GetDown();
      curr_level--;
    }

    if(traversal_mode == SKIPLISTNODE_TYPE::TraversalMode::GO_DOWN_ON_LEQ)
      node_node = SearchRightLEQ(key, curr_node);
    else if(traversal_mode == SKIPLISTNODE_TYPE::TraversalMode::GO_DOWN_ON_LT)
      node_node = SearchRightLT(key, curr_node);
    return node_node;
  }

  void ScanAllKeys(std::vector<ValueType> &result){

    SKIPLISTNODE_TYPE *curr_node = head_;
    SKIPLISTNODE_TYPE *next_node = head_->GetRight();

    while(next_node){

      /* next node has been marked for deletion */
      while(next_node!=nullptr  && SKIPLISTNODE_TYPE::IsMarkedReference(next_node->GetTowerRoot()->GetSucc())){
        NodeStatusResultTuple tuple = TryFlagNode(curr_node, next_node);

        StatusType status = std::get<1>(tuple);

        if(status==StatusType::FLAGGED){
          HelpFlagged(curr_node, next_node);
        }
        next_node = curr_node->GetRight();
      }

      if(next_node){
        SKIPLISTLEAFNODE_TYPE *leaf_node = reinterpret_cast<SKIPLISTLEAFNODE_TYPE *>(next_node);
        result.push_back(leaf_node->GetValue());

        curr_node = next_node;
        next_node = curr_node->GetRight();
      }
    }
  }


  NodeNodePair SearchRightLEQ(const KeyType &key, SKIPLISTNODE_TYPE *curr_node) {

    PL_ASSERT(curr_node);
    PL_ASSERT(IS_WORD_ALIGNED(curr_node));

    SKIPLISTNODE_TYPE *next_node = curr_node->GetRight();

    while(next_node && key_cmp_obj_(next_node->GetKey(), key)<=0){


      /*
       * next_node has been marked for deletion,
       * help with the deletion and then
       * continue traversing.
       */
      while(next_node!=nullptr  && SKIPLISTNODE_TYPE::IsMarkedReference(next_node->GetTowerRoot()->GetSucc())){
        NodeStatusResultTuple tuple = TryFlagNode(curr_node, next_node);

        StatusType status = std::get<1>(tuple);

        if(status==StatusType::FLAGGED){
          HelpFlagged(curr_node, next_node);
        }
        next_node = curr_node->GetRight();
      }

      if(next_node && key_cmp_obj_(next_node->GetKey(), key)<=0){
        curr_node = next_node;
        next_node = curr_node->GetRight();
      }
    }
    return std::make_pair(curr_node, next_node);
  }

  NodeNodePair SearchRightLT(const KeyType &key, SKIPLISTNODE_TYPE *curr_node) {

    PL_ASSERT(curr_node);
    PL_ASSERT(IS_WORD_ALIGNED(curr_node));

    SKIPLISTNODE_TYPE *next_node = curr_node->GetRight();

    while(next_node && key_cmp_obj_(next_node->GetKey(), key)<0){

      /*
       * next_node has been marked for deletion,
       * help with the deletion and then
       * continue traversing.
       */
      while(next_node && SKIPLISTNODE_TYPE::IsMarkedReference(next_node->GetTowerRoot()->GetSucc())){

        NodeStatusResultTuple tuple = TryFlagNode(curr_node, next_node);

        StatusType status = std::get<1>(tuple);

        if(status==StatusType::FLAGGED){
          HelpFlagged(curr_node, next_node);
        }
        next_node = curr_node->GetRight();
      }

      if(next_node && key_cmp_obj_(next_node->GetKey(), key)<0){
        curr_node = next_node;
        next_node = curr_node->GetRight();
      }
    }
    return std::make_pair(curr_node, next_node);
  }


  bool DuplicateKeyValue(const NodeNodePair &arg_pair, const KeyType &key, const ValueType &value) {
    assert(arg_pair.first);
    if (arg_pair.second == nullptr) {
      return false;
    }
    SKIPLISTNODE_TYPE *curr_node = arg_pair.first, *next_node = arg_pair.second;
    while(next_node && key_cmp_obj_(next_node->GetKey(), key)<=0){
      /*
       * next_node has been marked for deletion,
       * help with the deletion and then
       * continue traversing.
       */
      while(next_node!=nullptr && SKIPLISTNODE_TYPE::IsMarkedReference(next_node->GetTowerRoot()->GetSucc())){
        NodeStatusResultTuple tuple = TryFlagNode(curr_node, next_node);

        StatusType status = std::get<1>(tuple);

        if(status==StatusType::FLAGGED){
          HelpFlagged(curr_node, next_node);
        }
        next_node = curr_node->GetRight();
      }
      if (next_node && key_eq_check_obj_(next_node->GetKey(), key) &&
          val_eq_check_obj_(reinterpret_cast<SKIPLISTLEAFNODE_TYPE *>(next_node)->GetValue(), value)) {
        return true;
      }

      if(next_node && key_cmp_obj_(next_node->GetKey(), key)<=0) {
        curr_node = next_node;
        next_node = curr_node->GetRight();
      }
    }
    return false;
  }


  bool Insert(const KeyType &key, const ValueType &value) {

    int tower_height = 1, curr_level = 1;


    NodeNodePair node_node;
    if (is_unique_) {
      node_node = SearchToLevel(key, 1, SKIPLISTNODE_TYPE::TraversalMode::GO_DOWN_ON_LEQ);
    } else {
      node_node = SearchToLevel(key, 1, SKIPLISTNODE_TYPE::TraversalMode::GO_DOWN_ON_LT);
    }


    SKIPLISTNODE_TYPE *inserted_node;
    SKIPLISTNODE_TYPE *prev_node = node_node.first, *next_node = node_node.second;

    PL_ASSERT(prev_node);

    if (is_unique_ && prev_node->GetTowerType() != SKIPLISTNODE_TYPE::TowerType::HEAD_TOWER) {
      if (key_eq_check_obj_(prev_node->GetKey(), key)) {
        return false;   // DUPLICATE
      }
    }

    if (is_unique_ == false && DuplicateKeyValue(node_node, key, value)) {
      return false;
    }

    SKIPLISTNODE_TYPE *new_root_node = reinterpret_cast<SKIPLISTNODE_TYPE *>(
                                            new SKIPLISTLEAFNODE_TYPE(
                                              key, SKIPLISTNODE_TYPE::TowerType::MIDDLE_TOWER,
                                              nullptr, value));

    new_root_node->SetTowerRoot(new_root_node);
    SKIPLISTNODE_TYPE *new_node = new_root_node, *tower_root = new_node;

    while (rand() % 2) {
      tower_height++;
    }

    // Create new root if needed
    SKIPLISTHEADNODE_TYPE *root = root_;
    while (tower_height > root->GetLevel()) {
      SKIPLISTHEADNODE_TYPE *new_root = new SKIPLISTHEADNODE_TYPE(nullptr, root, nullptr, nullptr, root->GetLevel() + 1);
      if (__sync_bool_compare_and_swap(&root_, root, new_root)) {
        root = new_root;
      } else {
        delete new_root;
        root = root_;
      }
    }

    NodeNodePair result_pair;

    while (1) {

      PL_ASSERT(IS_WORD_ALIGNED(prev_node));
      PL_ASSERT((nullptr==next_node) || (IS_WORD_ALIGNED(next_node)));

      result_pair = InsertNode(new_node, prev_node, next_node);
      prev_node = result_pair.first;
      inserted_node = result_pair.second;

      if (inserted_node == nullptr && curr_level == 1) {
        delete new_node;
        return false;
      }

      if(SKIPLISTNODE_TYPE::IsMarkedReference(tower_root->GetSucc())){
        if (inserted_node == new_node && new_node != tower_root) {
          //TODO(gandeevan): uncomment this later
          // DeleteNode(prev_node, new_node);
        }
        return true;
      }
      curr_level++;

      if (curr_level == tower_height + 1) {
        return true;
      }

      SKIPLISTNODE_TYPE *down = new_node;
      new_node = new SKIPLISTNODE_TYPE(key,
                                       SKIPLISTNODE_TYPE::TowerType::MIDDLE_TOWER,
                                       nullptr, down, tower_root);

      // NOTE: Search being done for level 1, 2, ....n. This might hurt performance if not a lot of concurrency.
      // Should we also store the prev and curr at every level while doing searchtolevel?
      if (is_unique_) {
        result_pair = SearchToLevel(key, curr_level, SKIPLISTNODE_TYPE::TraversalMode::GO_DOWN_ON_LEQ);
      } else {
        result_pair = SearchToLevel(key, curr_level, SKIPLISTNODE_TYPE::TraversalMode::GO_DOWN_ON_LT);
      }
      prev_node = result_pair.first;
      next_node = result_pair.second;
    }
  }

  NodeNodePair InsertNode(SKIPLISTNODE_TYPE *new_node, SKIPLISTNODE_TYPE *prev_node,
                          SKIPLISTNODE_TYPE *next_node) {

    PL_ASSERT(IS_WORD_ALIGNED(new_node));
    PL_ASSERT(IS_WORD_ALIGNED(prev_node));
    PL_ASSERT(nullptr==prev_node || IS_WORD_ALIGNED(next_node));

    if(prev_node->GetTowerType()!=SKIPLISTNODE_TYPE::TowerType::HEAD_TOWER) {
      if (key_eq_check_obj_(prev_node->GetKey(), new_node->GetKey())) {
        return std::make_pair(prev_node, nullptr); // DUPLICATE
      }
    }

    while (1) {


      if (SKIPLISTNODE_TYPE::IsFlagged(prev_node->GetSucc())) {
        HelpFlagged(prev_node, prev_node->GetRight());
      }
      else {
        new_node->SetSucc(next_node);
        if (__sync_bool_compare_and_swap(prev_node->GetSuccPtr(), next_node, new_node)) {
          return std::make_pair(prev_node, new_node);
        } else {
          if (SKIPLISTNODE_TYPE::IsFlagged(prev_node->GetSucc())) {
            HelpFlagged(prev_node, prev_node->GetRight());
          }
          while (SKIPLISTNODE_TYPE::IsMarkedReference(prev_node->GetSucc())) {
            prev_node = prev_node->GetBackLink();
          }
        }
      }

      NodeNodePair result = SearchRightLEQ(new_node->GetKey(), prev_node);
      prev_node = result.first;
      next_node = result.second;

      if(prev_node->GetTowerType()!=SKIPLISTNODE_TYPE::TowerType::HEAD_TOWER) {
        if (key_eq_check_obj_(prev_node->GetKey(), new_node->GetKey())) {
          return std::make_pair(prev_node, nullptr);
        }
      }

    }
  }

  bool Delete(const KeyType &key, const ValueType &value) {

    NodeNodePair result = SearchToLevel(key, 1, SKIPLISTNODE_TYPE::TraversalMode::GO_DOWN_ON_LT);
    if (!key_eq_check_obj_(result.second->GetKey(), key)) {
      return false;
    }
    if (is_unique_ == false && !DuplicateKeyValue(result, key, value)) {
      return false;
    }

    /* Unlinks the root node of the tower */
    SKIPLISTNODE_TYPE *root_node = DeleteNode(result.first, result.second);
    if (root_node == nullptr) {
      return false;
    }

    /* Unlinks the node at higher levels */
    SearchToLevel(key, 2, SKIPLISTNODE_TYPE::TraversalMode::GO_DOWN_ON_LEQ);
    return true;
  }

  SKIPLISTNODE_TYPE *DeleteNode(SKIPLISTNODE_TYPE *prev_node, SKIPLISTNODE_TYPE *del_node) {
    NodeStatusResultTuple result_tuple = TryFlagNode(prev_node, del_node);
    if (std::get<1>(result_tuple)==StatusType::FLAGGED) {
      HelpFlagged(prev_node, del_node);
    }
    if (std::get<2>(result_tuple) == false) {
      return nullptr;
    }
    return del_node;
  }



  void HelpMarked(SKIPLISTNODE_TYPE *prev_node, SKIPLISTNODE_TYPE *del_node) {
    PL_ASSERT(IS_WORD_ALIGNED(prev_node));
    PL_ASSERT(IS_WORD_ALIGNED(del_node));

    SKIPLISTNODE_TYPE *old_val = SKIPLISTNODE_TYPE::DoMark(del_node);
    SKIPLISTNODE_TYPE *new_val = del_node->GetRight();

    __sync_bool_compare_and_swap(prev_node->GetSuccPtr(), old_val, new_val);
  }

  void HelpFlagged(SKIPLISTNODE_TYPE *prev_node, SKIPLISTNODE_TYPE *del_node) {
    PL_ASSERT(IS_WORD_ALIGNED(prev_node));
    PL_ASSERT(IS_WORD_ALIGNED(del_node));

    del_node->SetBackLink(prev_node);

    if(!SKIPLISTNODE_TYPE::IsMarkedReference(del_node->GetSucc()))
      TryMark(del_node);

    HelpMarked(prev_node, del_node);
  }


  NodeStatusResultTuple TryFlagNode(SKIPLISTNODE_TYPE *prev_node,
                                    SKIPLISTNODE_TYPE *target_node) {

    PL_ASSERT(IS_WORD_ALIGNED(prev_node));
    PL_ASSERT(IS_WORD_ALIGNED(target_node));

    while(true){
      if(SKIPLISTNODE_TYPE::IsFlagged(prev_node->GetSucc())){
        return  std::make_tuple(prev_node, StatusType::FLAGGED, false);
      }

      SKIPLISTNODE_TYPE* target_node_flagged =  SKIPLISTNODE_TYPE::DoFlag(target_node);

      /* atomically flag the prev_node */
      SKIPLISTNODE_TYPE *result = __sync_val_compare_and_swap(prev_node->GetSuccPtr(),
                                                              target_node, target_node_flagged);

      /* c&s was successful */
      if(result==target_node){
        return std::make_tuple(prev_node, StatusType::FLAGGED, true);
      }

      /* c&s failed, but the prev node was flagged - probably by another thread */
      if(result==target_node_flagged){
        return std::make_tuple(prev_node, StatusType::FLAGGED, false);
      }

      /* prev node was marked - backtrack to an unmarked node */
      while(SKIPLISTNODE_TYPE::IsMarkedReference(prev_node->GetSucc())) {
        prev_node = prev_node->GetBackLink();
        PL_ASSERT(IS_WORD_ALIGNED(prev_node));
      }


      NodeNodePair node_node = SearchRightLT(target_node->GetKey(), prev_node);

      if(node_node.second != target_node)
        return std::make_tuple(prev_node, StatusType::DELETED, false);

    }
  }

  void TryMark(SKIPLISTNODE_TYPE *del_node) {
    PL_ASSERT(IS_WORD_ALIGNED(del_node));

    while(!SKIPLISTNODE_TYPE::IsMarkedReference(del_node->GetSucc())){

      SKIPLISTNODE_TYPE *next = del_node->GetRight();
      SKIPLISTNODE_TYPE *next_marked = SKIPLISTNODE_TYPE::DoMark(next);

      SKIPLISTNODE_TYPE *result = __sync_val_compare_and_swap(del_node->GetSuccPtr(), next, next_marked);

      /* the next node is being deleted as well */
      if(SKIPLISTNODE_TYPE::IsFlagged(result))
        HelpFlagged(del_node, del_node->GetRight());

    }
  }

  inline SKIPLISTNODE_TYPE *GetHead() {
    return head_;
  } 

private:

  SKIPLISTNODE_TYPE *head_;
  SKIPLISTHEADNODE_TYPE *root_;
  const KeyComparator key_cmp_obj_;
  const KeyEqualityChecker key_eq_check_obj_;
  const ValueEqualityChecker val_eq_check_obj_;
  bool is_unique_;
};

}  // namespace index
}  // namespace peloton
