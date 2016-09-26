/*******************************************************************\

Module: Semantic diff

Author: Anna Trostanetski

Date: August 2016

\*******************************************************************/

#ifndef CPROVER_SEMANTIC_DIFF_H
#define CPROVER_SEMANTIC_DIFF_H

class goto_modelt;

void semantic_diff(
  const goto_modelt &model_old,
  const goto_modelt &model_new);

#endif
