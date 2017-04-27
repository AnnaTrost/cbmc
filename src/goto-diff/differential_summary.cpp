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
	if(type==CHANGED)
		type=CHANGED_BUT_UNAFFECTED;
	else
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
  case CHANGED_BUT_UNAFFECTED:
  	return "changed but unaffected";
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

  namespacet ns(st);
  set<exprt> inputs=summary_new->get_inputs();
  set<exprt> outputs=summary_new->get_outputs();
  bool is_differ=summary_new->get_paths().empty() ? true:false;
  std::cout << "recompute_diff1 "<<std::endl;
  for(exprt old_path_sum : summary_old->get_paths())
  {
    add_prefix(old_path_sum,old_prefix);
    for(exprt new_path_sum : summary_new->get_paths())
    {
//      std::cout << "recompute_diff2 "<<std::endl;
      satcheckt satcheck;
      bv_pointerst bv_pointers(summary_old->ns, satcheck);
        add_prefix(new_path_sum,new_prefix);
//      std::cout << "old_path_sum: "<<from_expr(summary_old->ns,"",old_path_sum)<<std::endl;
//      std::cout << "new_path_sum: "<<from_expr(summary_new->ns,"",new_path_sum)<<std::endl;

      bv_pointers << old_path_sum;
      bv_pointers << new_path_sum;
      for (exprt input:inputs)
        {
          irep_idt old_name=old_prefix+id2string(input.get(ID_identifier));
          irep_idt new_name=new_prefix+id2string(input.get(ID_identifier));
          exprt old_exp=input;
          old_exp.set(ID_identifier,old_name);
          exprt new_exp=input;
          new_exp.set(ID_identifier,new_name);
          exprt eq=equal_exprt(old_exp,new_exp);
//          exprt eq=equal_exprt(symbol_exprt(old_name,input.type()),symbol_exprt(new_name,input.type()));
          new_path_sum=and_exprt(new_path_sum,eq);
//          std::cout << "eq name: "<<old_name<<std::endl;

//          std::cout << "eq input: "<<from_expr(eq)<<std::endl;

          bv_pointers << eq;
        }

        exprt differ=false_exprt();
        for (exprt output:outputs)
        {
          irep_idt old_name=old_prefix+id2string(output.get(ID_identifier));
//          std::cout << "recompute_diff8 output " <<old_name<<std::endl;
          irep_idt new_name=new_prefix+id2string(output.get(ID_identifier));
          exprt old_exp=output;
                    old_exp.set(ID_identifier,old_name);
                    exprt new_exp=output;
                    new_exp.set(ID_identifier,new_name);
                    exprt eq=equal_exprt(old_exp,new_exp);
          differ=or_exprt(differ, not_exprt(equal_exprt(symbol_exprt(old_name,output.type()),symbol_exprt(new_name,output.type()))));
        }
//        std::cout << "differ: "<<from_expr(differ)<<std::endl;

        bv_pointers << differ;
        switch(bv_pointers())
         {
         case decision_proceduret::D_SATISFIABLE:
           std::cout << "diff sat\n";
//           bv_pointers.print_assignment(std::cout);
           is_differ=true;
           break;

         case decision_proceduret::D_UNSATISFIABLE:
//           std::cout << "unsat\n";
           break;

         case decision_proceduret::D_ERROR:
           throw "error from decision procedure";
         }
    }
  }

  satcheckt satcheck;
  bv_pointerst bv_pointers(summary_old->ns, satcheck);
  exprt uncovered=summary_old->get_uncovered();
  uncovered=or_exprt(uncovered,summary_new->get_uncovered());
  std::cout << "uncovered: "<<from_expr(uncovered)<<std::endl;
  bv_pointers << uncovered;
  switch(bv_pointers())
  {
  case decision_proceduret::D_SATISFIABLE:
    std::cout<<"uncovered sat\n";
//    bv_pointers.print_assignment(std::cout);
//    std::cout<<"uncovered sat2\n";
    is_differ=true;
    break;

  case decision_proceduret::D_UNSATISFIABLE:
    std::cout<<"uncovered unsat\n";
    break;

  case decision_proceduret::D_ERROR:
    throw "error from decision procedure";
  }
  if (!is_differ)
  {
    set_unaffected();
  }

  return false;
}

void differential_summaryt::add_prefix(exprt &e, const string &prefix)
{
//  std::cout <<"adding prefix " << from_expr(summary_old->ns,"",e) <<" id " << id2string(e.id())<<std::endl;
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
