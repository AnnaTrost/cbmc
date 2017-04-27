/*
 * summary.cpp
 *
 *  Created on: Oct 11, 2016
 *      Author: annat
 */

#include "summary.h"
#include <string>
#include <util/suffix.h>
#include <util/prefix.h>
#include <util/i2string.h>
using std::string;

//summaryt::summaryt(): ns(namespacet(symbol_tablet))
//{
//}

summaryt::summaryt(const namespacet &_ns,
    const irep_idt &_function_name,
		const goto_programt &goto_program, const goto_functionst &_goto_functions) :
    ns(_ns), function_name(_function_name), goto_functions(_goto_functions)
{
  formula=false_exprt();
  uncovered=true_exprt();
  collect_inputs_outputs(goto_program);
}

summaryt::~summaryt()
{
}

void summaryt::add_path(statet &state, bool is_terminating)
{
  if(!is_terminating)
    return;
  exprt path_sum=get_path_summary(state);
  exprt reduced_path_sum=get_reduced_path_summary(state);
//  std::cout << "reduced_path_sum: " << from_expr(ns, "", reduced_path_sum) <<std::endl;
  collect_inputs(reduced_path_sum);
  for(exprt output:global_outputs)
  	fix_outputs(path_sum,output.get(ID_identifier),output.type());
  paths.push_back(path_sum);
  exprt negated_path=negate_path(state);
  enumerate_path(negated_path);
  string string_value=from_expr(ns, "", path_sum);
  string string_neg_value=from_expr(ns, "", negated_path);

//  std::cout << "adding path_summary: " << string_value << '\n';
//  std::cout << "adding negated path_summary: " << string_neg_value << '\n';

  formula=or_exprt(formula, path_sum);
  uncovered=and_exprt(uncovered,negated_path);
}

exprt summaryt::get_path_summary(statet &state)
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
    if((*s_it)->func_call_guard.is_not_nil())
    {
      res=and_exprt(res, (*s_it)->func_call_guard);
    }

    if((*s_it)->ssa_rhs.is_not_nil())
    {
      equal_exprt equality((*s_it)->ssa_lhs, (*s_it)->ssa_rhs);
      res=and_exprt(res, equality);
    }
  }

  return res;
}

exprt summaryt::get_reduced_path_summary(statet &state)
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
      string lhs_id=id2string((*s_it)->ssa_lhs.get(ID_identifier));
      string rhs_id=id2string((*s_it)->ssa_rhs.get(ID_identifier));
      //avoid inputs to functions being considered as functions.
      if((has_suffix(lhs_id, "#0")&&lhs_id!="#0") ||
          (rhs_id.find("return_value#0")!=string::npos))
        continue;

      equal_exprt equality((*s_it)->ssa_lhs, (*s_it)->ssa_rhs);
      res=and_exprt(res, equality);
    }
  }

  return res;
}


exprt summaryt::negate_path(statet &state)
{
  exprt res=true_exprt();
  exprt negated_cond=false_exprt();

  std::vector<path_symex_step_reft> steps;
  state.history.build_history(steps);

  for(std::vector<path_symex_step_reft>::const_iterator s_it=
      steps.begin(); s_it!=steps.end(); s_it++)
  {
    if((*s_it)->guard.is_not_nil())
    {
      negated_cond=or_exprt(negated_cond, not_exprt((*s_it)->guard));
    }
    if((*s_it)->func_call_guard.is_not_nil())
    {
      res=and_exprt(res, or_exprt((*s_it)->func_call_guard,(*s_it)->func_call_uncovered));
      negated_cond=or_exprt(negated_cond, (*s_it)->func_call_uncovered);
    }

    if((*s_it)->ssa_rhs.is_not_nil())
    {
      equal_exprt equality((*s_it)->ssa_lhs, (*s_it)->ssa_rhs);
      res=and_exprt(res, equality);
    }
  }
  res=and_exprt(res, negated_cond);
  return res;
}

void summaryt::collect_inputs_outputs(
    const goto_programt &goto_program)
{
  forall_goto_program_instructions(i_it, goto_program)
  {
    if(i_it->type==ASSIGN)
    {
      code_assignt assignment=to_code_assign(i_it->code);
      exprt lhs=assignment.lhs();
      exprt rhs=assignment.rhs();
      add_outputs(lhs,false);
    }
    if(i_it->type==FUNCTION_CALL)
    {
//    	std::cout <<"function call\n";
    	const exprt &function=to_code_function_call(i_it->code).function();
    	if(function.id()==ID_symbol)
    	{
    		const irep_idt &identifier=function.get(ID_identifier);
    		if(called_functions.find(identifier)!=called_functions.end())
    			continue;
    		called_functions.insert(identifier);
    		collect_prints(identifier);

    	}
    }
    if(i_it->type==OTHER && i_it->code.get(ID_statement)==ID_printf)
    {
      bool first = true;
      forall_expr(it, i_it->code.operands())
        {
        if(first) {
          first=false;
          continue;
        }
        add_outputs(*it,true);
        }
    }
  }
}

