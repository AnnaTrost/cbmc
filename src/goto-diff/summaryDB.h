/*
 * summaryDB.h
 *
 *  Created on: Mar 17, 2017
 *      Author: annat
 */

#ifndef SRC_GOTO_DIFF_SUMMARYDB_H_
#define SRC_GOTO_DIFF_SUMMARYDB_H_

#include <map>
#include <goto-programs/goto_model.h>
#include "summary.h"

using std::map;

class SummaryDBt
{
private:
  SummaryDBt(const namespacet &ns_old, const namespacet &ns_new,
      const goto_functionst &old_goto_functions,
      const goto_functionst &new_goto_functions);
  virtual ~SummaryDBt();


  map<irep_idt, summaryt> old_summaries;
  map<irep_idt, summaryt> new_summaries;

  const namespacet &ns_old;
  const namespacet &ns_new;

  const goto_functionst &old_goto_functions;
  const goto_functionst &new_goto_functions;

public:
  static SummaryDBt& getInstance()
  {
    assert (instance!=NULL);
    return *instance;
  }

  static void initialize(const namespacet &ns_old, const namespacet &ns_new,
      const goto_functionst &old_goto_functions,
      const goto_functionst &new_goto_functions) {
    instance=new SummaryDBt(ns_old, ns_new, old_goto_functions,
            new_goto_functions);
  }
  summaryt& get_summary(bool old, const irep_idt &function);
protected:
  static SummaryDBt *instance;
};

#endif /* SRC_GOTO_DIFF_SUMMARYDB_H_ */
