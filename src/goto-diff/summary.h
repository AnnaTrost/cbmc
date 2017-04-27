
#ifndef SRC_GOTO_DIFF_SUMMARY_H_
#define SRC_GOTO_DIFF_SUMMARY_H_

#include <set>
#include <iostream>

#include <path-symex/path_symex_state.h>
#include <util/namespace.h>

class summaryt
{
public:
  typedef path_symex_statet statet;
	summaryt(const namespacet &ns, const irep_idt &function_name,
			const goto_programt &goto_program, const goto_functionst &goto_functions);
  virtual ~summaryt();
  void add_path(statet &state, bool is_terminating=true);
  void output(std::ostream &out) const;
  std::set<exprt> get_inputs() const;
  std::set<exprt> get_outputs() const;
  exprt get_formula() const;
  std::vector<exprt>& get_paths() { return paths;}
  exprt& get_uncovered() { return uncovered;}


  const namespacet &ns;
private:
  irep_idt function_name;
  const goto_functionst &goto_functions;
//  const value_setst &value_sets;

  exprt uncovered;
  exprt formula;
  std::vector<exprt> paths;
  std::set<exprt> inputs;
  std::set<exprt> global_outputs;

  std::set<irep_idt> called_functions;

  exprt get_path_summary(statet &state);
  exprt get_reduced_path_summary(statet &state);
  exprt negate_path(statet& path_sum);
  void collect_inputs_outputs(const goto_programt &goto_program);
  void collect_inputs(const exprt &e);
  void collect_prints(const irep_idt& id);

  void add_outputs(exprt e, bool in_print);

  void fix_outputs(exprt& e, const irep_idt &output_name, const typet &type);
  void enumerate_path(exprt& e);

  int find_max(exprt& e, const irep_idt &output_name, int max);

};

#endif /* SRC_GOTO_DIFF_SUMMARY_H_ */
