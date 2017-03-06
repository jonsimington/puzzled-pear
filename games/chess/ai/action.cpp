#include "action.hpp"

#include <map>
#include <sstream>

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

// Special values for Castling
const Space WHITE_KINGSIDE_ROOK_START    = {0,7};
const Space WHITE_KINGSIDE_ROOK_CASTLED  = {0,5};
const Space WHITE_QUEENSIDE_ROOK_START   = {0,0};
const Space WHITE_QUEENSIDE_ROOK_CASTLED = {0,3};
const Space BLACK_KINGSIDE_ROOK_START    = {7,7};
const Space BLACK_KINGSIDE_ROOK_CASTLED  = {7,5};
const Space BLACK_QUEENSIDE_ROOK_START   = {7,0};
const Space BLACK_QUEENSIDE_ROOK_CASLTED = {7,3};

const Space NO_EN_PASSANT = {-1,-1};

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
            //std::cout << "Action " << action << " Discarded because it would put player in check" << std::endl;
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
    // Read in pieces
    for (const auto &piece: game->pieces) {
        PieceModel piecemodel(piece);
        int rank = piecemodel.location.rank;
        int file = piecemodel.location.file;

        int piece_owner = piece->owner->id[0] - '0';
        m_player_pieces[piece_owner].push_back(piecemodel);

        char piece_code = PIECE_CODE_LOOKUP[piece->type];
        if(piece_owner == 1) piece_code = char(tolower(piece_code));
        m_collision_map[rank][file] = piece_code;
    }

    // Read in castling status and En Passant from FEN
    std::istringstream fen(game->fen);
    std::string piece_placement, active_color, castling_status, en_passant, clock, move;
    fen >> piece_placement >> active_color >> castling_status >> en_passant >> clock >> move;

    if(castling_status.find('K') != std::string::npos)
    {
        m_castling_status[0] = CASTLE_KINGSIDE;
    }
    if(castling_status.find('Q') != std::string::npos)
    {
        if(m_castling_status[0] == CASTLE_KINGSIDE) m_castling_status[0] = CASTLE_BOTH;
        else m_castling_status[0] = CASTLE_QUEENSIDE;
    }
    if(castling_status.find('k') != std::string::npos)
    {
        m_castling_status[1] = CASTLE_KINGSIDE;
    }
    if(castling_status.find('q') != std::string::npos)
    {
        if(m_castling_status[1] == CASTLE_KINGSIDE) m_castling_status[0] = CASTLE_BOTH;
        else m_castling_status[1] = CASTLE_QUEENSIDE;
    }

    if(en_passant == "-")
    {
        m_en_passant = NO_EN_PASSANT;
    } else{
        int file = en_passant[0] - 'a';
        int rank = en_passant[1] - 1;
        m_en_passant = {rank, file};
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
    int player_id = action.m_piece.parent->owner->id[0] - '0';
    const Space &from = action.m_piece.location;
    const Space &to = action.m_space;
    m_collision_map[from.rank][from.file] = 0;
    char piece_code = player_id == 0 ? action.m_piece.type : char(tolower(action.m_piece.type));
    m_collision_map[to.rank][to.file] = piece_code;

    // Update the moved piece in the list of player pieces
    // Searching this list is O(n), but n is small and the alternative
    // is introducing pointers that point to objects inside STL
    // containers (could segfault), or making the containers themselves
    // contain smart pointers and then defining a custom copy operation
    // that's super careful to get all of the smart pointer objects
    // deep copied.
    //
    // I don't feel like chasing memory leaks right now, so I'll take the hit and
    // do the O(n) search operation. It's safe. I'll change it if the profiler
    // starts complaining.
    //
    // I miss python. Why am I doing this to myself?
    for (auto &piece : m_player_pieces[player_id]) {
        if (piece.location == action.m_piece.location) {
            piece.location = action.m_space;
            assert(piece.type == action.m_piece.type);
            break;
        }
    }

    // If the move takes a piece, delete it from the list
    // The collision map doesn't store links,
    // so we have to iterate through the opponent's pieces until we find the
    // right one. In practice this isn't a problem because there are never more
    // than 16 opponent pieces, which is a small number.
    if (action.m_target_piece != 0) {
        for (int i = 0; i < m_player_pieces[player_id].size() - 1; i++) {
            auto &piece = m_player_pieces[player_id][i];
            if (piece.location.rank == to.rank && piece.location.file == to.file) {
                // Delete the piece from the list
                m_player_pieces[player_id].erase(m_player_pieces[player_id].begin() + i);
                break;
            }
        }
    }

    // Handle Pawn Promotion
    if ((action.m_piece.type == 'p' or action.m_piece.type == 'P')
        and action.m_promotion != "") {
        for (auto &piece : m_player_pieces[player_id]) {
            if (piece.location == action.m_piece.location) {
                piece.type = PIECE_CODE_LOOKUP[action.m_promotion];
                break;
            }
        }
    }

    // Apply Castling. Should already have been applied to the king, but we
    // need to get the rook now, too.
    if (action.m_castle != CASTLE_NONE) {
        Space rook_start, rook_finish;
        char rook_symbol = player_id == 0 ? 'R' : 'r';
        assert(action.m_castle == CASTLE_KINGSIDE or action.m_castle == CASTLE_QUEENSIDE);
        if (action.m_castle == CASTLE_KINGSIDE) {
            rook_start = player_id == 0 ? WHITE_KINGSIDE_ROOK_START : BLACK_KINGSIDE_ROOK_START;
            rook_finish = player_id == 0 ? WHITE_KINGSIDE_ROOK_CASTLED : BLACK_KINGSIDE_ROOK_CASTLED;
        } else if (action.m_castle == CASTLE_QUEENSIDE) {
            rook_start = player_id == 0 ? WHITE_QUEENSIDE_ROOK_START : BLACK_QUEENSIDE_ROOK_START;
            rook_finish = player_id == 0 ? WHITE_QUEENSIDE_ROOK_CASTLED : BLACK_QUEENSIDE_ROOK_CASLTED;
        }

        // Update board
        m_collision_map[rook_start.rank][rook_start.file] = 0;
        m_collision_map[rook_finish.rank][rook_finish.file] = rook_symbol;

        // Update piece in list
        for (auto &piece : m_player_pieces[player_id]) {
            if (piece.location == rook_start) {
                piece.location = rook_finish;
                assert(piece.type == 'R');
                break;
            }
        }
    }

    // Check if the player can still castle
    if (m_castling_status[player_id] != CASTLE_NONE) {
        auto can_castle = m_castling_status[player_id];
        bool king_moved = action.m_piece.type == 'K';
        bool kingside_rook_moved, queenside_rook_moved;

        // We can just check if the rooks are in their original spots
        // If they moved away and back, castling status would have
        // been disabled, so this is safe.
        if (player_id == 0) {
            kingside_rook_moved = m_collision_map[0][7] != 'R';
            queenside_rook_moved = m_collision_map[0][0] != 'R';
        } else {
            kingside_rook_moved = m_collision_map[7][0] != 'r';
            queenside_rook_moved = m_collision_map[7][7] != 'r';
        }
        if (king_moved
            or (kingside_rook_moved && (can_castle == CASTLE_KINGSIDE))
            or (queenside_rook_moved && (can_castle == CASTLE_QUEENSIDE))) {
            m_castling_status[player_id] = CASTLE_NONE;
        } else if (can_castle == CASTLE_BOTH) {
            if (kingside_rook_moved) m_castling_status[player_id] = CASTLE_QUEENSIDE;
            if (queenside_rook_moved) m_castling_status[player_id] = CASTLE_KINGSIDE;
        }
    }

    // Apply En Passant
    // Check for an en passant capture
    if((action.m_space == m_en_passant)
       and (action.m_piece.type == 'P'))
    {
        int rank = m_en_passant.rank > 4 ? 4 : 3;
        int file = m_en_passant.file;
        // Delete the captured pawn
        m_collision_map[rank][file] = 0;
        for (int i = 0; i < m_player_pieces[1-player_id].size() - 1; i++) {
            auto &piece = m_player_pieces[player_id][i];
            if ((piece.location.rank == rank) and (piece.location.file == file)) {
                assert(m_player_pieces[player_id][i].type == 'P');
                m_player_pieces[player_id].erase(m_player_pieces[player_id].begin() + i);
                break;
            }
        }
    }

    // Set up en passant target square for next move
    if(action.m_piece.type == 'P'
            and action.m_piece.location.rank == 1
            and action.m_space.rank == 3)
    {
        Space forward = {1, 0};
        m_en_passant = action.m_piece.location + forward;
    } else if (action.m_piece.type == 'P'
            and action.m_piece.location.rank == 6
            and action.m_space.rank == 4)
    {
        Space forward = {-1, 0};
        m_en_passant = action.m_piece.location + forward;
    } else
    {
        m_en_passant = NO_EN_PASSANT;
    }
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
                if ( (has_opponent_piece(attack_spaces[i], player_id))
                        or attack_spaces[i] == m_en_passant) {
                    char target = m_collision_map[attack_spaces[i].rank][attack_spaces[i].file];
                    if(can_promote)
                    {
                        for(auto& promotion_type : POSSIBLE_PROMOTIONS)
                        {
                            actions.push_back(Action(piece, attack_spaces[i], target, promotion_type));
                        }
                    } else {
                        actions.push_back(Action(piece, attack_spaces[i], target));
                    }
                }
            }
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

            //Castling is weird. I'll just hardcode all the locations
            if(m_castling_status[player_id] != CASTLE_NONE)
            {
                int rank = player_id == 0 ? 0 : 7;
                if(m_castling_status[player_id] == CASTLE_QUEENSIDE
                   or m_castling_status[player_id] == CASTLE_BOTH)
                {
                    bool clear_to_castle = true;
                    for(int file = 3; file > 0; file --)
                    {
                        clear_to_castle &= is_clear(Space{rank,file});
                    }
                    if(clear_to_castle)
                    {
                        actions.push_back(Action(piece, {rank,2}, 0, "", CASTLE_QUEENSIDE));
                    }
                }
                if(m_castling_status[player_id] == CASTLE_KINGSIDE
                   or m_castling_status[player_id] == CASTLE_BOTH)
                {
                    bool clear_to_castle = true;
                    for(int file = 5; file <7; file ++)
                    {
                        clear_to_castle &= is_clear(Space{rank,file});
                    }
                    if(clear_to_castle)
                    {
                        actions.push_back(Action(piece, {rank,6}, 0, "", CASTLE_KINGSIDE));
                    }
                }
            }
        }
        else
        {
            std::cout << "Warning: " << piece.parent->type << " moves not yet implemented." << std::endl;
        }
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