//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// abstract_expression.h
//
// Identification: src/include/expression/abstract_expression.h
//
// Copyright (c) 2015-16, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//


#pragma once

#include <string>

#include "common/serializeio.h"
#include "common/printable.h"
#include "common/types.h"
#include "common/value_factory.h"
#include "common/macros.h"
#include "common/logger.h"

namespace peloton {

class Printable;
class AbstractTuple;

namespace executor {
class ExecutorContext;
}

namespace expression {

//===----------------------------------------------------------------------===//
// AbstractExpression
//
// Predicate objects for filtering tuples during query execution.
// These objects are stored in query plans and passed to Storage Access Manager.
//
// An expression usually has a longer life cycle than an execution, because,
// for example, it can be cached and reused for several executions of the same
// query template. Moreover, those executions can run simultaneously.
// So, an expression should not store per-execution information in its states.
// An expression tree (along with the plan node tree containing it) should
// remain constant and read-only during an execution.
//===----------------------------------------------------------------------===//

using namespace peloton::common;

class AbstractExpression : public Printable {
 public:

  virtual Value Evaluate(const AbstractTuple *tuple1,
                              const AbstractTuple *tuple2,
                              executor::ExecutorContext *context) const = 0;

  /**
   * Return true if this expression or any descendent has a value that should be
   * substituted with a parameter.
   */
  virtual bool HasParameter() const {
    for(auto &child : children_){
      if (child->HasParameter()){
        return true;
      }
    }
    return false;
  }

  const AbstractExpression * GetChild(int index) const{
    return GetModifiableChild(index);
  }

  size_t GetChildrenSize() const{
    return children_.size();
  }

  AbstractExpression * GetModifiableChild(int index) const{
    if(index < 0 || index >= (int)children_.size()){
      return nullptr;
    }
    return children_[index].get();
  }

  void SetChild(int index, AbstractExpression* expr){
    if(index >= (int)children_.size()){
      children_.resize(index+1);
    }
    children_[index].reset(expr);
  }

  /** accessors */

  ExpressionType GetExpressionType() const { return exp_type_; }

  Type::TypeId GetValueType() const { return value_type_; }

  virtual void DeduceExpressionType() {}

  const std::string GetInfo() const {
    std::ostringstream os;

    os << "\tExpression :: "
       << " expression type = " << GetExpressionType() << ","
       << " value type = " << Type::GetInstance(GetValueType())->ToString() << ","
       << std::endl;

    return os.str();
  }

  virtual AbstractExpression *Copy() const = 0;

  inline AbstractExpression *CopyUtil(
      const AbstractExpression *expression) const {
    return (expression == nullptr) ? nullptr : expression->Copy();
  }

  //===--------------------------------------------------------------------===//
  // Serialization/Deserialization
  // Each sub-class will have to implement this function
  //===--------------------------------------------------------------------===//

  //virtual bool SerializeTo(SerializeOutput &output) const {}

  //virtual bool DeserializeFrom(SerializeInput &input) const {}

  virtual int SerializeSize() { return 0; }

  const char* GetExpressionName() const {
    return expr_name_.c_str();
  }

  // Parser stuff
  int ival_ = 0;

  std::string expr_name_;
  std::string alias;

  bool distinct_ = false;

 protected:
  AbstractExpression(ExpressionType type) : exp_type_(type) {}
  AbstractExpression(ExpressionType exp_type, Type::TypeId type_id)
      : exp_type_(exp_type), value_type_(type_id) {}
  AbstractExpression(ExpressionType exp_type, Type::TypeId type_id,
                     AbstractExpression *left,
                     AbstractExpression *right)
      : exp_type_(exp_type), value_type_(type_id) {
    // Order of these is important!
    children_.push_back(std::unique_ptr<AbstractExpression>(left));
    children_.push_back(std::unique_ptr<AbstractExpression>(right));
  }
  AbstractExpression(const AbstractExpression &other) : ival_(other.ival_), expr_name_(other.expr_name_), distinct_(other.distinct_), exp_type_(other.exp_type_),
      value_type_(other.value_type_), has_parameter_(other.has_parameter_){
    for (auto &child : other.children_){
      children_.push_back(std::unique_ptr<AbstractExpression>(child->Copy()));
    }
  }

  ExpressionType exp_type_ = EXPRESSION_TYPE_INVALID;
  Type::TypeId value_type_ = Type::INVALID;

  std::vector<std::unique_ptr<AbstractExpression>> children_;

  bool has_parameter_ = false;
};

}  // End expression namespace
}  // End peloton namespace
