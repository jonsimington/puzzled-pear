#include "action.hpp"
#include <memory>

#include <map>

std::map<std::string, char> PIECE_CODE_LOOKUP {
        {"Pawn",   'P'},
        {"Rook",   'R'},
        {"Knight", 'N'},
        {"Bishop", 'B'},
        {"Queen",  'Q'},
        {"King",   'K'}
};

// Vectors representing piece move offsets
std::vector<Space> KNIGHT_MOVES {
        { 2,   1},
        { 1 ,  2},
        {-1 ,  2},
        {-2 ,  1},
        {-1 , -2},
        {-2 , -1},
        { 1 , -2},
        { 2 , -1}
};

// And all multiples
std::vector<Space> BISHOP_MOVES {
        {-1 , -1},
        {-1 ,  1},
        { 1 , -1},
        { 1 ,  1}
};

std::vector<Space> ROOK_MOVES {
        { 1 ,  0},
        {-1 ,  0},
        { 0 ,  1},
        { 0 , -1}
};

// King and Queen can move the same directions, just different amounts
std::vector<Space> ROYAL_MOVES {
        {-1 , -1},
        {-1 ,  1},
        { 1 , -1},
        { 1 ,  1},
        { 1 ,  0},
        {-1 ,  0},
        { 0 ,  1},
        { 0 , -1}
};

std::vector<std::string> POSSIBLE_PROMOTIONS {
        "Queen",
        "Bishop",
        "Knight",
        "Rook"
};

// Accessed as PAWN_START_RANK[player_id]
int PAWN_START_RANK[] = {1, 6};

void Action::execute() {
    // Convert location back from zero-indexed
    auto file = std::string(1, 'a' + char(m_space.file));
    auto rank = m_space.rank + 1;
    m_piece.parent->move(file, rank, m_promotion);
}

std::ostream &operator<<(std::ostream &os, const Action &rhs) {
    os << rhs.m_piece.parent->type
       << " at " << rhs.m_piece.parent->file << rhs.m_piece.parent->rank << " to ";
    if(rhs.m_target_piece != 0)
        os << "capture " << rhs.m_target_piece << " @ ";
    os << char(rhs.m_space.file + 'a') << rhs.m_space.rank + 1;
    return os;
}

std::vector<Action> State::available_actions(int player_id) {

    auto possible_actions = all_actions(player_id);
    std::vector<Action> valid_actions;

    // filter to actions that don't result in going into check
    for(auto& action : possible_actions)
    {
        // Copy board
        auto new_state = this->apply(action);
        // Apply action
        if(new_state.in_check(player_id) == false)
        {
            valid_actions.push_back(action);

        } else
        {
            std::cout << "Action " << action << " Discarded because it would put player in check" << std::endl;
        }
    }

    return valid_actions;
}

// Calculates moves in straight lines from the given piece
// Straight lines are defined as multiples of directions
// post: Moves added to actions
void State::straight_line_moves(const PieceModel &piece, std::vector<Space> directions, int player_id,
                                std::vector<Action> &actions) {
    for(auto& direction : directions)
    {
        Space space = piece.location + direction;
        while(true)
        {
            if (is_clear(space) or has_opponent_piece(space, player_id))
            {
                char target_piece = m_collision_map[space.rank][space.file];
                actions.push_back(Action(piece, space, target_piece));
            }
            if (!is_clear(space)) break;
            space = space + direction;
        }
    }
}

State::State(const cpp_client::chess::Game& game) : m_collision_map() {
    // If player is white
    int player_id = game->current_player->id[0] - '0';

    for (const auto &piece: game->pieces) {
        PieceModel piecemodel(piece);
        int rank = piecemodel.location.rank;
        int file = piecemodel.location.file;

        int piece_owner = piece->owner->id[0] - '0';
        m_player_pieces[piece_owner].push_back(piecemodel);

        char piece_code = PIECE_CODE_LOOKUP[piece->type];
        if(piece_owner == 1) piece_code = tolower(piece_code);
        m_collision_map[rank][file] = piece_code;
    }
}

bool State::is_clear(const Space& space) {
    if ((space.rank > 7) or (space.rank < 0)
        or (space.file > 7) or (space.file < 0)) {
        return false;
    } else {
        return m_collision_map[space.rank][space.file] == 0;
    }
}

bool State::has_opponent_piece(Space space, int player_id) {
    if ((space.rank > 7) or (space.rank < 0)
        or (space.file > 7) or (space.file < 0)) {
        return false;
    } else {
        char piece_code = m_collision_map[space.rank][space.file];
        // Black's pieces are lowercase, White's pieces are uppercase
        if(player_id == 0) {
            return (('a' <= piece_code) and (piece_code <= 'z'));
        } else
        {
            return (('A' <= piece_code) and (piece_code <= 'Z'));
        }
    }

}

State State::apply(const Action& action) {
    State copy = *this;   // I'm hoping and praying that c++ is smart enough
                          // To generate a default copy constructor that deep copies
                          // But in my heart I know that this line will break something
                          // and cost me hours to fix
    copy.mutate(action);
    return copy;
}

