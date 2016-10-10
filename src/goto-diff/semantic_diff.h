/*******************************************************************\

Module: Semantic diff

Author: Anna Trostanetski

Date: August 2016

\*******************************************************************/

#ifndef CPROVER_SEMANTIC_DIFF_H
#define CPROVER_SEMANTIC_DIFF_H

class goto_modelt;

void semantic_diff(
  goto_modelt &model_old,
  goto_modelt &model_new);

#endif
