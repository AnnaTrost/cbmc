/*******************************************************************
 Module: Semantic diff

 Author: Anna Trostanetski

 Date: August 2016

 \*******************************************************************/

#include <iostream>
#include <set>
#include <unordered_set>
#include <list>

#include <goto-programs/goto_model.h>
#include <analyses/dependence_graph.h>
#include <analyses/interval_analysis.h>

#include "semantic_diff.h"
#include "unified_diff.h"
#include "differential_summary.h"
#include "bt_call_graph.h"
#include "path_search.h"

using std::set;
using std::map;
using std::unordered_set;
using std::pair;
using std::make_pair;
using std::list;

class semantic_difft
{
public:
  semantic_difft(goto_modelt &model_old,
      goto_modelt &model_new);

  void operator()();

protected:
  /******************** Fields ********************/

  goto_functionst &old_goto_functions;
  const namespacet ns_old;
//  bt_call_grapht old_call_graph;
  goto_functionst &new_goto_functions;
  const namespacet ns_new;
//  bt_call_grapht new_call_graph;

  map<irep_idt, differential_summaryt> summaries;

  unified_difft unified_diff;

//  dependence_grapht old_dep_graph;
//  dependence_grapht new_dep_graph;

  //caching visited locations in the change impact
  set<pair<irep_idt,unsigned>> affected_locations;

  /******************** Methods ********************/

  void initialize(list<irep_idt> &workset);
  void add_summary(const irep_idt& function,
      differential_summaryt::typet type);
  void mark_affected(const irep_idt& function);
  void mark_affected(
      const irep_idt& function,
      const goto_programt &old_goto_program,
      const goto_programt &new_goto_program,
      const unified_difft::goto_program_difft &diff);
  void propogate_forward(
      const dependence_grapht::nodet &d_node,
      const dependence_grapht &dep_graph,
      const set<irep_idt> &ancestors);
  irep_idt choose(list<irep_idt> &workset);
  differential_summaryt recompute_summary(const irep_idt& function);
  void output_summaries();
};

/*******************************************************************
 Function: semantic_difft::semantic_difft

 Inputs:

 Outputs:

 Purpose:

 \*******************************************************************/

semantic_difft::semantic_difft(goto_modelt &model_old,
    goto_modelt &model_new) :
    old_goto_functions(model_old.goto_functions), ns_old(
        model_old.symbol_table), new_goto_functions(
        model_new.goto_functions), ns_new(model_new.symbol_table), unified_diff(model_old, model_new)
{

//  interval_analysis(ns_old,old_goto_functions);
  // syntactic difference?
  if(!unified_diff())
    return;

  // compute program dependence graph of old code
//  old_dep_graph(old_goto_functions, ns_old);

  // compute program dependence graph of new code
//  new_dep_graph(new_goto_functions, ns_new);
}

void semantic_difft::initialize(list<irep_idt> &workset)
{
  // sorted iteration over intersection(old functions, new functions)
  typedef std::map<irep_idt,
      goto_functionst::function_mapt::const_iterator> function_mapt;

  function_mapt old_funcs, new_funcs;

  forall_goto_functions(it, old_goto_functions)
    old_funcs.insert(std::make_pair(it->first, it));
  forall_goto_functions(it, new_goto_functions)
    new_funcs.insert(std::make_pair(it->first, it));

  function_mapt::const_iterator ito=old_funcs.begin();
  for(function_mapt::const_iterator itn=new_funcs.begin();
      itn!=new_funcs.end(); ++itn)
  {
    while(ito!=old_funcs.end() && ito->first<itn->first)
    {
      add_summary(ito->first, differential_summaryt::REMOVED);
      ++ito;
    }

    if(ito!=old_funcs.end()&&itn->first==ito->first)
    {
      const irep_idt &function_id=itn->first;

      unified_difft::goto_program_difft diff;
      unified_diff.get_diff(function_id, diff);

      if(!diff.empty())
      {
        add_summary(function_id, differential_summaryt::CHANGED);
        mark_affected(function_id);
        workset.push_back(function_id);
      }
      ++ito;
    } else
    {
      add_summary(itn->first, differential_summaryt::ADDED);
    }
  }
}

