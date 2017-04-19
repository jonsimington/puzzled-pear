//
// Created by owen on 3/14/17.
//

#include "state.hpp"

// Relative weights of different parts of heuristic
const int WEIGHT_PIECES_OWNED = 25;
const int WEIGHT_PIECES_CAN_CAPTURE = 3;
const int WEIGHT_OPPONENT_PIECES = -20;
const int WEIGHT_PAWN_ADVANCEMENT = 2;
const int WEIGHT_GUARD_OWN_PIECES = 5;

// Values assigned to situations and actions
const int IN_CHECK_VALUE = 50;

const int KNIGHT_STRENGTH_TABLE[8][8]{
    {}
};

const std::map<char, int> PIECE_VALUE{
    {'P', 1},
    {'R', 5},
    {'N', 3},
    {'B', 3},
    {'Q', 9},
    {'K', 0}, // Can't be taken
};

int State::heuristic_eval(int player_id) const {
  assert((player_id == 0) | (player_id == 1));
  int opponent_id = (player_id == 0 ? 1 : 0);
  int score = 0;

  // Add pieces owned by the player
  for (const auto &piece: m_player_pieces[player_id]) {
    score += WEIGHT_PIECES_OWNED * PIECE_VALUE.at(piece.type);

    // Encourage pieces to guard other pieces
    if (space_threatened(piece.location, player_id)) {
      score += WEIGHT_GUARD_OWN_PIECES * PIECE_VALUE.at(piece.type);
    }

    // Try to get pawns staggered off the home row
    if (piece.type == 'P' && piece.location.file % 2) {
      int advancement = player_id == 0 ? piece.location.rank - 1 : 6 - piece.location.rank;
      score += WEIGHT_PAWN_ADVANCEMENT * advancement;
    }
  }

  for (const auto &piece: m_player_pieces[opponent_id]) {
    score += WEIGHT_OPPONENT_PIECES * PIECE_VALUE.at(piece.type);
  }

  // Add pieces threatened by the player
  for (const auto &piece : m_player_pieces[opponent_id]) {
    if (space_threatened(piece.location, player_id)) {
      score += WEIGHT_PIECES_CAN_CAPTURE * PIECE_VALUE.at(piece.type);
    }
  }

  return score;
}

bool State::is_non_quiescent() const {
  // Very very simple quiescence detection
  // if the last piece we moved is in danger, look deeper
  return space_threatened(m_last_move, m_active_player);
}