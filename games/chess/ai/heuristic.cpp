//
// Created by owen on 3/14/17.
//

#include "state.hpp"

// Relative weights of different parts of heuristic
const int WEIGHT_PIECES_OWNED = 20;
const int WEIGHT_PIECES_CAN_CAPTURE = 3;
const int WEIGHT_OPPONENT_PIECES = -20;

// Values assigned to situations and actions
const int IN_CHECK_VALUE = 50;

const std::map<char, int> PIECE_VALUE{
    {'P', 1},
    {'R', 5},
    {'N', 3},
    {'B', 3},
    {'Q', 9},
    {'K', 0}, // Can't be taken
};

int State::heuristic_eval(int player_id) const
{
    assert((player_id == 0) | (player_id == 1));
    int opponent_id = (player_id == 0 ? 1 : 0);
    int score = 0;

    // Add pieces owned by the player
    for (const auto &piece: m_player_pieces[player_id])
    {
        score += WEIGHT_PIECES_OWNED * PIECE_VALUE.at(piece.type);
    }

    for (const auto &piece: m_player_pieces[opponent_id])
    {
        score += WEIGHT_OPPONENT_PIECES * PIECE_VALUE.at(piece.type);
    }

    // Add pieces threatened by the player
    for (const auto &action : this->available_actions(player_id))
    {
        if (action.m_target_piece != 0)
        {
            char piece_type = (char) toupper(action.m_target_piece);
            score += WEIGHT_PIECES_CAN_CAPTURE * PIECE_VALUE.at(piece_type);
        }
    }

    // Being in check is bad. Putting the other player in check is good.
    if (in_check(player_id))
    {
        score -= IN_CHECK_VALUE;
    }

    if (in_check(opponent_id))
    {
        score += IN_CHECK_VALUE;
    }

    return score;
}