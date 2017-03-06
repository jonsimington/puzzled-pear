//////////////////////////////////////////////////////////////////////
/// @file state.hpp
/// @author Owen Chiaventone
/// @brief Chess state modelling, generation, and transitions
//////////////////////////////////////////////////////////////////////

#ifndef CPP_CLIENT_STATE_HPP
#define CPP_CLIENT_STATE_HPP

#include "../impl/chess.hpp"
#include "../game.hpp"
#include "../game_object.hpp"
#include "../move.hpp"
#include "../piece.hpp"
#include "../player.hpp"

#include "../../../joueur/src/base_ai.hpp"
#include "../../../joueur/src/attr_wrapper.hpp"

#include "action.hpp"
#include <iostream>

class State
{
public:
    // Create a state from the chess game
    State(const cpp_client::chess::Game &game);

    // The default copy constructor is fine, no need to override

    // Calculate valid actions
    std::vector<Action> available_actions(int player_id);

    // Apply an action and return a copy of the board
    State apply(const Action &action);

    // Tell if a player is in check
    bool in_check(int player_id);

    bool friend operator==(const State &lhs, const State &rhs);

private:
    bool is_clear(const Space &space);
    bool has_opponent_piece(Space space, int player_id);
    // Apply an action in place
    void mutate(const Action &action);

    // Just for quick checks. All real operations are on the player pieces vector
    // Contains piece codes with the same notation as used in the print_board example
    // White pieces are uppercase, black pieces are lowercase
    // A null character (0x00) is used for an empty space
    char m_collision_map[8][8];
    // Arrays of 2 - one for each player
    std::vector<PieceModel> m_player_pieces[2];
    castling_status_type m_castling_status[2];
    Space m_en_passant;

    // Calculates all actions allowed by traditional moves of chess
    // Including actions that could put the player in check
    std::vector<Action> all_actions(int player_id);

    void straight_line_moves(const PieceModel &piece, std::vector<Space> directions, int player_id,
                             std::vector<Action> &actions);
};


#endif //CPP_CLIENT_STATE_HPP
