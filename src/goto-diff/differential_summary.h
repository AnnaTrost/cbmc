/*
 * differential_summary.h
 *
 *  Created on: Aug 15, 2016
 *      Author: annat
 */

#ifndef GOTO_DIFF_DIFFERENTIAL_SUMMARY_H_
#define GOTO_DIFF_DIFFERENTIAL_SUMMARY_H_

#include <set>
#include <string>
#include <vector>
#include <iostream>
#include <path-symex/path_symex_history.h>

#include "summary.h"

using std::set;
using std::string;
using std::vector;

class differential_summaryt {
  string old_prefix="old#";
  string new_prefix="new#";
public:
  typedef enum {ADDED,REMOVED,CHANGED,AFFECTED,UNAFFECTED} typet;
  differential_summaryt(typet type=UNAFFECTED);
//  exprt get_summary_old();
//  exprt get_summary_new();
//  exprt get_change_over_approx();
  void set_unaffected();
  typet get_type() { return type; }
  bool operator==(const differential_summaryt &other) const;
  bool operator!=(const differential_summaryt &other) const;

  void add_summary_old(summaryt& sum);
  void add_summary_new(summaryt& sum);

  bool recompute_diff();

  void output(std::ostream &out) const;

protected:
  typet type;
  exprt changed;
  exprt unchanged;
  summaryt* summary_old;
  summaryt* summary_new;
  symbol_tablet st;

  static string typet2string(const typet& type);
  void add_prefix(exprt &e, const string &prefix);
};

#endif /* GOTO_DIFF_DIFFERENTIAL_SUMMARY_H_ */
