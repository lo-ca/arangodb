////////////////////////////////////////////////////////////////////////////////
/// @brief infrastructure for query optimizer
///
/// @file arangod/Aql/Optimizer.cpp
///
/// DISCLAIMER
///
/// Copyright 2010-2014 triagens GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is triAGENS GmbH, Cologne, Germany
///
/// @author Max Neunhoeffer
/// @author Copyright 2014, triagens GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include "Aql/Optimizer.h"
#include "Aql/OptimizerRules.h"

using namespace triagens::aql;

// -----------------------------------------------------------------------------
// --SECTION--                                               the optimizer class
// -----------------------------------------------------------------------------

std::vector<Optimizer::Rule> Optimizer::_rules;

////////////////////////////////////////////////////////////////////////////////
// @brief constructor, this will initialize the rules database
////////////////////////////////////////////////////////////////////////////////

Optimizer::Optimizer () {
  if (_rules.empty()) {
    setupRules();
  }
}

////////////////////////////////////////////////////////////////////////////////
// @brief the actual optimization
////////////////////////////////////////////////////////////////////////////////

int Optimizer::createPlans (ExecutionPlan* plan) {
  int res;
  int leastDoneLevel = 0;
  int maxRuleLevel = _rules.back().level;

  // _plans contains the previous optimisation result
  _plans.clear();
  _plans.push_back(plan, 0);

  int pass = 1;
  while (leastDoneLevel < maxRuleLevel) {
    std::cout << "Entering pass " << pass << " of query optimization..." 
              << std::endl;
    
    // This vector holds the plans we have created in this pass:
    PlanList newPlans;

    // Find variable usage for all old plans now:
    for (auto p : _plans.list) {
      if (! p->varUsageComputed()) {
        p->findVarUsage();
      }
    }

    std::cout << "Have " << _plans.size() << " plans:" << std::endl;

    for (auto p : _plans.list) {
      p->show();
      std::cout << std::endl;
    }

    int count = 0;

    // For all current plans:
    while (_plans.size() > 0) {
      int level;
      auto p = _plans.pop_front(level);
      if (level == maxRuleLevel) {
        newPlans.push_back(p, level);  // nothing to do, just keep it
      }
      else {   // some rule needs applying
        Rule r("dummy", dummyRule, level);
        auto it = std::upper_bound(_rules.begin(), _rules.end(), r);
        TRI_ASSERT(it != _rules.end());
        std::cout << "Trying rule " << it->name << " (" << &(it->func) << ") with level "
                  << it->level << " on plan " << count++
                  << std::endl;
        try {
          res = it->func(this, p, it->level, newPlans);
        }
        catch (...) {
          delete p;
          throw;
        }
        if (res != TRI_ERROR_NO_ERROR) {
          return res;
        }
      }

      // TODO: abort early here if we found a good-enough plan
      // a good-enough plan is probably every plan with costs below some
      // defined threshold. this requires plan costs to be calculated here
    }
    _plans.steal(newPlans);
    leastDoneLevel = maxRuleLevel;
    for (auto l : _plans.levelDone) {
      if (l < leastDoneLevel) {
        leastDoneLevel = l;
      }
    }
    std::cout << "Least done level is " << leastDoneLevel << std::endl;

    // Stop if the result gets out of hand:
    if (_plans.size() >= maxNumberOfPlans) {
      break;
    }
  }

  estimatePlans();
  sortPlans();
  std::cout << "Optimisation ends with " << _plans.size() << " plans."
            << std::endl;
  for (auto p : _plans.list) {
    p->show();
    std::cout << "costing: " << p->getCost() << std::endl;
    std::cout << std::endl;
  }

  return TRI_ERROR_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief estimatePlans
////////////////////////////////////////////////////////////////////////////////

void Optimizer::estimatePlans () {
  for (auto p : _plans.list) {
    p->getCost();
    // this value is cached in the plan, so formally this step is
    // unnecessary, but for the sake of cleanliness...
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief sortPlans
////////////////////////////////////////////////////////////////////////////////

void Optimizer::sortPlans () {
  std::sort(_plans.list.begin(), _plans.list.end(), [](ExecutionPlan* const& a, ExecutionPlan* const& b) -> bool {
    return a->getCost() < b->getCost();
  });
}

////////////////////////////////////////////////////////////////////////////////
/// @brief set up the optimizer rules once and forever
////////////////////////////////////////////////////////////////////////////////

void Optimizer::setupRules () {
  TRI_ASSERT(_rules.empty());

  // List all the rules in the system here:
  // lower level values mean earlier rule execution
  // if two rules have the same level value, they will be executed in declaration order

  //////////////////////////////////////////////////////////////////////////////
  // "Pass 1": moving nodes "up" (potentially outside loops):
  //           please use levels between 1 and 99 here
  //////////////////////////////////////////////////////////////////////////////

  // move calculations up the dependency chain (to pull them out of
  // inner loops etc.)
  registerRule("move-calculations-up", moveCalculationsUpRule, 10);

  // move filters up the dependency chain (to make result sets as small
  // as possible as early as possible)
  registerRule("move-filters-up", moveFiltersUpRule, 20);

  //////////////////////////////////////////////////////////////////////////////
  /// "Pass 2": interchange EnumerateCollection nodes in all possible ways
  ///           this is level 100, please never let new plans from higher
  ///           levels go back to this or lower levels!
  //////////////////////////////////////////////////////////////////////////////

  registerRule("interchangeAdjacentEnumerations", 
               interchangeAdjacentEnumerations, 100);

  //////////////////////////////////////////////////////////////////////////////
  /// "Pass 3": try to remove redundant or unnecessary nodes
  ///           use levels between 101 and 199 for this
  //////////////////////////////////////////////////////////////////////////////

  // remove filters from the query that are not necessary at all
  // filters that are always true will be removed entirely
  // filters that are always false will be replaced with a NoResults node
  registerRule("remove-unnecessary-filters", removeUnnecessaryFiltersRule, 110);
  
  // remove calculations that are never necessary
  registerRule("remove-unnecessary-calculations", 
               removeUnnecessaryCalculationsRule, 120);

  // remove redundant sort blocks
  registerRule("remove-redundant-sorts", removeRedundantSorts, 130);

  //////////////////////////////////////////////////////////////////////////////
  /// "Pass 4": use indexes if possible for FILTER and/or SORT nodes
  ///           use levels between 200 and 299 for this
  //////////////////////////////////////////////////////////////////////////////

  // try to find a filter after an enumerate collection and find an index . . . 
  registerRule("use-index-range", useIndexRange, 210);

  // try to find sort blocks which are superseeded by indexes
  registerRule("use-index-for-sort", useIndexForSort, 220);

  // Now sort them by level:
  std::stable_sort(_rules.begin(), _rules.end());
}

// Local Variables:
// mode: outline-minor
// outline-regexp: "^\\(/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|// --SECTION--\\|/// @\\}\\)"
// End:


