/*
 * path_searchDB.h
 *
 *  Created on: Mar 17, 2017
 *      Author: annat
 */

#ifndef SRC_GOTO_DIFF_PATH_SEARCHDB_H_
#define SRC_GOTO_DIFF_PATH_SEARCHDB_H_

#include <map>
#include <goto-programs/goto_model.h>
#include "path_search.h"

class Path_searchDBt
{
  Path_searchDBt(const namespacet &ns_old, const namespacet &ns_new,
      const goto_functionst &old_goto_functions,
      const goto_functionst &new_goto_functions);
  virtual ~Path_searchDBt();

  static Path_searchDBt *instance;

  map<irep_idt, path_searcht> old_path_searches;
  map<irep_idt, path_searcht> new_path_searches;

  const namespacet &ns_old;
  const namespacet &ns_new;

  const goto_functionst &old_goto_functions;
  const goto_functionst &new_goto_functions;

public:
  static Path_searchDBt& getInstance()
  {
    assert (instance!=NULL);
    return *instance;
  }

  void initialize(const namespacet &ns_old, const namespacet &ns_new,
      const goto_functionst &old_goto_functions,
      const goto_functionst &new_goto_functions);
  path_searcht& get_path_search(bool old, const irep_idt &function);

};
Path_searchDBt *Path_searchDBt::instance = NULL;

#endif /* SRC_GOTO_DIFF_PATH_SEARCHDB_H_ */
