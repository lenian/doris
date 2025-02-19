// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
#pragma once

#include "exprs/like_predicate.h"
#include "olap/column_predicate.h"
#include "runtime/string_value.h"
#include "udf/udf.h"
#include "vec/columns/column_dictionary.h"
#include "vec/core/types.h"

namespace doris {

class LikeColumnPredicate : public ColumnPredicate {
public:
    LikeColumnPredicate(bool opposite, uint32_t column_id, doris_udf::FunctionContext* fn_ctx,
                        doris_udf::StringVal val);
    ~LikeColumnPredicate() override = default;

    PredicateType type() const override { return PredicateType::EQ; }
    void evaluate_vec(const vectorized::IColumn& column, uint16_t size, bool* flags) const override;

    void evaluate(ColumnBlock* block, uint16_t* sel, uint16_t* size) const override;

    void evaluate_or(ColumnBlock* block, uint16_t* sel, uint16_t size, bool* flags) const override {
    }
    void evaluate_and(ColumnBlock* block, uint16_t* sel, uint16_t size,
                      bool* flags) const override {}
    Status evaluate(const Schema& schema, const std::vector<BitmapIndexIterator*>& iterators,
                    uint32_t num_rows, roaring::Roaring* roaring) const override {
        return Status::OK();
    }

private:
    template <bool is_nullable>
    void _base_evaluate(const ColumnBlock* block, uint16_t* sel, uint16_t* size) const {
        uint16_t new_size = 0;
        for (uint16_t i = 0; i < *size; ++i) {
            uint16_t idx = sel[i];
            sel[new_size] = idx;
            const StringValue* cell_value =
                    reinterpret_cast<const StringValue*>(block->cell(idx).cell_ptr());
            doris_udf::StringVal target;
            cell_value->to_string_val(&target);
            if constexpr (is_nullable) {
                new_size += _opposite ^ (!block->cell(idx).is_null() &&
                                         (_state->function)(_fn_ctx, target, pattern).val);
            } else {
                new_size += _opposite ^ (_state->function)(_fn_ctx, target, pattern).val;
            }
        }
        *size = new_size;
    }

    std::string _origin;
    // life time controlled by scan node
    doris_udf::FunctionContext* _fn_ctx;
    doris_udf::StringVal pattern;

    LikePredicateState* _state;
};

} //namespace doris
