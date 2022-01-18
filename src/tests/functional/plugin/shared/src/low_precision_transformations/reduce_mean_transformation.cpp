// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "low_precision_transformations/reduce_mean_transformation.hpp"
#include <sstream>
#include <string>
#include <vector>
#include <ngraph/ngraph.hpp>

#include "lpt_ngraph_functions/reduce_function.hpp"

namespace LayerTestsDefinitions {

std::string ReduceMeanTransformation::getTestCaseName(const testing::TestParamInfo<ReduceMeanTransformationParams>& obj) {
    ngraph::element::Type netPrecision;
    ngraph::PartialShape inputShape;
    std::string targetDevice;
    ngraph::pass::low_precision::LayerTransformation::Params params;
    ReduceMeanTransformationParam param;;
    std::tie(netPrecision, inputShape, targetDevice, params, param) = obj.param;

    std::ostringstream result;
    result << getTestCaseNameByParams(netPrecision, inputShape, targetDevice, params) << "_" <<
        param.fakeQuantize <<
        param.convert <<
        param.dequantizationBefore <<
        (param.reduceMean.keepDims ? "_keepDims_" : "");

    result << "_reduce_axis_";
    for (const auto& elem : param.reduceMean.constantValues) {
        result << elem << "_";
    }

    return result.str();
}

void ReduceMeanTransformation::SetUp() {
    ngraph::element::Type netPrecision;
    ngraph::PartialShape inputShape;
    ngraph::pass::low_precision::LayerTransformation::Params params;
    ReduceMeanTransformationParam param;;
    std::tie(netPrecision, inputShape, targetDevice, params, param) = GetParam();

    function = ngraph::builder::subgraph::ReduceFunction::get<ngraph::opset1::ReduceMean>(
        netPrecision,
        inputShape,
        param.fakeQuantize,
        param.convert,
        param.dequantizationBefore,
        param.reduceMean.constantValues,
        param.reduceMean.keepDims,
        param.dequantizationAfter);
}

void ReduceMeanTransformation::Run() {
    LayerTestsCommon::Run();

    const auto params = std::get<4>(GetParam());
    const auto actualType = getRuntimePrecision(params.layerName);
    EXPECT_EQ(actualType, params.expectedKernelType);
}

TEST_P(ReduceMeanTransformation, CompareWithRefImpl) {
    Run();
};

} // namespace LayerTestsDefinitions
