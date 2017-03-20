#ifndef CPP_CLIENT_DEPTH_LIMITED_MINIMAX_HPP
#define CPP_CLIENT_DEPTH_LIMITED_MINIMAX_HPP

#include "state.hpp"
#include "action.hpp"

using move_val_pair = std::tuple<Action, int>;

// Returns the best action for the active player
Action depth_limited_minimax_search(const State& state, int depth_limit);

// Find the value of the objective function
// if min player takes the move here that is worst for max player
//
// @pre only called on min player's turn
//
// keeping track of the max player's ID is necessary to
// know who to calculate the state eval function for.
int dlmm_minv(const State& state, int max_player_id, int depth_limit);

// Find the move for max player that maximizes objective function
//
// @pre only called on max player's turn
int dlmm_maxv(const State& state, int max_player_id, int depth_limit);

#endif //CPP_CLIENT_DEPTH_LIMITED_MINIMAX_HPP
