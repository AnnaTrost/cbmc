/*******************************************************************\

Module: Path-based Symbolic Execution

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include <util/time_stopping.h>

#include <solvers/flattening/bv_pointers.h>
#include <solvers/sat/satcheck.h>

#include <path-symex/build_goto_trace.h>

#include "path_search.h"
#include "path_symex.h"
#include "summaryDB.h"
#include "path_symex_class.h"

#include <iostream>
#include <string>
/*******************************************************************\

Function: path_searcht::operator()

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/
exprt path_searcht::get_path_summary(statet &state)
{
  exprt res=true_exprt();

  std::vector<path_symex_step_reft> steps;
  state.history.build_history(steps);

  for(std::vector<path_symex_step_reft>::const_iterator s_it=
      steps.begin(); s_it!=steps.end(); s_it++)
  {
    if((*s_it)->guard.is_not_nil())
    {
      res=and_exprt(res, (*s_it)->guard);
    }

    if((*s_it)->ssa_rhs.is_not_nil())
    {
      equal_exprt equality((*s_it)->ssa_lhs, (*s_it)->ssa_rhs);
      res=and_exprt(res, equality);
    }
  }

  return res;
}

void initialize_path_search(path_searcht &path_search)
{
  path_search.eager_infeasibility = true;
  //path_search.set_unwind_limit(2);
//  path_search.set_branch_bound(500);
//  path_search.set_depth_limit(500);
//  path_search.set_context_bound(5);
}

path_searcht::resultt path_searcht::operator()(
    const goto_functionst &goto_functions,
    const irep_idt &entry_point, bool old, bool refine)
{
  locst locs(ns);
  var_mapt var_map(ns);
  
  locs.build(goto_functions, entry_point);

  // this is the container for the history-forest  
  path_symex_historyt history;
  
  queue.push_back(initial_state(var_map, locs, history));
  
  // set up the statistics
  number_of_dropped_states=0;
  number_of_paths=0;
  number_of_VCCs=0;
  number_of_steps=0;
  number_of_feasible_paths=0;
  number_of_infeasible_paths=0;
  number_of_VCCs_after_simplification=0;
  number_of_failed_properties=0;
  number_of_locs=locs.size();

  // stop the time
  start_time=current_time();
  
  SummaryDBt &summaryDB=SummaryDBt::getInstance();
  summaryt& sum=summaryDB.get_summary(old,entry_point);

  //initialize_property_map(goto_functions);
  
  while(!queue.empty())
  {
    number_of_steps++;
//    std::cout << "number_of_steps\n";
  
    // Pick a state from the queue,
    // according to some heuristic.
    // The state moves to the head of the queue.
    pick_state();
    
    // move into temporary queue
    queuet tmp_queue;
    tmp_queue.splice(
      tmp_queue.begin(), queue, queue.begin(), ++queue.begin());
    
    try
    {
      statet &state=tmp_queue.front();
      
      // record we have seen it
      loc_data[state.get_pc().loc_number].visited=true;

      debug() << "Loc: #" << state.get_pc().loc_number
              << ", queue: " << queue.size()
              << ", depth: " << state.get_depth();
      for(const auto & s : queue)
        debug() << ' ' << s.get_depth();
        
      debug() << eom;
    
      if(drop_state(state))
      {
//        std::cout << "drop_state\n";
        sum.add_path(state,false);
        number_of_dropped_states++;
        number_of_paths++;
        continue;
      }
      
      if(!state.is_executable())
      {
        sum.add_path(state,true);
//        std::cout << "not executable\n";
        number_of_paths++;
        continue;
      }

      if(eager_infeasibility &&
         state.last_was_branch())
      {
        if (!is_feasible(state)) {
          std::cout << "not feasible\n";
          number_of_infeasible_paths++;
          number_of_paths++;
          continue;
        }
        else {
//          std::cout << "feasible\n";
        }
      }
      
      if(number_of_steps%1000==0)
      {
        status() << "Queue " << queue.size()
                 << " thread " << state.get_current_thread()
                 << '/' << state.threads.size()
                 << " PC " << state.pc() << messaget::eom;
      }

      // an error, possibly?
//      if(state.get_instruction()->is_assert())
//      {
//        if(show_vcc)
//          do_show_vcc(state);
//        else
//        {
//          check_assertion(state);
//
//          // all assertions failed?
////          if(number_of_failed_properties==property_map.size())
////            break;
//        }
//      }

      // execute
      path_symex(state, tmp_queue, old, refine);
      
      const goto_programt::instructiont &instruction=
          *state.get_instruction();
      if(instruction.type==FUNCTION_CALL)
      {
        const code_function_callt &call=to_code_function_call(
            instruction.code);
        exprt f=state.read(call.function());
        if(f.id()==ID_symbol)
        {
          const irep_idt &function_identifier=
              to_symbol_expr(f).get_identifier();
          if(refine)
          {
            assign_arguments(state, function_identifier, call);

            summaryt& summary=summaryDB.get_summary(old,
                function_identifier);
            if(is_uncovered_feasible(state, summary.get_uncovered()))
            {
              std::cout<<"FUNCTION_CALL uncovered_feasible"
                  <<std::endl;

              path_searcht new_path_search(ns);
              exprt precond=get_path_summary(state);
              std::cout<<"precond "<<from_expr(ns, "", precond)
                  <<std::endl;

              initialize_path_search(new_path_search);
              new_path_search.set_precondition(precond);
              new_path_search(goto_functions, function_identifier,
                  old, refine);
            }
          }
        }
      }

      // put at head of main queue
      queue.splice(queue.begin(), tmp_queue);
    }
    catch(const std::string &e)
    {
      std::cout << e << eom;
      number_of_dropped_states++;
    }
    catch(const char *e)
    {
      std::cout << e << eom;
      number_of_dropped_states++;
    }
//    catch(int)
//    {
//      std::cout << "catch int\n";
//      number_of_dropped_states++;
//    }
  }
  
//  report_statistics();
  
  return number_of_failed_properties==0?SAFE:UNSAFE;
}

void path_searcht::assign_arguments(statet &state,
    const irep_idt &function_identifier,
    const code_function_callt &call)
{
  locst::function_mapt::const_iterator f_it=
      state.locs.function_map.find(function_identifier);
  if(f_it==state.locs.function_map.end())
    throw "failed to find `"+id2string(function_identifier)
        +"' in function_map";

  path_symext path_sym(false, false);

  const locst::function_entryt &function_entry=f_it->second;
  const code_typet &code_type=function_entry.type;
  const code_typet::parameterst &function_parameters=
      code_type.parameters();
  const exprt::operandst &call_arguments=call.arguments();
  // now assign the argument values to parameters
  for(unsigned i=0; i<call_arguments.size(); i++)
  {
    if(i<function_parameters.size())
    {
      const code_typet::parametert &function_parameter=
          function_parameters[i];
      irep_idt identifier=function_parameter.get_identifier();

      if(identifier==irep_idt())
        throw "function_call "+id2string(function_identifier)
            +" no identifier for function parameter";

      symbol_exprt lhs(identifier, function_parameter.type());
      exprt rhs=call_arguments[i];

      // lhs/rhs types might not match
      if(lhs.type()!=rhs.type())
        rhs.make_typecast(lhs.type());

      path_sym.assign(state, lhs, rhs, false);
    }
  }
}

path_searcht::resultt path_searcht::operator()(
  const goto_functionst &goto_functions)
{
  locst locs(ns);
  var_mapt var_map(ns);

  locs.build(goto_functions);

  // this is the container for the history-forest
  path_symex_historyt history;

  queue.push_back(initial_state(var_map, locs, history));

  // set up the statistics
  number_of_dropped_states=0;
  number_of_paths=0;
  number_of_VCCs=0;
  number_of_steps=0;
  number_of_feasible_paths=0;
  number_of_infeasible_paths=0;
  number_of_VCCs_after_simplification=0;
  number_of_failed_properties=0;
  number_of_locs=locs.size();

  // stop the time
  start_time=current_time();

  //initialize_property_map(goto_functions);

  while(!queue.empty())
  {
    number_of_steps++;

    // Pick a state from the queue,
    // according to some heuristic.
    // The state moves to the head of the queue.
    pick_state();

    // move into temporary queue
    queuet tmp_queue;
    tmp_queue.splice(
      tmp_queue.begin(), queue, queue.begin(), ++queue.begin());

    try
    {
      statet &state=tmp_queue.front();

      // record we have seen it
      loc_data[state.get_pc().loc_number].visited=true;

      debug() << "Loc: #" << state.get_pc().loc_number
              << ", queue: " << queue.size()
              << ", depth: " << state.get_depth();
      for(const auto & s : queue)
        debug() << ' ' << s.get_depth();

      debug() << eom;

      if(drop_state(state))
      {
        std::cout << "drop_state\n";
        number_of_dropped_states++;
        number_of_paths++;
        continue;
      }

      if(!state.is_executable())
      {
        std::cout << "not executable\n";
        number_of_paths++;
        continue;
      }

      if(eager_infeasibility &&
         state.last_was_branch())
      {
        if (!is_feasible(state)) {
          std::cout << "not feasible\n";
          number_of_infeasible_paths++;
          number_of_paths++;
          continue;
        }
        else {
//          std::cout << "feasible\n";
        }
      }

      if(number_of_steps%1000==0)
      {
        status() << "Queue " << queue.size()
                 << " thread " << state.get_current_thread()
                 << '/' << state.threads.size()
                 << " PC " << state.pc() << messaget::eom;
      }

      // an error, possibly?
      if(state.get_instruction()->is_assert())
      {
        if(show_vcc)
          do_show_vcc(state);
        else
        {
          check_assertion(state);

          // all assertions failed?
          if(number_of_failed_properties==property_map.size())
            break;
        }
      }

      // execute
//      path_symex(state, tmp_queue);

      // put at head of main queue
      queue.splice(queue.begin(), tmp_queue);
    }
    catch(const std::string &e)
    {
      std::cout << e << eom;
      number_of_dropped_states++;
    }
    catch(const char *e)
    {
      std::cout << e << eom;
      number_of_dropped_states++;
    }
    catch(int)
    {
      std::cout << "catch int\n";
      number_of_dropped_states++;
    }
  }

  report_statistics();

  return number_of_failed_properties==0?SAFE:UNSAFE;
}

/*******************************************************************\

Function: path_searcht::report_statistics

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void path_searcht::report_statistics()
{
  std::size_t number_of_visited_locations=0;
  for(const auto & l : loc_data)
    if(l.visited)
      number_of_visited_locations++;

  #if 0
  for(unsigned l=0; l<loc_data.size(); l++)
    if(!loc_data[l].visited) status() << "NV: " << l << eom;
  #endif

  // report a bit
  std::cout << "Number of visited locations: "
           << number_of_visited_locations << " (out of "
           << number_of_locs << ')' << messaget::eom;

  std::cout << "Number of dropped states: "
           << number_of_dropped_states << messaget::eom;

  std::cout << "Number of paths: "
           << number_of_paths << messaget::eom;

  std::cout << "Number of infeasible paths: "
           << number_of_infeasible_paths << messaget::eom;

  std::cout << "Generated " << number_of_VCCs << " VCC(s), "
           << number_of_VCCs_after_simplification
           << " remaining after simplification"
           << messaget::eom;
  
  time_periodt total_time=current_time()-start_time;
  std::cout << "Runtime: " << total_time << "s total, "
           << sat_time << "s SAT" << messaget::eom;
}

/*******************************************************************\

Function: path_searcht::pick_state

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void path_searcht::pick_state()
{
  switch(search_heuristic)
  {
  case search_heuristict::DFS:
    // Picking the first one (most recently added) is a DFS.
    return;
  
  case search_heuristict::BFS:
    // Picking the last one is a BFS.
    if(queue.size()>=2)
      // move last to first position
      queue.splice(queue.begin(), queue, --queue.end(), queue.end());
    return;
    
  case search_heuristict::LOCS:
    return;
  }  
}

/*******************************************************************\

Function: path_searcht::do_show_vcc

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void path_searcht::do_show_vcc(statet &state)
{
  // keep statistics
  number_of_VCCs++;
  
  const goto_programt::instructiont &instruction=
    *state.get_instruction();
    
//  mstreamt &out=result();
//  out << "do_show_vcc"<< eom;
  std::ostream &out=std::cout;
  if(instruction.source_location.is_not_nil())
    out << instruction.source_location << '\n';
  
  if(instruction.source_location.get_comment()!="")
    out << instruction.source_location.get_comment() << '\n';
    
  unsigned count=1;
  
  std::vector<path_symex_step_reft> steps;
  state.history.build_history(steps);

  for(std::vector<path_symex_step_reft>::const_iterator
      s_it=steps.begin();
      s_it!=steps.end();
      s_it++)
  {      
    if((*s_it)->guard.is_not_nil())
    {
      std::string string_value=from_expr(ns, "", (*s_it)->guard);
      out << "{-" << count << "} " << string_value << '\n';
      count++;
    }

    if((*s_it)->ssa_rhs.is_not_nil())
    {
      equal_exprt equality((*s_it)->ssa_lhs, (*s_it)->ssa_rhs);
      std::string string_value=from_expr(ns, "", equality);
      out << "{-" << count << "} " << string_value << '\n';
      count++;
    }
  }

  out << "|--------------------------" << '\n';
  
  exprt assertion=state.read(instruction.guard);

  out << "{" << 1 << "} "
      << from_expr(ns, "", assertion) << '\n';

  if(!assertion.is_true())
    number_of_VCCs_after_simplification++;
  
  out << eom;
}

/*******************************************************************\

Function: path_searcht::drop_state

  Inputs:

 Outputs:

 Purpose: decide whether to drop a state

\*******************************************************************/

