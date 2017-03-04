#include "action.hpp"

#include <map>

std::map<std::string, piece_type> piece_type_lookup = {
        {"Pawn",   PAWN},
        {"Rook",   ROOK},
        {"Knight", KNIGHT},
        {"Bishop", BISHOP},
        {"Queen",  QUEEN},
        {"King", KING}
};

void Action::execute() {
    // Convert location back from zero-indexed
    auto file = std::string(1, 'a' + char(m_space.file));
    auto rank = m_space.file + 1;
    m_piece.parent->move(file, rank);
}

std::vector<Action> BoardModel::available_actions() {
    std::vector<Action> actions;

    for (auto &piece : m_player_pieces) {
        if(piece.type == PAWN)
        {
                Space space_ahead = piece.location + m_forward;
                if (is_clear(space_ahead)) {
                    actions.push_back(Action(piece, space_ahead));
                }

                Space left = {0,-1}, right = {0, 1};
                Space attack_spaces[] = {piece.location + m_forward + left,
                                         piece.location + m_forward + right};
                for (int i = 0; i < 2; i++) {
                    if (has_opponent_piece(attack_spaces[i])) {
                        actions.push_back(Action(piece, attack_spaces[i]));
                    }
                }

                // En passant

                // Promotion
        } else
        {
            std::cout << "Warning: " << piece.parent->type << " moves not yet implemented." << std::endl;
        }

        // Pawns go forward 2 on first move

        // Castling
        /*
        if(piece->type == "King")
        {
            // If king is in original position and hasn't moved
            // If rook is in original position and hasn't moved
            // If spaces between king and rook are clear
        }
         */

        // En Passant
    }

    return actions;
}

BoardModel::BoardModel(const cpp_client::chess::Game& game) : m_collision_map() {
    // If player is white
    if(game->current_player->id == "0")
    {
        m_forward = {1, 0};
    } else
    {
        m_forward = {-1, 0};
    }

    for (const auto &piece: game->pieces) {
        PieceModel piecemodel(piece);
        int rank = piecemodel.location.rank;
        int file = piecemodel.location.file;
        m_collision_map[rank][file] = piece_type_lookup[piece->type];

        if (piece->owner->id == game->current_player->id) {
            m_player_pieces.push_back(piecemodel);
        } else {
            m_opponent_pieces.push_back(piecemodel);
        }
    }
}

bool BoardModel::is_clear(const Space& space) {
    if((space.rank > 7) or (space.rank < 0)
       or (space.file > 7) or (space.file < 0));
    return m_collision_map[space.rank][space.file] ? false : true;
}

bool BoardModel::has_opponent_piece(Space space) {
    // TODO:
    return false;
}

Space operator+(const Space &lhs, const Space &rhs) {
    return {lhs.rank + rhs.rank, lhs.file + rhs.file};
}

PieceModel::PieceModel(cpp_client::chess::Piece piece) {
    parent = piece;
    type = piece_type_lookup[piece->type];
    // Convert location to 0-indexed
    auto rank = piece->rank;
    auto file = piece->file;
    location = {rank - 1, file[0] - 'a'};
}