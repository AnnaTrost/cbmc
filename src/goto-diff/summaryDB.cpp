/*
 * summaryDB.cpp
 *
 *  Created on: Mar 17, 2017
 *      Author: annat
 */

#include "summaryDB.h"

SummaryDBt* SummaryDBt::instance = NULL;

SummaryDBt::SummaryDBt(const namespacet &_ns_old,
    const namespacet &_ns_new,
    const goto_functionst &_old_goto_functions,
    const goto_functionst &_new_goto_functions) :
    ns_old(_ns_old), ns_new(_ns_new), old_goto_functions(
        _old_goto_functions), new_goto_functions(_new_goto_functions)
{
}

SummaryDBt::~SummaryDBt()
{
}

//void SummaryDBt::initialize(const namespacet &ns_old,
//    const namespacet &ns_new,
//    const goto_functionst &old_goto_functions,
//    const goto_functionst &new_goto_functions)
//{
//
//}

summaryt& SummaryDBt::get_summary(bool old, const irep_idt &function)
{
  map<irep_idt, summaryt>& summaries=
      old ? old_summaries : new_summaries;
  auto it=summaries.find(function);
  if(it!=summaries.end())
    return it->second;

  const namespacet &ns=old ? ns_old : ns_new;
  const goto_functionst &goto_functions=
      old ? old_goto_functions : new_goto_functions;

  auto res=summaries.insert(
      std::make_pair(function,
          summaryt(ns, function,
              goto_functions.function_map.find(function)->second.body,
              goto_functions)));
  return res.first->second;
}
