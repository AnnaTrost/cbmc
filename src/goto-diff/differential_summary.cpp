/*
 * differential_summary.cpp
 *
 *  Created on: Aug 15, 2016
 *      Author: annat
 */

#include <langapi/language_util.h>
#include <solvers/flattening/bv_pointers.h>
#include <solvers/sat/satcheck.h>
#include "differential_summary.h"


differential_summaryt::differential_summaryt(typet _type) :
    type(_type),
    summary_old(NULL),
    summary_new(NULL)
{
  if(type!=UNAFFECTED)
    unchanged=false_exprt();
  else
    unchanged=true_exprt();
}

void differential_summaryt::set_unaffected()
{
  type=UNAFFECTED;
  unchanged=true_exprt();
}
/*******************************************************************\

Function: differential_summaryt::output

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void differential_summaryt::output(std::ostream &out) const
{
  out << "type " << typet2string(type) <<std::endl;

//  if(changed.empty()) {
//    out << "changed: empty" << std::endl;
//  }
//  else
//  {
////    out << "changed: " << changed.size() << std::endl;
////    for(auto& change : changed)
////    {
////
////    }
//  }
  out << "unchanged " << from_expr(unchanged) << std::endl;
}

/*******************************************************************\

Function: differential_summaryt::typet2string

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

string differential_summaryt::typet2string(const typet& type) {
  switch(type) {
  case ADDED:
    return "added";
  case REMOVED:
    return "removed";
  case CHANGED:
    return "changed";
  case AFFECTED:
    return "affected";
  case UNAFFECTED:
    return "unaffected";
  }
  return "";
}



bool differential_summaryt::operator==(
    const differential_summaryt &other) const
{
  return type==other.type
    //  && changed==other.changed
      && unchanged==other.unchanged;
}

bool differential_summaryt::operator!=(
    const differential_summaryt &other) const
{
  return !((*this)==other);
}



void differential_summaryt::add_summary_old(summaryt& sum)
{
  summary_old=&sum;
}
void differential_summaryt::add_summary_new(summaryt& sum)
{
  summary_new=&sum;
}

bool differential_summaryt::recompute_diff()
{
  assert(summary_old && summary_new);
//  if(summary_old->get_inputs() != summary_new->get_inputs() ||
//      summary_old->get_outputs() != summary_new->get_outputs())
//    return false;


  namespacet ns(st);
  satcheckt satcheck;
  set<exprt> inputs=summary_new->get_inputs();
  set<exprt> outputs=summary_new->get_outputs();
  bool is_differ=false;
  std::cout << "recompute_diff1 "<<std::endl;
  for(exprt old_path_sum : summary_old->get_paths())
  {
    for(exprt new_path_sum : summary_new->get_paths())
    {
      std::cout << "recompute_diff2 "<<std::endl;
      bv_pointerst bv_pointers(ns, satcheck);
      std::cout << "recompute_diff3 "<<std::endl;
      add_prefix(old_path_sum,old_prefix);
      add_prefix(new_path_sum,new_prefix);
      std::cout << "recompute_diff4 "<<std::endl;

//      std::cout << "old_path_sum: "<<from_expr(old_path_sum)<<std::endl;
//      std::cout << "new_path_sum: "<<from_expr(new_path_sum)<<std::endl;

      bv_pointers << old_path_sum;
      bv_pointers << new_path_sum;
      std::cout << "recompute_diff5 "<<std::endl;
      for (exprt input:inputs)
        {
        std::cout << "recompute_diff6 "<<std::endl;
          irep_idt old_name=old_prefix+id2string(input.get(ID_identifier));
          irep_idt new_name=new_prefix+id2string(input.get(ID_identifier));
          exprt eq=equal_exprt(symbol_exprt(old_name,input.type()),symbol_exprt(new_name,input.type()));
//          std::cout << "eq name: "<<old_name<<std::endl;

//          std::cout << "eq input: "<<from_expr(eq)<<std::endl;

          bv_pointers << eq;
        }
      std::cout << "recompute_diff7 "<<std::endl;
        exprt differ=false_exprt();
        for (exprt output:outputs)
        {
          std::cout << "recompute_diff8 "<<std::endl;
          irep_idt old_name=old_prefix+id2string(output.get(ID_identifier));
          irep_idt new_name=new_prefix+id2string(output.get(ID_identifier));
          differ=or_exprt(differ, not_exprt(equal_exprt(symbol_exprt(old_name,output.type()),symbol_exprt(new_name,output.type()))));
        }
//        std::cout << "differ: "<<from_expr(differ)<<std::endl;
        std::cout << "recompute_diff9 "<<std::endl;
        bv_pointers << differ;
        std::cout << "running sat\n";

        switch(bv_pointers())
         {
         case decision_proceduret::D_SATISFIABLE:
           std::cout << "sat\n";
           is_differ=true;
//           bv_pointers.print_assignment(std::cout);
           break;
//           return false;

         case decision_proceduret::D_UNSATISFIABLE:
           std::cout << "unsat\n";
           break;
//           return true;

         case decision_proceduret::D_ERROR:
           throw "error from decision procedure";
         }

    }
  }
  if (!is_differ)
  {
    set_unaffected();

  }

  return false;
}

void differential_summaryt::add_prefix(exprt &e, const string &prefix)
{
  if(e.id()==ID_symbol)
  {
    irep_idt name=prefix+id2string(e.get(ID_identifier));
//    std::cout <<"setting "<<name <<std::endl;
    e.set(ID_identifier,name);

  } else
  {
    for(exprt& op : e.operands())
    {
      add_prefix(op,prefix);
    }
  }
//  std::cout << "add_prefix: "<<from_expr(e)<<std::endl;
}
