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

using std::set;
using std::string;
using std::vector;

class differential_summaryt {
public:
  typedef enum {ADDED,REMOVED,CHANGED,AFFECTED,UNAFFECTED} typet;
  differential_summaryt(typet type=UNAFFECTED);
//  exprt get_summary_old();
//  exprt get_summary_new();
//  exprt get_change_over_approx();
  typet get_type() { return type; }
  bool operator==(const differential_summaryt &other) const;
  bool operator!=(const differential_summaryt &other) const;

  void output(std::ostream &out) const;

protected:
  typet type;
  vector<path_symex_historyt> changed;
  exprt unchanged;

  static string typet2string(const typet& type);
};




#endif /* GOTO_DIFF_DIFFERENTIAL_SUMMARY_H_ */
