/*******************************************************************\

Module: Show Value Sets

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#ifndef CPROVER_GOTO_PROGRAMS_SHOW_VALUE_SETS_H
#define CPROVER_GOTO_PROGRAMS_SHOW_VALUE_SETS_H

#include <util/ui_message.h>

class goto_functionst;
class goto_programt;
class value_set_analysist;

void show_value_sets(
  ui_message_handlert::uit ui,
  const goto_functionst &goto_functions,
  value_set_analysist &value_set_analysis,
  const namespacet &ns);

void show_value_sets(
  ui_message_handlert::uit ui,
  const goto_programt &goto_program,
  value_set_analysist &value_set_analysis,
  const namespacet &ns);

#endif
