//
// Created by owen on 3/14/17.
//

#include "state.hpp"

// Relative weights of different parts of heuristic
const int WEIGHT_PIECES_OWNED = 5;
const int WEIGHT_PIECES_CAN_CAPTURE = 3;

// Values assigned to situations and actions
const int IN_CHECK_VALUE = -100; // Being in check is not good. Negative points.

const std::map<char, int> PIECE_VALUE{
    {'P', 1},
    {'R', 5},
    {'N', 3},
    {'B', 3},
    {'Q', 9},
    {'K', 0}, // Can't be taken
};

int State::heuristic_eval(int player_id)
{
    assert((player_id == 0) | (player_id == 1));
    int score = 0;

    // Add pieces owned by the player
    for (const auto& piece: m_player_pieces[player_id]) {
        score += WEIGHT_PIECES_OWNED * PIECE_VALUE.at(piece.type);
    }

    // Add pieces that the player can capture
    for (const auto& action : this->all_actions(player_id)) {
        if(action.m_target_piece != 0) {
            char piece_type = (char) toupper(action.m_target_piece);
            score += WEIGHT_PIECES_CAN_CAPTURE * PIECE_VALUE.at(piece_type);
        }
    }
}