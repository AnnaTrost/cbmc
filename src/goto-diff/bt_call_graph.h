/*******************************************************************\

Module: Function Call Graph,
        Bidirectional and contains Transitive closure

Author: Anna Trostanetski

\*******************************************************************/

#ifndef GOTO_DIFF_DIFFERENTIAL_BT_CALL_GRAPH_H
#define GOTO_DIFF_DIFFERENTIAL_BT_CALL_GRAPH_H

#include <set>
#include <map>

#include <goto-programs/goto_functions.h>
#include <analyses/call_graph.h>

using std::set;
using std::map;

class bt_call_grapht : protected call_grapht
{
public:
  bt_call_grapht(const goto_functionst &funcs);
  ~bt_call_grapht() {}
  void add(const irep_idt &caller, const irep_idt &callee);
  set<irep_idt> get_ancestors(const irep_idt &func);
  set<irep_idt> get_callers(const irep_idt &func);

protected:
  grapht back_graph;
  //computed lazily by demand (caching previous get_ancestors calls)
  map<irep_idt,set<irep_idt> > ancestors;

  void get_ancestors(const irep_idt &func, set<irep_idt> &res);
};

#endif
