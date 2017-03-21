//
// Created by owen on 3/15/17.
//

#include "depth_limited_minimax.hpp"

#define INFINITY INT32_MAX //Ehh, close enough

Action depth_limited_minimax_search(const State &state, int depth_limit)
{
    int active_player = state.get_active_player();
    auto actions = state.available_actions(active_player);
    int best_action_score = -1, best_action_index = 0;
    assert(actions.size() > 0);
    std::cout << "Available actions: " << actions.size() << "\nActions Explored: " << std::flush;
    for (int i = 0; i < actions.size(); i++)
    {
        std::cout  << i << " " << std::flush;
        int score = dlmm_minv(state.apply(actions[i]), active_player, depth_limit - 1);
        if ((score > best_action_score)
            or ((score == best_action_score)
                && (random() % 2 == 0)))
        {
            best_action_score = score;
            best_action_index = i;
        }
        assert(best_action_score != -1);
        // If there's a tie, 50% chance of replacement
        // Statistically, this implementation might favor later
        // generated actions instead of being completely fair.
        // TODO: Check this out.
    }
    std::cout << std::endl;
    return (actions[best_action_index]);
}

int dlmm_minv(const State &state, int max_player_id, int depth_limit)
{
    assert(state.get_active_player() != max_player_id);

    if (depth_limit <= 0)
    {
        return state.heuristic_eval(max_player_id);
    }

    auto actions = state.available_actions(state.get_active_player());
    // Check for terminal state
    if(actions.size() <= 0)
    {
        return state.heuristic_eval(max_player_id); // Maybe this should be something special for checkmate?
    }
    int best_action_score = INFINITY; //Trying to minimize, so start out with +inf and reduce
    for (const auto &action : actions)
    {
        int score = dlmm_maxv(state.apply(action), max_player_id, depth_limit - 1);
        if ((score < best_action_score)
            or ((score == best_action_score)
                && (random() % 2 == 0)))
        {
            best_action_score = score;
        }
        assert(best_action_score < INFINITY);
    }
    return best_action_score;
}

int dlmm_maxv(const State &state, int max_player_id, int depth_limit)
{
    assert(state.get_active_player() == max_player_id);

    if (depth_limit <= 0)
    {
        return state.heuristic_eval(max_player_id);
    }

    auto actions = state.available_actions(state.get_active_player());
    if(actions.size() <= 0)
    {
        return state.heuristic_eval(max_player_id); // Maybe this should be something special for checkmate?
    }

    int best_action_score = -1;
    for (const auto &action : actions)
    {
        int score = dlmm_minv(state.apply(action), max_player_id, depth_limit - 1);
        assert(score != -1); // If this trips, it means a terminal condition has not been detected
        if ((score > best_action_score)
            or ((score == best_action_score)
                && (random() % 2 == 0)))
        {
            best_action_score = score;
        }
    }
    return best_action_score;
}
