/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*!
 * \file auto_schedule/search_policy/empty_policy.cc
 * \brief This is an brief example of search policy.
 */

#include "empty_policy.h"

#include <tvm/runtime/registry.h>

#include "../measure.h"

namespace tvm {
namespace auto_schedule {

TVM_REGISTER_NODE_TYPE(EmptyPolicyNode);

State EmptyPolicyNode::Search(SearchTask task, int num_measure_trials, int early_stopping,
                              int num_measures_per_round, int verbose, ProgramMeasurer measurer,
                              Optional<Array<SearchCallback>> pre_search_callbacks) {
  cur_task = task;

  // Run pre_search_callbacks before the search process
  // This Interface is usually used to set some init status
  RunCallbacks(pre_search_callbacks);

  // Basic design principe: `SearchOneRound()` several times to get candidate states,
  // measure them and return the best one
  // Measure is disabled if num_measure_trials <= 1
  if (num_measure_trials <= 1) {
    const auto& res = SearchOneRound();
    CHECK_GT(res.size(), 0);

    return res[0];
  } else {
    Array<MeasureInput> inputs;
    Array<MeasureResult> results;

    measurer->Reset();
    int ct = 0;
    // In each round, we call SearchOneRound to get several candidate states,
    // then use ProgramMeasurer to test their performance
    while (ct < num_measure_trials) {
      const auto& res = SearchOneRound();
      ct += res.size();
      // Build MeasureInputs for measuring
      inputs.clear();
      for (const auto& state : res) {
        // The class members measured_states_set_ provided by SearchPolicy can be used to filter
        // out the already measured states
        inputs.push_back(MeasureInput(cur_task, state));
      }
      // ProgramMeasurer will record the state with best performance during measure process
      measurer->Measure(cur_task, GetRef<SearchPolicy>(this), inputs, &results);
    }

    // Return a state with best measured performance
    return measurer->best_state[cur_task->workload_key];
  }
}

// As an example policy, EmptyPolicy always returns a init state
Array<State> EmptyPolicyNode::SearchOneRound() {
  Array<State> res;

  // 1. We will process `Program sampling` first to generate several initial schedules
  res.push_back(cur_task->compute_dag->init_state);

  // 2. Then `Performance Tuning`: use cost model and evolutionary search to seek for the schedule
  // with best performance
  // Note: This example policy does not include this part

  // 3. The returned candidate schedules will be measured in hardware
  return res;
}

TVM_REGISTER_GLOBAL("auto_schedule.EmptyPolicy").set_body_typed([]() {
  return EmptyPolicy(make_object<EmptyPolicyNode>());
});

}  // namespace auto_schedule
}  // namespace tvm
