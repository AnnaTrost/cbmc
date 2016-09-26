/*
 * differential_summary.cpp
 *
 *  Created on: Aug 15, 2016
 *      Author: annat
 */

#include <langapi/language_util.h>
#include "differential_summary.h"


differential_summaryt::differential_summaryt(typet _type) :
    type(_type),
    unchanged(true_exprt())
{
  if(type!=UNAFFECTED)
    unchanged=false_exprt();
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

  if(changed.empty()) {
    out << "changed: empty" << std::endl;
  }
  else
  {
    out << "changed: " << changed.size() << std::endl;
//    for(auto& change : changed)
//    {
//
//    }
  }
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
