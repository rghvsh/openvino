// Copyright (C) 2018-2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "shared_test_classes/single_op/logical.hpp"

namespace ov {
namespace test {
TEST_P(LogicalLayerTest, Inference) {
    run();
}
} // namespace test
} // namespace ov
