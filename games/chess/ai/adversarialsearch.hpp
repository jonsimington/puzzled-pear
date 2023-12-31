#ifndef CPP_CLIENT_DEPTH_LIMITED_MINIMAX_HPP
#define CPP_CLIENT_DEPTH_LIMITED_MINIMAX_HPP

#include "state.hpp"
#include "action.hpp"
#include "hash.hpp"

using move_val_pair = std::tuple<Action, int>;

class AdversarialSearch {
 public:
  AdversarialSearch(std::unordered_map<Action, int> *history_table, std::unordered_map<long, int> *transposition_table)
      : m_history_table(history_table), m_transposition_table(transposition_table) {};

  // Returns the best action for the active player
  Action depth_limited_minimax_search(const State &state, int depth_limit, int quiescence_limit);

  // Find the value of the objective function
  // if min player takes the move here that is worst for max player
  //
  // @pre only called on min player's turn
  //
  // keeping track of the max player's ID is necessary to
  // know who to calculate the state eval function for.
  int dlmm_minv(const State &state, int max_player_id, int depth_limit, int quiescence_limit, int alpha, int beta);

  // Find the move for max player that maximizes objective function
  //
  // @pre only called on max player's turn
  int dlmm_maxv(const State &state, int max_player_id, int depth_limit, int quiescence_limit, int alpha, int beta);
 private:
  std::unordered_map<Action, int> *m_history_table;
  std::unordered_map<long, int> *m_transposition_table;
  std::vector<Action> history_table_sort(const std::vector<Action> &actions) const;
  int transposition_table_heuristic(const State& state, int max_player_id);
  void history_table_update(const Action &action);
};

#endif //CPP_CLIENT_DEPTH_LIMITED_MINIMAX_HPP
