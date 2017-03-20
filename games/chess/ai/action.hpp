//////////////////////////////////////////////////////////////////////
/// @file action.hpp
/// @author Owen Chiaventone
/// @brief Basic model definitions for chess
//////////////////////////////////////////////////////////////////////

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
#include <map>

extern std::map<std::string, char> PIECE_CODE_LOOKUP;

enum castling_status_type
{
    CASTLE_NONE,
    CASTLE_KINGSIDE,
    CASTLE_QUEENSIDE,
    CASTLE_BOTH
};

// Zero-indexed way of representing rank and file
struct Space
{
    int rank;
    int file;
};

Space operator+(const Space &lhs, const Space &rhs);

bool operator==(const Space &lhs, const Space &rhs);

class PieceModel
{
public:
    PieceModel(cpp_client::chess::Piece piece);

    cpp_client::chess::Piece_* parent;
    char type; // Always an uppercase one character piece code
    Space location;
};

class Action
{
public:
    Action(PieceModel piece,
           Space space,
           char target_piece = 0,
           std::string promotion = "",
           castling_status_type castle = CASTLE_NONE)
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

    friend std::ostream &operator<<(std::ostream &os, const Action &rhs);
};

#endif //CPP_CLIENT_ACTION_HPP_H

