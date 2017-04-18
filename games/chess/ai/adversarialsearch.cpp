//
// Created by owen on 3/15/17.
//

#include <algorithm>
#include "adversarialsearch.hpp"

#define INT_INFINITY INT32_MAX //Ehh, close enough

using tuple = std::pair<int, int>;
const int CHECKMATE_BASE_VAL = INT_INFINITY - 50; // Give some wiggle room for delay prevention

Action AdversarialSearch::depth_limited_minimax_search(const State &state, int depth_limit, int quiescence_limit) {
  int active_player = state.get_active_player();
  auto actions = state.available_actions(active_player);
  actions = history_table_sort(actions);
  assert(actions.size() > 0);
  std::vector<int> scores(actions.size());
  int alpha = -INT_INFINITY;
  int beta = INT_INFINITY;

  for (int i = 0; i < actions.size(); i++) {
    scores[i] = dlmm_minv(state.apply(actions[i]),
                          active_player,
                          depth_limit - 1,
                          quiescence_limit,
                          alpha,
                          beta);
    if (scores[i] > alpha) {
      alpha = scores[i];
    }
  }

  bool action_picked = false;
  int best_action_score = -INT_INFINITY, best_action_index = 0;
  for (int i = 0; i < scores.size(); i++) {
    if ((scores[i] > best_action_score)
        or ((scores[i] == best_action_score)
            && (random() % 2 == 0))) {
      best_action_score = scores[i];
      best_action_index = i;
      action_picked = true;
    }
    //assert(best_action_score != -INT_INFINITY);
    // If there's a tie, 50% chance of replacement
    // Statistically, this implementation might favor later
    // generated actions instead of being completely fair.
    // TODO: Check this out.
  }
  history_table_update(actions[best_action_index]);
  //assert(action_picked); This was triggering when checkmate was assured, so temporarily disabled
  return (actions[best_action_index]);
}

int AdversarialSearch::dlmm_minv(const State &state,
                                 int max_player_id,
                                 int depth_limit,
                                 int quiescence_limit,
                                 int alpha,
                                 int beta) {
  assert(state.get_active_player() != max_player_id);
  bool quiescent_search = false;

  if (depth_limit <= 0) {
    if (quiescence_limit > 0 && state.is_non_quiescent()) {
      quiescent_search = true;
    } else {
      return transposition_table_heuristic(state, max_player_id);
    }
  }

  auto actions = state.available_actions(state.get_active_player());
  actions = history_table_sort(actions);
  // Check for terminal state
  if (actions.size() <= 0) {
    int remaining_depth = depth_limit + quiescence_limit;
    return CHECKMATE_BASE_VAL + remaining_depth; // CHECKMATE! Victory. Earlier checkmates are better, so prefer those
  }
  int best_action_score = INT_INFINITY, best_action_index = 0; //Trying to minimize, so start out with +inf and reduce
  for (int i = 0; i < actions.size(); i++) {
    int score;
    if (quiescent_search) {
      score = dlmm_maxv(state.apply(actions[i]), max_player_id, depth_limit, quiescence_limit - 1, alpha, beta);
    } else {
      score = dlmm_maxv(state.apply(actions[i]), max_player_id, depth_limit - 1, quiescence_limit, alpha, beta);
    }

    // Check for a fail-low
    if (score <= alpha) {
      history_table_update(actions[i]);
      return score;
    }
    if (score < beta) {
      beta = score;
    }
    if ((score < best_action_score)
        or ((score == best_action_score)
            && (random() % 2 == 0))) {
      best_action_score = score;
      best_action_index = i;
    }
    //assert(best_action_score < INT_INFINITY);
  }
  history_table_update(actions[best_action_index]);
  return best_action_score;
}

int AdversarialSearch::dlmm_maxv(const State &state,
                                 int max_player_id,
                                 int depth_limit,
                                 int quiescence_limit,
                                 int alpha,
                                 int beta) {
  assert(state.get_active_player() == max_player_id);
  bool quiescent_search = false;

  if (depth_limit <= 0) {
    if (quiescence_limit > 0 && state.is_non_quiescent()) {
      quiescent_search = true;
    } else {
      return transposition_table_heuristic(state, max_player_id);
    }
  }

  auto actions = state.available_actions(state.get_active_player());
  actions = history_table_sort(actions);
  if (actions.size() <= 0) {
    return -INT_INFINITY; // CHECKMATE! Loss.
  }

  int best_action_score = -INT_INFINITY, best_action_index = 0;
  for (int i = 0; i < actions.size(); i++) {
    int score;
    if (quiescent_search) {
      score = dlmm_minv(state.apply(actions[i]), max_player_id, depth_limit, quiescence_limit - 1, alpha, beta);
    } else {
      score = dlmm_minv(state.apply(actions[i]), max_player_id, depth_limit - 1, quiescence_limit, alpha, beta);
    }

    // Check for fail-high
    if (score >= beta) {
      history_table_update(actions[i]);
      return score;
    }
    if (score > alpha) {
      alpha = score;
    }
    if ((score > best_action_score)
        or ((score == best_action_score)
            && (random() % 2 == 0))) {
      best_action_score = score;
      best_action_index = i;
    }
    //assert(best_action_score > -INT_INFINITY);
  }
  history_table_update(actions[best_action_index]);
  return best_action_score;
}

std::vector<Action> AdversarialSearch::history_table_sort(const std::vector<Action> &actions) const {
  // tuples contain <occurrence frequency, action index>
  // We load the frequency in, then sort them together
  std::vector<tuple> scores(actions.size());
  std::vector<Action> results(actions.size());
  for (int i = 0; i < actions.size(); i++) {
    scores[i].second = i;
    auto it = m_history_table->find(actions[i]);
    if (it != m_history_table->end()) {
      scores[i].first = it->second; //Load the
    } else {
      scores[i].first = 0;
    }
  }

  std::sort(scores.rbegin(), scores.rend());

  for (int i = 0; i < actions.size(); i++) {
    results[i] = actions[scores[i].second];
  }
  return results;
}

void AdversarialSearch::history_table_update(const Action &action) {
  auto it = m_history_table->find(action);
  if (it != m_history_table->end()) {
    it->second++;
  } else {
    m_history_table->insert(std::make_pair(action, 1));
  }
}

int AdversarialSearch::transposition_table_heuristic(const State &state, int max_player_id) {

  auto it = m_transposition_table->find(state.hash());
  if (it != m_transposition_table->end()) {
    return it->second;
  } else {
    int heuristic_val = state.heuristic_eval(max_player_id);
    m_transposition_table->insert(std::make_pair(state.hash(), heuristic_val));
    return heuristic_val;
  }
}