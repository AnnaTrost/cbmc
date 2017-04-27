/*
 * path_searchDB.cpp
 *
 *  Created on: Mar 17, 2017
 *      Author: annat
 */

#include "path_searchDB.h"

Path_searchDBt::Path_searchDBt(const namespacet &_ns_old,
    const namespacet &_ns_new,
    const goto_functionst &_old_goto_functions,
    const goto_functionst &_new_goto_functions) :
    ns_old(_ns_old), ns_new(_ns_new), old_goto_functions(
        _old_goto_functions), new_goto_functions(_new_goto_functions)
{
}

Path_searchDBt::~Path_searchDBt()
{
}

void Path_searchDBt::initialize(const namespacet &ns_old,
    const namespacet &ns_new,
    const goto_functionst &old_goto_functions,
    const goto_functionst &new_goto_functions)
{
  if(instance==NULL)
    instance=new Path_searchDBt(ns_old, ns_new, old_goto_functions,
        new_goto_functions);
}

path_searcht& Path_searchDBt::get_path_search(bool old, const irep_idt &function)
{
  map<irep_idt, path_searcht>& path_searches=
      old ? old_path_searches : new_path_searches;
  auto it=path_searches.find(function);
  if(it!=path_searches.end())
    return it->second;

  const namespacet &ns=old ? ns_old : ns_new;
  const goto_functionst &goto_functions=
      old ? old_goto_functions : new_goto_functions;

  auto res=path_searches.insert(
      std::make_pair(function,
          path_searcht(ns)));
  return res.first->second;
}