/*******************************************************************
 Function: semantic_difft::add_summary

 Inputs:

 Outputs:

 Purpose:

 \*******************************************************************/

void semantic_difft::add_summary(const irep_idt& func,
    differential_summaryt::typet type)
{
  map<irep_idt, differential_summaryt>::iterator it=summaries.find(
      func);
  if(it==summaries.end())
  {
    summaries[func]=differential_summaryt(type);
    return;
  }

  //Enforce hierarchy of added,removed > changed > affected > unaffected
  switch(it->second.get_type())
  {
  case differential_summaryt::ADDED:
  case differential_summaryt::REMOVED:
    return;
  case differential_summaryt::CHANGED:
    if(type==differential_summaryt::ADDED
        ||type==differential_summaryt::REMOVED)
      it->second=differential_summaryt(type);
    break;
  case differential_summaryt::AFFECTED:
    if(type==differential_summaryt::ADDED
        ||type==differential_summaryt::REMOVED
        ||type==differential_summaryt::CHANGED)
      it->second=differential_summaryt(type);
    break;
  default: //unaffected
    it->second=differential_summaryt(type);
  }
}

/*******************************************************************
 Function: semantic_difft::mark_affected

 Inputs:

 Outputs:

 Purpose:

 \*******************************************************************/

void semantic_difft::mark_affected(const irep_idt& function)
{
  unified_difft::goto_program_difft diff;
  unified_diff.get_diff(function, diff);

  if(diff.empty())
    return;

  goto_functionst::function_mapt::const_iterator old_fit=
      old_goto_functions.function_map.find(function);
  goto_functionst::function_mapt::const_iterator new_fit=
      new_goto_functions.function_map.find(function);

  goto_programt empty;

  const goto_programt &old_goto_program=
      old_fit==old_goto_functions.function_map.end() ?
          empty : old_fit->second.body;
  const goto_programt &new_goto_program=
      new_fit==new_goto_functions.function_map.end() ?
          empty : new_fit->second.body;

  mark_affected(function, old_goto_program, new_goto_program, diff);
}

void semantic_difft::mark_affected(
  const irep_idt& function,
  const goto_programt &old_goto_program,
  const goto_programt &new_goto_program,
  const unified_difft::goto_program_difft &diff)
{
  goto_programt::const_targett o_it=
    old_goto_program.instructions.begin();
  goto_programt::const_targett n_it=
    new_goto_program.instructions.begin();

//  set<irep_idt> old_ancestors=old_call_graph.get_ancestors(function);
//  set<irep_idt> new_ancestors=new_call_graph.get_ancestors(function);

  for(const auto &d : diff)
  {
    switch(d.second)
    {
      case unified_difft::differencet::SAME:
        assert(o_it!=old_goto_program.instructions.end());
        assert(n_it!=new_goto_program.instructions.end());
        ++o_it;
        assert(n_it==d.first);
        ++n_it;
        break;
      case unified_difft::differencet::DELETED:
        assert(o_it!=old_goto_program.instructions.end());
        assert(o_it==d.first);
//        for(irep_idt ancestor : old_ancestors)
//        {
//          add_summary(ancestor, differential_summaryt::AFFECTED);
//        }

//        propogate_forward(
//          old_dep_graph[old_dep_graph[o_it].get_node_id()],
//          old_dep_graph,
//          old_ancestors);
        ++o_it;
        break;
      case unified_difft::differencet::NEW:
        assert(n_it!=new_goto_program.instructions.end());
        assert(n_it==d.first);
//        for(irep_idt ancestor : new_ancestors)
//        {
//          add_summary(ancestor, differential_summaryt::AFFECTED);
//        }
        //        propogate_forward(
//          new_dep_graph[new_dep_graph[n_it].get_node_id()],
//          new_dep_graph,
//          new_ancestors);
        ++n_it;
        break;
    }
  }
}