void State::mutate(const Action& action) {
    // Update the board
    int player_id  = action.m_piece.parent->owner->id[0] - '0';
    const Space& from = action.m_piece.location;
    const Space& to   = action.m_space;
    m_collision_map[from.rank][from.file] = 0;
    char piece_code = player_id == 0 ? action.m_piece.type : tolower(action.m_piece.type);
    m_collision_map[to.rank][to.file] = piece_code;

    // If the move takes a piece, delete it from the list
    // The collision map doesn't store links,
    // so we have to iterate through the opponent's pieces until we find the
    // right one. In practice this isn't a problem because there are never more
    // than 16 opponent pieces, which is a small number.
    if(action.m_target_piece != 0)
    {
        for(int i = 0; i < m_player_pieces[player_id].size() - 1; i++)
        {
            auto& piece = m_player_pieces[player_id][i];
            if(piece.location.rank == to.rank && piece.location.file == to.file)
            {
                // Delete the piece from the list
                m_player_pieces[player_id].erase(m_player_pieces[player_id].begin() + i);
                break;
            }
        }
    }

    // Handle Pawn Promotion
    if((action.m_piece.type == 'p' or action.m_piece.type == 'P')
        and action.m_promotion != "")
    {
        for(auto& piece : m_player_pieces[player_id])
        {
            if(piece.location == action.m_piece.location)
            {
                piece.type = PIECE_CODE_LOOKUP[action.m_promotion];
                break;
            }
        }
    }

    // Update special variables, like en passant and castling

    // Update the piece in the list of player pieces
}

std::vector<Action> State::all_actions(int player_id) {
    assert(player_id == 0 or player_id == 1);

    std::vector<Action> actions;

    // maybe forward[player_id] would be better?
    Space forward = {1, 0};
    if(player_id == 1) forward = {-1, 0};

    for (auto &piece : m_player_pieces[player_id]) {
        if(piece.type == 'P')
        {
            // Regular Moves
            bool in_original_space = (piece.location.rank == PAWN_START_RANK[player_id]);
            Space space_ahead = piece.location + forward;
            bool can_promote = (space_ahead.rank == 7) or (space_ahead.rank== 0);
            if (is_clear(space_ahead))
            {
                // Promotion
                if(can_promote)
                {
                    for(auto& promotion_type : POSSIBLE_PROMOTIONS)
                    {
                        actions.push_back(Action(piece, space_ahead, 0, promotion_type));
                    }
                } else {
                    actions.push_back(Action(piece, space_ahead));
                    if (in_original_space and is_clear(space_ahead + forward)) {
                        actions.push_back(Action(piece, space_ahead + forward));
                    }
                }
            }

            // Attacks
            Space left = {0,-1}, right = {0, 1};
            Space attack_spaces[] = {piece.location + forward + left,
                                     piece.location + forward + right};
            for (int i = 0; i < 2; i++) {
                if (has_opponent_piece(attack_spaces[i], player_id)) {
                    char target = m_collision_map[attack_spaces[i].rank][attack_spaces[i].file];
                    actions.push_back(Action(piece, attack_spaces[i], target));
                }
            }


            // En passant


        } else if (piece.type == 'N')
        {
            for(auto& offset : KNIGHT_MOVES)
            {
                auto space = piece.location + offset;
                if(is_clear(space) or has_opponent_piece(space, player_id))
                {
                    char target = m_collision_map[space.rank][space.file];
                    actions.push_back(Action(piece, space, target));
                }
            }
        } else if (piece.type == 'R')
        {
            straight_line_moves(piece, ROOK_MOVES, player_id, actions);
        } else if (piece.type == 'B')
        {
            straight_line_moves(piece, BISHOP_MOVES, player_id, actions);
        } else if (piece.type == 'Q')
        {
            straight_line_moves(piece, ROYAL_MOVES, player_id, actions);
        } else if(piece.type == 'K')
        {
            for(auto& direction: ROYAL_MOVES)
            {
                Space space = piece.location + direction;
                if (is_clear(space) or has_opponent_piece(space, player_id))
                {
                    char target = m_collision_map[space.rank][space.file];
                    actions.push_back(Action(piece, space, target));
                }
            }
        }
        else
        {
            std::cout << "Warning: " << piece.parent->type << " moves not yet implemented." << std::endl;
        }

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

bool State::in_check(int player_id) {
    auto possible_opponent_actions = all_actions(1 - player_id);
    bool in_check = false;
    char king = player_id==0 ? 'K' : 'k';
    for(auto& action : possible_opponent_actions)
    {
        if(action.m_target_piece == king)
        {
            in_check = true;
            break;
        }
    }
    return in_check;
}

Space operator+(const Space &lhs, const Space &rhs) {
    return {lhs.rank + rhs.rank, lhs.file + rhs.file};
}

bool operator==(const Space &lhs, const Space &rhs) {
    return lhs.rank == rhs.rank && lhs.file == rhs.file;
}

PieceModel::PieceModel(cpp_client::chess::Piece piece) {
    parent = piece;
    type = PIECE_CODE_LOOKUP[piece->type];
    // Convert location to 0-indexed
    auto rank = piece->rank;
    auto file = piece->file;
    location = {rank - 1, file[0] - 'a'};
}