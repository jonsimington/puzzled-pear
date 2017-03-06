#ifndef CPP_CLIENT_ACTION_HPP_H
#define CPP_CLIENT_ACTION_HPP_H

#include "../impl/chess.hpp"
#include "../game.hpp"
#include "../game_object.hpp"
#include "../move.hpp"
#include "../piece.hpp"
#include "../player.hpp"

#include "../../../joueur/src/base_ai.hpp"
#include "../../../joueur/src/attr_wrapper.hpp"

#include <iostream>

enum castling_status_type {
    CASTLE_NONE,
    CASTLE_KINGSIDE,
    CASTLE_QUEENSIDE,
    CASTLE_BOTH
};

// Zero-indexed way of representing rank and file
struct Space {
    int rank;
    int file;
};

Space operator+ (const Space& lhs, const Space& rhs);
bool  operator==(const Space& lhs, const Space& rhs);

class PieceModel {
public:
    PieceModel(cpp_client::chess::Piece piece);
    cpp_client::chess::Piece parent;
    char type; // Always an uppercase one character piece code
    Space location;
};

class Action {
public:
    Action(PieceModel piece,
           Space space,
           char target_piece=0,
           std::string promotion="",
           castling_status_type castle=CASTLE_NONE)
            : m_piece(piece),
              m_space(space),
              m_target_piece(target_piece),
              m_promotion(promotion),
              m_castle(castle)
                 {};
    PieceModel m_piece;
    Space m_space;
    char m_target_piece; // 0 for none
    std::string m_promotion;
    castling_status_type m_castle;
    void execute();
    friend std::ostream& operator << (std::ostream& os, const Action& rhs);
};

class State {
public:
    State(const cpp_client::chess::Game& game); // Normal constructor
    // The default copy constructor is fine, no need to override

    // Calculate valid actions
    std::vector<Action> available_actions(int player_id);

    // Apply an action and return a copy of the board
    State apply(const Action& action);

    // Tell if a player is in check
    bool in_check(int player_id);

    bool friend operator == (const State& lhs, const State& rhs);

private:
    bool is_clear(const Space& space);
    bool has_opponent_piece(Space space, int player_id);
    // Apply an action in place
    void mutate(const Action& action);

    // Just for quick checks. All real operations are on the player pieces vector
    // Contains piece codes with the same notation as used in the print_board example
    // White pieces are uppercase, black pieces are lowercase
    // A null character (0x00) is used for an empty space
    char m_collision_map[8][8];
    // Arrays of 2 - one for each player
    std::vector<PieceModel> m_player_pieces[2];
    castling_status_type m_castling_status[2];

    // Calculates all actions allowed by traditional moves of chess
    // Including actions that could put the player in check
    std::vector<Action> all_actions(int player_id);

    void straight_line_moves(const PieceModel &piece, std::vector<Space> directions, int player_id,
                             std::vector<Action> &actions);
};

#endif //CPP_CLIENT_ACTION_HPP_H

