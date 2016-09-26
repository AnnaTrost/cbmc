/*******************************************************************\

Module: Show Value Sets

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include <iostream>

#include <util/xml.h>

#include "value_set_analysis.h"
#include "show_value_sets.h"

/*******************************************************************\

Function: show_value_sets

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void show_value_sets(
  ui_message_handlert::uit ui,
  const goto_functionst &goto_functions,
  value_set_analysist &value_set_analysis,
  const namespacet &ns)
{
  switch(ui)
  {
  case ui_message_handlert::XML_UI:
    {
      xmlt xml;
      value_set_analysis.convert(ns, goto_functions, xml);
      std::cout << xml << std::endl;
    }
    break;
    
  case ui_message_handlert::PLAIN:
    value_set_analysis.output(ns, goto_functions, std::cout);
    break;
      
  default:;
  }
}

/*******************************************************************\

Function: show_value_sets

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void show_value_sets(
  ui_message_handlert::uit ui,
  const goto_programt &goto_program,
  value_set_analysist &value_set_analysis,
  const namespacet &ns)
{
  switch(ui)
  {
  case ui_message_handlert::XML_UI:
    {
      xmlt xml;
      value_set_analysis.convert(ns, goto_program, xml);
      std::cout << xml << std::endl;
    }
    break;
    
  case ui_message_handlert::PLAIN:
    value_set_analysis.output(ns, goto_program, std::cout);
    break;
      
  default:;
  }
}
