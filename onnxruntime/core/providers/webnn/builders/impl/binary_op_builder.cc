// Copyright (c) Microsoft Corporation. All rights reserved.
// Copyright (c) Intel Corporation. All rights reserved.
// Licensed under the MIT License.

#include <core/providers/common.h>

#include "core/providers/webnn/builders/model_builder.h"
#include "core/providers/webnn/builders/op_builder_factory.h"
#include "core/providers/webnn/builders/helper.h"

#include "base_op_builder.h"

namespace onnxruntime {
namespace webnn {

class BinaryOpBuilder : public BaseOpBuilder {
  // Add operator related.
 private:
  Status AddToModelBuilderImpl(ModelBuilder& model_builder, const Node& node,
                               const logging::Logger& logger) const override ORT_MUST_USE_RESULT;

  // Operator support related.
  bool HasSupportedInputsImpl(const GraphViewer&, const Node& node,
                              const emscripten::val& wnn_limits, const logging::Logger& logger) const override;
};

// Add operator related.

Status BinaryOpBuilder::AddToModelBuilderImpl(ModelBuilder& model_builder, const Node& node,
                                              const logging::Logger& /* logger */) const {
  const auto& op_type(node.OpType());

  emscripten::val input0 = model_builder.GetOperand(node.InputDefs()[0]->Name());
  emscripten::val input1 = model_builder.GetOperand(node.InputDefs()[1]->Name());
  emscripten::val output = emscripten::val::object();
  emscripten::val options = emscripten::val::object();
  options.set("label", node.Name());

  if (op_type == "Add") {
    output = model_builder.GetBuilder().call<emscripten::val>("add", input0, input1, options);
  } else if (op_type == "Sub") {
    output = model_builder.GetBuilder().call<emscripten::val>("sub", input0, input1, options);
  } else if (op_type == "Mul") {
    output = model_builder.GetBuilder().call<emscripten::val>("mul", input0, input1, options);
  } else if (op_type == "Div") {
    output = model_builder.GetBuilder().call<emscripten::val>("div", input0, input1, options);
  } else if (op_type == "Pow") {
    output = model_builder.GetBuilder().call<emscripten::val>("pow", input0, input1, options);
  } else if (op_type == "PRelu") {
    output = model_builder.GetBuilder().call<emscripten::val>("prelu", input0, input1, options);
  } else {
    return ORT_MAKE_STATUS(ONNXRUNTIME, INVALID_ARGUMENT,
                           "BinaryOpBuilder::AddToModelBuilderImpl, unknown op: ", op_type);
  }

  model_builder.AddOperand(node.OutputDefs()[0]->Name(), std::move(output));
  return Status::OK();
}

bool BinaryOpBuilder::HasSupportedInputsImpl(const GraphViewer&, const Node& node,
                                             const emscripten::val& wnn_limits, const logging::Logger& logger) const {
  const auto& input_defs = node.InputDefs();
  const std::string_view op_type = node.OpType();
  int32_t input0_type;
  int32_t input1_type;

  if (!GetType(*input_defs[0], input0_type, logger) ||
      !GetType(*input_defs[1], input1_type, logger))
    return false;

  std::array<int32_t, 2> input_types{input0_type, input1_type};
  if (!AreDataTypesSame(op_type, input_types, logger)) {
    return false;
  }

  std::string webnn_input_name = op_type == "PRelu" ? "input" : "a";
  std::string onnx_input_name = op_type == "PRelu" || op_type == "Pow" ? "X" : "A";
  return IsInputRankSupportedByOp(node, wnn_limits, logger) && IsDataTypeSupportedByOp(op_type, input0_type, wnn_limits, webnn_input_name, onnx_input_name, logger);
}

void CreateBinaryOpBuilder(const std::string& op_type, OpBuilderRegistrations& op_registrations) {
  if (op_registrations.op_builder_map.count(op_type) > 0)
    return;

  static std::vector<std::string> op_types =
      {
          "Add",
          "Sub",
          "Mul",
          "Div",
          "Pow",
          "PRelu",
      };

  op_registrations.builders.push_back(std::make_unique<BinaryOpBuilder>());
  for (const auto& type : op_types) {
    op_registrations.op_builder_map.emplace(type, op_registrations.builders.back().get());
  }
}

}  // namespace webnn
}  // namespace onnxruntime
