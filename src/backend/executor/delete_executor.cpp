/*-------------------------------------------------------------------------
 *
 * delete_executor.cpp
 * file description
 *
 * Copyright(c) 2015, CMU
 *
 * /n-store/src/executor/delete_executor.cpp
 *
 *-------------------------------------------------------------------------
 */

#include "backend/executor/delete_executor.h"

#include "backend/catalog/manager.h"
#include "backend/common/logger.h"
#include "backend/executor/logical_tile.h"
#include "backend/planner/delete_node.h"

namespace peloton {
namespace executor {

/**
 * @brief Constructor for delete executor.
 * @param node Delete node corresponding to this executor.
 */
DeleteExecutor::DeleteExecutor(planner::AbstractPlanNode *node,
                               ExecutorContext *executor_context)
    : AbstractExecutor(node, executor_context) {}

/**
 * @brief Nothing to init at the moment.
 * @return true on success, false otherwise.
 */
bool DeleteExecutor::DInit() {
  assert(children_.size() == 1);
  assert(executor_context_);

  assert(target_table_ == nullptr);

  // Delete tuples in logical tile
  LOG_TRACE("Delete executor :: 1 child \n");

  // Grab data from plan node.
  const planner::DeleteNode &node = GetPlanNode<planner::DeleteNode>();

  target_table_ = node.GetTable();

  return true;
}

/**
 * @brief Delete the table tuples using the position list in the logical tile.
 *
 * If truncate is on, then it will truncate the table itself.
 * @return true on success, false otherwise.
 */
bool DeleteExecutor::DExecute() {
  assert(target_table_);

  // Retrieve next tile.
  const bool success = children_[0]->Execute();
  if (!success) {
    return false;
  }

  std::unique_ptr<LogicalTile> source_tile(children_[0]->GetOutput());

  storage::Tile *tile = source_tile->GetBaseTile(0);
  storage::TileGroup *tile_group = tile->GetTileGroup();

  auto &pos_lists = source_tile.get()->GetPositionLists();
  auto tile_group_id = tile_group->GetTileGroupId();
  auto transaction_ = executor_context_->GetTransaction();
  auto txn_id = transaction_->GetTransactionId();

  LOG_TRACE("Source tile : %p Tuples : %lu \n", source_tile.get(),
            source_tile->NumTuples());

  LOG_TRACE("Transaction ID: %lu\n", txn_id);

  // Delete each tuple
  for (oid_t visible_tuple_id : *source_tile) {
    oid_t physical_tuple_id = pos_lists[0][visible_tuple_id];

    LOG_TRACE("Visible Tuple id : %d, Physical Tuple id : %d \n",
              visible_tuple_id, physical_tuple_id);

    ItemPointer delete_location(tile_group_id, physical_tuple_id);

    // try to delete the tuple
    // this might fail due to a concurrent operation that has latched the tuple
    bool status = target_table_->DeleteTuple(txn_id, delete_location);

    if (status == false) {
      auto &txn_manager = concurrency::TransactionManager::GetInstance();
      txn_manager.AbortTransaction(transaction_);
      transaction_->SetResult(Result::RESULT_FAILURE);
      return false;
    }
    transaction_->RecordDelete(delete_location);
  }

  return true;
}

}  // namespace executor
}  // namespace peloton
