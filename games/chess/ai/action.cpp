#include "action.hpp"

#include <map>

std::map<std::string, piece_type> piece_type_lookup {
        {"Pawn",   PAWN},
        {"Rook",   ROOK},
        {"Knight", KNIGHT},
        {"Bishop", BISHOP},
        {"Queen",  QUEEN},
        {"King", KING}
};

std::vector<Space> KNIGHT_MOVES {
        {2,1},
        {1,2},
        {-1,2},
        {-2,1},
        {-1,-2},
        {-2,-1},
        {1,-2},
        {2,-1}
};

// Accessed as PAWN_START_RANK[player_id]
int PAWN_START_RANK[] = {1, 6};

void Action::execute() {
    // Convert location back from zero-indexed
    auto file = std::string(1, 'a' + char(m_space.file));
    auto rank = m_space.rank + 1;
    m_piece.parent->move(file, rank);
}

std::vector<Action> State::available_actions(int player_id) {
    std::vector<Action> actions;
    //std::vector<Action> valid_actions;

    for (auto &piece : m_player_pieces[player_id]) {
        if(piece.type == PAWN)
        {
                // Regular Moves
                bool in_original_space = (piece.location.rank == PAWN_START_RANK[player_id]);
                Space space_ahead = piece.location + m_forward;
                if (is_clear(space_ahead))
                {
                    actions.push_back(Action(piece, space_ahead));
                    if( in_original_space and is_clear(space_ahead + m_forward))
                    {
                        actions.push_back(Action(piece, space_ahead + m_forward));
                    }
                }

                // Attacks
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
        } else if (piece.type == KNIGHT)
        {
            for(auto& offset : KNIGHT_MOVES)
            {
                auto space = piece.location + offset;
                if(is_clear(space) or has_opponent_piece(space))
                {
                    actions.push_back(Action(piece, space));
                }
            }
        }
        else
        {
            //std::cout << "Warning: " << piece.parent->type << " moves not yet implemented." << std::endl;
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

    // See if the move would put us in check
    /*
    for(auto& action : actions)
    {
        // Copy board
        auto new_state = this->apply(action);
        // Apply action
        if(new_state.in_check() == false)
        {
            valid_actions.push_back()
        }
    }
     */

    return actions;
}

State::State(const cpp_client::chess::Game& game) : m_collision_map() {
    // If player is white
    int player_id = game->current_player->id[0] - '0';
    if(player_id == 0)
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

        int piece_owner = piece->owner->id[0] - '0';
        m_player_pieces[piece_owner].push_back(piecemodel);
        m_collision_map[rank][file] = (piece_owner == player_id) ? FRIEND : ENEMY;
    }
}

bool State::is_clear(const Space& space) {
    if ((space.rank > 7) or (space.rank < 0)
        or (space.file > 7) or (space.file < 0)) {
        return false;
    } else {
        return m_collision_map[space.rank][space.file] == EMPTY;
    }
}

bool State::has_opponent_piece(Space space) {
    if ((space.rank > 7) or (space.rank < 0)
        or (space.file > 7) or (space.file < 0)) {
        return false;
    } else {
        return m_collision_map[space.rank][space.file] == ENEMY;
    }

}

/*
State State::apply(Action action) {
    State copy = *this;   // I'm hoping and praying that c++ is smart enough
                          // To generate a default copy constructor that deep copies
                          // But in my heart I know that this line will break something
                          // and cost me hours to fix
    copy.mutate(action);
    return copy;
}

void State::mutate(Action action) {
    // Update the board


    // Update the piece in the list of player pieces
}
*/

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