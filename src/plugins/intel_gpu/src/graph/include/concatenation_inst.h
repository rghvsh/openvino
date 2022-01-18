// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

///////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "intel_gpu/primitives/concatenation.hpp"
#include "primitive_inst.h"

#include <string>
#include <memory>

namespace cldnn {

template <>
struct typed_program_node<concatenation> : public typed_program_node_base<concatenation> {
    using parent = typed_program_node_base<concatenation>;
    typed_program_node(const std::shared_ptr<concatenation> prim, program& prog) : parent(prim, prog) {
        support_padding_all(true);
    }

public:
    using parent::parent;

    program_node& input(size_t idx = 0) const { return get_dependency(idx); }

    size_t inputs_count() const { return desc->input.size(); }
};

using concatenation_node = typed_program_node<concatenation>;

template <>
class typed_primitive_inst<concatenation> : public typed_primitive_inst_base<concatenation> {
    using parent = typed_primitive_inst_base<concatenation>;

public:
    static layout calc_output_layout(concatenation_node const& node);
    static std::string to_string(concatenation_node const& node);

public:
    typed_primitive_inst(network& network, concatenation_node const& node);
};

using concatenation_inst = typed_primitive_inst<concatenation>;

}  // namespace cldnn
