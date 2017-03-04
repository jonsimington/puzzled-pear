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

enum piece_type {
    PAWN = 1, ROOK, KNIGHT, BISHOP, QUEEN, KING
};

struct Space {
    int rank;
    int file;
};

Space operator+(const Space &lhs, const Space &rhs);

class PieceModel {
public:
    PieceModel(cpp_client::chess::Piece piece);
    cpp_client::chess::Piece parent;
    piece_type type;
    Space location;
};

class Action {
public:
    Action(PieceModel piece, Space &space) : m_piece(piece), m_space(space) {};
    PieceModel m_piece;
    Space m_space;

    void execute();
};

class BoardModel {
public:
    BoardModel(const cpp_client::chess::Game& game);

    std::vector<Action> available_actions();

    void apply(Action action);

private:
    bool is_clear(const Space& space);
    bool has_opponent_piece(Space space);

    Space m_forward; // Forward means different things to white and black players
    piece_type m_collision_map[8][8];
    std::vector<PieceModel> m_player_pieces;
    std::vector<PieceModel> m_opponent_pieces;

};

#endif //CPP_CLIENT_ACTION_HPP_H