static string output_instruction(
    const goto_programt::const_targett& target)
{
  const irep_idt &file=target->source_location.get_file();
  const irep_idt &line=target->source_location.get_line();
//  if(!file.empty()&&!line.empty())
  return std::to_string(target->location_number) + " " + id2string(file)+" "+id2string(line);
}

void semantic_difft::propogate_forward(
    const dependence_grapht::nodet &d_node,
    const dependence_grapht &dep_graph,
    const set<irep_idt> &ancestors)
{
  std::cout<<"propogate_forward "<<output_instruction(d_node.PC)
      <<std::endl;
  for(dependence_grapht::edgest::const_iterator it=d_node.out.begin();
        it!=d_node.out.end(); ++it)
    std::cout<<output_instruction(dep_graph[it->first].PC)
          <<std::endl;
  std::cout<<std::endl;
  for(dependence_grapht::edgest::const_iterator it=d_node.out.begin();
      it!=d_node.out.end(); ++it)
  {
    goto_programt::const_targett src=dep_graph[it->first].PC;

//    if(affected_locations.find(
//        make_pair(src->function, src->location_number))
//        !=affected_locations.end())
//      continue;

    irep_idt from_function=d_node.PC->function;
    irep_idt to_function=src->function;

    if(from_function!=to_function)
    {
      //if to is not an ancestor ignore it
      if(ancestors.find(to_function)!=ancestors.end())
        add_summary(to_function, differential_summaryt::AFFECTED);
    }
//    affected_locations.insert(
//        make_pair(src->function, src->location_number));
    propogate_forward(dep_graph[dep_graph[src].get_node_id()],
        dep_graph, ancestors);
  }
}

irep_idt semantic_difft::choose(list<irep_idt> &workset)
{
  irep_idt front=workset.front();
  workset.pop_front();
  return front;
}

differential_summaryt semantic_difft::recompute_summary(const irep_idt& function)
{
  differential_summaryt &old=summaries[function];
  differential_summaryt res=summaries[function];
//  switch(old.get_type())
//  {
//  case differential_summaryt::ADDED:
//    path_searcht new_path_search(ns_new);
//    new_path_search.eager_infeasibility = true;
//    new_path_search();
//    break;
//  case differential_summaryt::REMOVED:
//    path_searcht old_path_search(ns_old);
//    old_path_search.eager_infeasibility = true;
//    old_path_search();
//    break;
//  case differential_summaryt::CHANGED:
//    path_searcht old_path_search(ns_old), new_path_search(ns_new);
//    old_path_search.eager_infeasibility = true;
//    new_path_search.eager_infeasibility = true;
//    old_path_search();
//    new_path_search();
//    break;
//  case differential_summaryt::AFFECTED:
//    //TODO
//    break;
//  }
  return res;
}


/*******************************************************************
 Function: semantic_difft::output_summaries

 Inputs:

 Outputs:

 Purpose:

 \*******************************************************************/

void semantic_difft::output_summaries()
{
  for(auto& sum : summaries)
  {
    std::cout << "/** " << sum.first << " **/" << std::endl;
    sum.second.output(std::cout);
  }
}

/*******************************************************************
 Function: semantic_difft::operator()

 Inputs:

 Outputs:

 Purpose:

 \*******************************************************************/

void semantic_difft::operator()()
{
  list<irep_idt> workset;
  initialize(workset);

  //Main action
  while(!workset.empty())
  {
    irep_idt curr=choose(workset);
    assert(summaries.find(curr)!=summaries.end());
    differential_summaryt &old_summary=summaries[curr];
    differential_summaryt new_summary=recompute_summary(curr);
    if(old_summary!=new_summary)
    {
      summaries[curr]=new_summary;
//      for(irep_idt caller : new_call_graph.get_callers(curr))
//        workset.push_back(caller);
    }
  }

  output_summaries();
}

/*******************************************************************
 Function: semantic_diff

 Inputs:

 Outputs:

 Purpose:

 \*******************************************************************/

void semantic_diff(goto_modelt &model_old,
    goto_modelt &model_new)
{
  semantic_difft sd(model_old,model_new);
//  sd();

}
