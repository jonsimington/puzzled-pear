#ifndef CPP_CLIENT_ACTION_HPP_H
#define CPP_CLIENT_ACTION_HPP_H

#include "../piece.hpp"

enum piece_type {PAWN=1, ROOK, KNIGHT, BISHOP, QUEEN, KING};

class Space
{
    int rank;
    char file;

    friend Space operator + (const Space& lhs, const Space& rhs);
};

class Action
{
public:
    Action(piece, space): m_piece(piece), m_space(space) {};
    chess::Piece m_piece;
    Space        m_space;
    void execute();

};

class BoardModel
{
public:
    BoardModel();
    BoardModel(std::string fen_string);
    void apply(Action action);
private:
    piece_type collision_map[8][8];
    bool is_clear(Space space);
    bool has_opponent_piece(Space space);
    std::vector<Piece> m_player_pieces;
    std::vector<Piece> m_opponent_pieces;
};

#endif //CPP_CLIENT_ACTION_HPP_H
