// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "vpu/private_plugin_config.hpp"
#include "vpu/utils/containers.hpp"
#include "vpu/configuration/options/disable_reorder.hpp"
#include "vpu/configuration/switch_converters.hpp"
#include "vpu/configuration/plugin_configuration.hpp"

namespace vpu {

void DisableReorderOption::validate(const std::string& value) {
    const auto& converters = string2switch();
    VPU_THROW_UNLESS(converters.count(value) != 0, R"(unexpected {} option value "{}", only {} are supported)",
        key(), value, getKeys(converters));
}

void DisableReorderOption::validate(const PluginConfiguration& configuration) {
    validate(configuration[key()]);
}

std::string DisableReorderOption::key() {
    return InferenceEngine::MYRIAD_DISABLE_REORDER;
}

details::Access DisableReorderOption::access() {
    return details::Access::Private;
}

details::Category DisableReorderOption::category() {
    return details::Category::CompileTime;
}

std::string DisableReorderOption::defaultValue() {
    return InferenceEngine::PluginConfigParams::NO;
}

DisableReorderOption::value_type DisableReorderOption::parse(const std::string& value) {
    const auto& converters = string2switch();
    VPU_THROW_UNSUPPORTED_OPTION_UNLESS(converters.count(value) != 0, R"(unexpected {} option value "{}", only {} are supported)",
        key(), value, getKeys(converters));
    return converters.at(value);
}

}  // namespace vpu
