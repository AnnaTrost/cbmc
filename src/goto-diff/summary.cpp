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

summaryt::summaryt(const namespacet &_ns,
    const irep_idt &_function_name, const goto_programt &goto_program) :
    ns(_ns), function_name(_function_name)
{
  formula=false_exprt();
  collect_inputs_outputs(goto_program);
}

summaryt::~summaryt()
{
}

void summaryt::add_path(statet &state, bool is_terminating)
{
  exprt path_sum=get_path_summary(state);
  for(exprt output:global_outputs)
    fix_outputs(path_sum,output.get(ID_identifier),output.type());
  paths.push_back(path_sum);
  collect_inputs(path_sum);
  std::string string_value=from_expr(ns, "", path_sum);
//  std::cout << "adding path_summary: " << string_value << '\n';
  formula=or_exprt(formula, path_sum);
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

    if((*s_it)->ssa_rhs.is_not_nil())
    {
//      std::string ssa_lhs=from_expr(ns, "", (*s_it)->ssa_lhs);
//      std::string full_lhs=from_expr(ns, "", (*s_it)->full_lhs);
//      std::cout << "ssa_lhs: "<< ssa_lhs << " full_lhs: " << full_lhs << '\n';
      equal_exprt equality((*s_it)->ssa_lhs, (*s_it)->ssa_rhs);
      res=and_exprt(res, equality);
    }
  }

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
      add_outputs(lhs);
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
        add_outputs(*it);
        }
    }
  }
}

void summaryt::output(std::ostream &out) const {
  out << "**** " << function_name << " summary:\n";
  out << from_expr(ns, "",formula)<<std::endl;

}

void summaryt::add_outputs(exprt e)
{
  if(e.id()==ID_symbol)
  {
    const irep_idt &identifier=e.get(ID_identifier);
    const symbolt &symbol=ns.lookup(identifier);
    if(!symbol.is_procedure_local())
    {
      global_outputs.insert(e);
    }
  }
  else
  {
    for(exprt op : e.operands())
    {
      add_outputs(op);
    }
  }
}

void summaryt::collect_inputs(const exprt &e) {
  if(e.id()==ID_symbol) {
    std::string id=id2string(e.get(ID_identifier));
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
    std::string id=id2string(e.get(ID_identifier));
    std::string prefix=id2string(output_name)+"#";
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
}