bool path_searcht::drop_state(const statet &state) const
{
	loc_reft l=state.get_pc();
	if(l.loc_number<0 || l.loc_number >= number_of_locs)
		return true;
  // depth limit
  if(depth_limit_set && state.get_depth()>depth_limit)
    return true;
  
  // context bound
  if(context_bound_set && state.get_no_thread_interleavings()>context_bound)
    return true;
  
  // branch bound
  if(branch_bound_set && state.get_no_branches()>branch_bound)
    return true;
  
  // unwinding limit -- loops
  if(unwind_limit_set && state.get_instruction()->is_backwards_goto())
  {
    for(path_symex_statet::unwinding_mapt::const_iterator
        it=state.unwinding_map.begin();
        it!=state.unwinding_map.end();
        it++)
      if(it->second>unwind_limit)
        return true;
  }
  
  // unwinding limit -- recursion
  if(unwind_limit_set && state.get_instruction()->is_function_call())
  {
    for(path_symex_statet::recursion_mapt::const_iterator
        it=state.recursion_map.begin();
        it!=state.recursion_map.end();
        it++)
      if(it->second>unwind_limit)
        return true;
  }
  
  return false;
}

/*******************************************************************\

Function: path_searcht::check_assertion

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void path_searcht::check_assertion(statet &state)
{
  // keep statistics
  number_of_VCCs++;
  
  const goto_programt::instructiont &instruction=
    *state.get_instruction();

  irep_idt property_name=instruction.source_location.get_property_id();
  property_entryt &property_entry=property_map[property_name];
  
  if(property_entry.status==FAILURE)
    return; // already failed
  else if(property_entry.status==NOT_REACHED)
    property_entry.status=SUCCESS; // well, for now!

  // the assertion in SSA
  exprt assertion=
    state.read(instruction.guard);

  if(assertion.is_true()) return; // no error, trivially

  // keep statistics
  number_of_VCCs_after_simplification++;
  
  status() << "Checking property " << property_name << eom;

  // take the time
  absolute_timet sat_start_time=current_time();

  satcheckt satcheck;
  bv_pointerst bv_pointers(ns, satcheck);
  
  satcheck.set_message_handler(get_message_handler());
  bv_pointers.set_message_handler(get_message_handler());

  if(!state.check_assertion(bv_pointers))
  {
    build_goto_trace(state, bv_pointers, property_entry.error_trace);
    property_entry.status=FAILURE;
    number_of_failed_properties++;
  }
  
  sat_time+=current_time()-sat_start_time;
}

/*******************************************************************\

Function: path_searcht::is_feasible

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool path_searcht::is_feasible(statet &state)
{
  status() << "Feasibility check" << eom;

  // take the time
  absolute_timet sat_start_time=current_time();

  satcheckt satcheck;
  bv_pointerst bv_pointers(ns, satcheck);
  
  satcheck.set_message_handler(get_message_handler());
  bv_pointers.set_message_handler(get_message_handler());
  if(precondition_set)
  {
    bv_pointers << precondition;
  }

  bool result=state.is_feasible(bv_pointers);
  
  sat_time+=current_time()-sat_start_time;
  
  return result;
}

bool path_searcht::is_uncovered_feasible(statet &state, exprt& uncovered)
{
  // take the time
  absolute_timet sat_start_time=current_time();

  satcheckt satcheck;
  bv_pointerst bv_pointers(ns, satcheck);

  satcheck.set_message_handler(get_message_handler());
  bv_pointers.set_message_handler(get_message_handler());
  if(precondition_set)
  {
    bv_pointers << precondition;
  }
  bv_pointers << uncovered;

  bool result=state.is_feasible(bv_pointers);

  sat_time+=current_time()-sat_start_time;

  return result;
}


/*******************************************************************\

Function: path_searcht::initialize_property_map

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void path_searcht::initialize_property_map(
  const goto_functionst &goto_functions)
{
  for(goto_functionst::function_mapt::const_iterator
      it=goto_functions.function_map.begin();
      it!=goto_functions.function_map.end();
      it++)
    if(!it->second.is_inlined())
    {
      const goto_programt &goto_program=it->second.body;
    
      for(goto_programt::instructionst::const_iterator
          it=goto_program.instructions.begin();
          it!=goto_program.instructions.end();
          it++)
      {
        if(!it->is_assert())
          continue;
      
        const source_locationt &source_location=it->source_location;
      
        irep_idt property_name=source_location.get_property_id();
        
        property_entryt &property_entry=property_map[property_name];
        property_entry.status=NOT_REACHED;
        property_entry.description=source_location.get_comment();
        property_entry.source_location=source_location;
      }
    }    
}
