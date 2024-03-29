// Copyright (C) 2018-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "openvino/op/ops.hpp"

namespace ov {
namespace opset8 {
#define _OPENVINO_OP_REG(a, b) using b::a;
#include "openvino/opsets/opset8_tbl.hpp"
#undef _OPENVINO_OP_REG
}  // namespace opset8
}  // namespace ov
