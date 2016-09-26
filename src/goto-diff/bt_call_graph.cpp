/*******************************************************************\

Module: Function Call Graph,
        Bidirectional and contains Transitive closure

Author: Anna Trostanetski

\*******************************************************************/

#include <set>

#include "bt_call_graph.h"

using std::pair;
using std::make_pair;
using std::set;


/*******************************************************************\

Function: bt_call_grapht::bt_call_grapht

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bt_call_grapht::bt_call_grapht(const goto_functionst &funcs) :
    call_grapht(funcs)
{
  call_grapht::operator()();
//  ancestors = graph;
//  apply_transitive_closure(ancestors);
}

/*******************************************************************\

Function: bt_call_grapht::add

  Inputs:

 Outputs:

 Purpose: 

\*******************************************************************/

void bt_call_grapht::add(
  const irep_idt &caller,
  const irep_idt &callee)
{
  graph.insert(std::pair<irep_idt, irep_idt>(caller, callee));
  back_graph.insert(std::pair<irep_idt, irep_idt>(callee, caller));
}

/*******************************************************************\

Function: bt_call_grapht::get_ancestors

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

set<irep_idt> bt_call_grapht::get_ancestors(const irep_idt &function)
{
  map<irep_idt,set<irep_idt> >::iterator it=ancestors.find(function);
  if(it!=ancestors.end())
    return it->second;

  set<irep_idt> res;
  get_ancestors(function, res);
  ancestors[function]=res;
  return res;
}

void bt_call_grapht::get_ancestors(const irep_idt &func,
    set<irep_idt> &res)
{
   res.insert(func);
  const auto& callers = back_graph.equal_range(func);
  for(auto it=callers.first; it!=callers.second; ++it)
  {
    if(res.find(it->second)!=res.end())
      continue;
    get_ancestors(it->second,res);
  }
}

set<irep_idt> bt_call_grapht::get_callers(const irep_idt &func)
{
  set<irep_idt> res;
  const auto& callers=back_graph.equal_range(func);
  for(auto it=callers.first; it!=callers.second; ++it)
  {
    res.insert(it->second);
  }
  return res;
}