void summaryt::collect_prints(const irep_idt& id)
{
//	std::cout <<"collect_prints "<<id <<"\n";
	const goto_programt& goto_program=goto_functions.function_map.find(id)->second.body;
	forall_goto_program_instructions(i_it, goto_program)
	  {
	    if(i_it->type==FUNCTION_CALL)
	    {
	    	const exprt &function=to_code_function_call(i_it->code).function();
	    	if(function.id()==ID_symbol)
	    	{
	    		const irep_idt &identifier=function.get(ID_identifier);
	    		if(called_functions.find(identifier)!=called_functions.end())
	    			continue;
	    		called_functions.insert(identifier);
	    		collect_prints(identifier);

	    	}
	    }
	    if(i_it->type==OTHER && i_it->code.get(ID_statement)==ID_printf)
	    {
//	    	std::cout <<"printf "<<i_it->code.operands().size()<<"\n";
	      bool first = true;
	      forall_expr(it, i_it->code.operands())
	        {
	        if(first) {
	          first=false;
	          continue;
	        }
	        add_outputs(*it,true);
	        }
	    }
	  }
}

void summaryt::output(std::ostream &out) const {
  out << "**** " << function_name << " summary:\n";
  out << from_expr(ns, "",formula)<<std::endl;

}

void summaryt::add_outputs(exprt e,bool in_print)
{
//	std::cout <<"add_output\n";
  if(e.id()==ID_symbol)
  {
    const irep_idt &identifier=e.get(ID_identifier);
//  	std::cout <<"add_output symbol "<<identifier<<"\n";

    const symbolt &symbol=ns.lookup(identifier);
    if(in_print || !symbol.is_procedure_local())
    {
      global_outputs.insert(e);
    }
  }
  else
  {
    for(exprt op : e.operands())
    {
      add_outputs(op, in_print);
    }
  }
}

void summaryt::collect_inputs(const exprt &e) {
  if(e.id()==ID_symbol) {
    string id=id2string(e.get(ID_identifier));
    if(has_suffix(id,"#0") && id!="#0")
    {
      inputs.insert(e);
    }
  }
  else
  {
    for(exprt op:e.operands())
    {
      collect_inputs(op);
    }
  }
}

void summaryt::enumerate_path(exprt &e) {
  string prefix="path" + i2string(paths.size()) +"_";
  if(e.id()==ID_symbol) {
    if(inputs.find(e)==inputs.end())
    {
      string id=id2string(e.get(ID_identifier));
      irep_idt name=prefix+id;
      e.set(ID_identifier,name);
    }
  }
  else
  {
    for(exprt op:e.operands())
    {
      enumerate_path(op);
    }
  }
}

std::set<exprt> summaryt::get_inputs() const
{
  return inputs;
}

std::set<exprt> summaryt::get_outputs() const
{
  return global_outputs;
}


exprt summaryt::get_formula() const
{
  return formula;
}

int summaryt::find_max(exprt& e, const irep_idt &output_name, int max)
{
  int res=max;
  if(e.id()==ID_symbol)
  {
    string id=id2string(e.get(ID_identifier));
    string prefix=id2string(output_name)+"#";
    if(has_prefix(id,prefix))
    {
      int curr=stoi(id.substr(prefix.length()));
      if (curr > res)
         res=curr;
    }
  } else
  {
    for(exprt op : e.operands())
    {
      int op_res=find_max(op,output_name,res);
      if (op_res > res)
        res=op_res;
    }
  }
  return res;
}


void summaryt::fix_outputs(exprt& e, const irep_idt &output_name, const typet &type)
{
  int max=find_max(e, output_name, 0);
  e=and_exprt(e,
      equal_exprt(symbol_exprt(output_name,type),symbol_exprt(id2string(output_name)+"#"+std::to_string(max),type)));
  e=and_exprt(e,
        equal_exprt(symbol_exprt(id2string(output_name)+"#0",type),symbol_exprt(id2string(output_name)+"#"+std::to_string(max),type)));
}
