//////////////////////////////////////////////////////////////////////
/// @file state.cpp
/// @author Owen Chiaventone
/// @brief Chess state modelling, generation, and transitions
//////////////////////////////////////////////////////////////////////

#include "state.hpp"

#include <map>
#include <sstream>

//////////////////////////////////////////////////////////////////////
///  Lookups for moves & state transitions
//////////////////////////////////////////////////////////////////////

// Vectors representing piece move offsets
const std::vector<Space> KNIGHT_MOVES{
    {2, 1},
    {1, 2},
    {-1, 2},
    {-2, 1},
    {-1, -2},
    {-2, -1},
    {1, -2},
    {2, -1}
};

// And all multiples
const std::vector<Space> BISHOP_MOVES{
    {-1, -1},
    {-1, 1},
    {1, -1},
    {1, 1}
};

const std::vector<Space> ROOK_MOVES{
    {1, 0},
    {-1, 0},
    {0, 1},
    {0, -1}
};

// King and Queen can move the same directions, just different amounts
const std::vector<Space> ROYAL_MOVES{
    {-1, -1},
    {-1, 1},
    {1, -1},
    {1, 1},
    {1, 0},
    {-1, 0},
    {0, 1},
    {0, -1}
};

const std::vector<std::string> POSSIBLE_PROMOTIONS{
    "Queen",
    "Bishop",
    "Knight",
    "Rook"
};

// Special values for Castling
const Space WHITE_KINGSIDE_ROOK_START = {0, 7};

const Space WHITE_KINGSIDE_ROOK_CASTLED = {0, 5};

const Space WHITE_QUEENSIDE_ROOK_START = {0, 0};

const Space WHITE_QUEENSIDE_ROOK_CASTLED = {0, 3};

const Space BLACK_KINGSIDE_ROOK_START = {7, 7};

const Space BLACK_KINGSIDE_ROOK_CASTLED = {7, 5};

const Space BLACK_QUEENSIDE_ROOK_START = {7, 0};

const Space BLACK_QUEENSIDE_ROOK_CASLTED = {7, 3};

const Space NO_EN_PASSANT = {-1, -1};

// Accessed as PAWN_START_RANK[player_id]
const int PAWN_START_RANK[] = {1, 6};

//////////////////////////////////////////////////////////////////////
///  Class Implementation
//////////////////////////////////////////////////////////////////////

State::State(const cpp_client::chess::Game &game)
    : m_collision_map() {
  m_active_player = game->current_player->id[0] - '0';
  assert(m_active_player == 0 or m_active_player == 1);

  // Read in pieces
  for (const auto &piece: game->pieces) {
    PieceModel piecemodel(piece);
    int rank = piecemodel.location.rank;
    int file = piecemodel.location.file;

    int piece_owner = piece->owner->id[0] - '0';
    m_player_pieces[piece_owner].push_back(piecemodel);

    char piece_code = PIECE_CODE_LOOKUP[piece->type];
    if (piece_owner == 1) piece_code = char(tolower(piece_code));
    m_collision_map[rank][file] = piece_code;
  }

  // Read in castling status and En Passant from FEN
  std::istringstream fen(game->fen);
  std::string piece_placement, active_color, castling_status, en_passant, clock, move;
  fen >> piece_placement >> active_color >> castling_status >> en_passant >> clock >> move;

  if (castling_status.find('K') != std::string::npos) {
    m_castling_status[0] = CASTLE_KINGSIDE;
  }
  if (castling_status.find('Q') != std::string::npos) {
    if (m_castling_status[0] == CASTLE_KINGSIDE) m_castling_status[0] = CASTLE_BOTH;
    else m_castling_status[0] = CASTLE_QUEENSIDE;
  }
  if (castling_status.find('k') != std::string::npos) {
    m_castling_status[1] = CASTLE_KINGSIDE;
  }
  if (castling_status.find('q') != std::string::npos) {
    if (m_castling_status[1] == CASTLE_KINGSIDE) m_castling_status[0] = CASTLE_BOTH;
    else m_castling_status[1] = CASTLE_QUEENSIDE;
  }

  if (en_passant == "-") {
    m_en_passant = NO_EN_PASSANT;
  } else {
    int file = en_passant[0] - 'a';
    int rank = en_passant[1] - '1';
    m_en_passant = {rank, file};
  }

  m_last_move = {-1, -1};
}

std::vector<Action> State::available_actions(int player_id) const {

  auto possible_actions = all_actions(player_id);
  std::vector<Action> valid_actions;

  // filter to actions that don't result in going into check
  for (auto &action : possible_actions) {
    // Castling moves can't take you out of or put you into check
    if (action.m_piece.type == 'K'
        && abs(action.m_space.rank - action.m_piece.location.rank) > 1) {
      if (!in_check(player_id)) {
        auto new_state = this->apply(action);
        if (!new_state.in_check(player_id) == false) {
          valid_actions.push_back(action);
        }
        valid_actions.push_back(action);
      }

    } else {
      // Regular, non-castling moves just can't put you into check
      auto new_state = this->apply(action);
      if (new_state.in_check(player_id) == false) {
        valid_actions.push_back(action);
      }
    }
  }

  return valid_actions;
}

State State::apply(const Action &action) const {
  State copy = *this;   // I'm hoping and praying that c++ is smart enough
  // To generate a default copy constructor that deep copies
  // But in my heart I know that this line will break something
  // and cost me hours to fix
  copy.mutate(action);
  return copy;
}

bool State::in_check(int player_id) const {
  Space king_location = {-1, -1};
  for (const auto &piece: m_player_pieces[player_id]) {
    if (piece.type == 'K') {
      king_location = piece.location;
      break;
    }
  }

  //assert(king_location.rank != -1);
  return space_threatened(king_location, 1 - player_id);
}

bool operator==(const State &lhs, const State &rhs) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (lhs.m_collision_map[i][j] != rhs.m_collision_map[i][j]) return false;
    }
  }
  return true;
}

std::vector<Action> State::all_actions(int player_id) const {
  assert(player_id == 0 or player_id == 1);

  std::vector<Action> actions;
  actions.reserve(40);

  // maybe forward[player_id] would be better?
  Space forward = {1, 0};
  Space backward = {-1, 0};
  if (player_id == 1) forward = {-1, 0};
  if (player_id == 1) backward = {1, 0};

  for (auto &piece : m_player_pieces[player_id]) {
    if (piece.type == 'P') {
      // Regular Moves
      bool in_original_space = (piece.location.rank == PAWN_START_RANK[player_id]);
      Space space_ahead = piece.location + forward;
      bool can_promote = (space_ahead.rank == 7) or (space_ahead.rank == 0);
      if (is_clear(space_ahead)) {
        // Promotion
        if (can_promote) {
          for (auto &promotion_type : POSSIBLE_PROMOTIONS) {
            actions.push_back(Action(piece, this, space_ahead, 0, promotion_type));
          }
        } else {
          actions.push_back(Action(piece, this, space_ahead));
          if (in_original_space and is_clear(space_ahead + forward)) {
            actions.push_back(Action(piece, this, space_ahead + forward));
          }
        }
      }

      // Attacks
      Space left = {0, -1}, right = {0, 1};
      Space attack_spaces[] = {piece.location + forward + left,
                               piece.location + forward + right};
      for (int i = 0; i < 2; i++) {
        if ((has_opponent_piece(attack_spaces[i], player_id))
            or (attack_spaces[i] == m_en_passant
                && has_opponent_piece(m_en_passant + backward, player_id))) {
          char target = m_collision_map[attack_spaces[i].rank][attack_spaces[i].file];
          if (attack_spaces[i] == m_en_passant) target = 'p';
          assert(target != 0);
          if (can_promote) {
            for (auto &promotion_type : POSSIBLE_PROMOTIONS) {
              actions.push_back(Action(piece, this, attack_spaces[i], target, promotion_type));
            }
          } else {
            actions.push_back(Action(piece, this, attack_spaces[i], target));
          }
        }
      }
    } else if (piece.type == 'N') {
      for (auto &offset : KNIGHT_MOVES) {
        auto space = piece.location + offset;
        if (is_clear(space) or has_opponent_piece(space, player_id)) {
          char target = m_collision_map[space.rank][space.file];
          actions.push_back(Action(piece, this, space, target));
        }
      }
    } else if (piece.type == 'R') {
      straight_line_moves(piece, ROOK_MOVES, player_id, actions);
    } else if (piece.type == 'B') {
      straight_line_moves(piece, BISHOP_MOVES, player_id, actions);
    } else if (piece.type == 'Q') {
      straight_line_moves(piece, ROYAL_MOVES, player_id, actions);
    } else if (piece.type == 'K') {
      for (auto &direction: ROYAL_MOVES) {
        Space space = piece.location + direction;
        if (is_clear(space) or has_opponent_piece(space, player_id)) {
          char target = m_collision_map[space.rank][space.file];
          actions.push_back(Action(piece, this, space, target));
        }
      }

      //Castling is weird. I'll just hardcode all the locations
      if (m_castling_status[player_id] != CASTLE_NONE) {
        int rank = player_id == 0 ? 0 : 7;
        char rook_code = player_id == 0 ? 'R' : 'r';
        if (m_castling_status[player_id] == CASTLE_QUEENSIDE
            or m_castling_status[player_id] == CASTLE_BOTH) {
          bool clear_to_castle = true;
          for (int file = 3; file > 0; file--) {
            clear_to_castle &= is_clear(Space{rank, file});
          }
          if (m_collision_map[rank][0] != rook_code) clear_to_castle = false;
          if (clear_to_castle) {
            actions.push_back(Action(piece, this, {rank, 2}, 0, "", CASTLE_QUEENSIDE));
          }
        }
        if (m_castling_status[player_id] == CASTLE_KINGSIDE
            or m_castling_status[player_id] == CASTLE_BOTH) {
          bool clear_to_castle = true;
          for (int file = 5; file < 7; file++) {
            clear_to_castle &= is_clear(Space{rank, file});
          }
          if (m_collision_map[rank][7] != rook_code) clear_to_castle = false;
          if (clear_to_castle) {
            actions.push_back(Action(piece, this, {rank, 6}, 0, "", CASTLE_KINGSIDE));
          }
        }
      }
    } else {
      std::cout << "Warning: " << piece.parent->type << " moves not yet implemented." << std::endl;
    }
  }
  return actions;
}

void State::mutate(const Action &action) {
  // Debug: Assert loop invariant is satisfied
  /*
  for(const auto& piece : m_player_pieces[0])
      assert(toupper(m_collision_map[piece.location.rank][piece.location.file]) == piece.type);
  for(const auto& piece : m_player_pieces[1])
      assert(toupper(m_collision_map[piece.location.rank][piece.location.file]) == piece.type);
  */
  // Update the board
  int player_id = action.m_piece.parent->owner->id[0] - '0'; // This can be different from current player
  // if we're checking for threatened squares
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
  bool piece_moved = false;
  for (auto &piece : m_player_pieces[player_id]) {
    if (piece.location == action.m_piece.location) {
      piece.location = action.m_space;
      //assert(piece.type == action.m_piece.type);
      piece_moved = true;
      break;
    }
  }
  assert(piece_moved);

  int opponent_id = (player_id == 0 ? 1 : 0);
  if (action.m_target_piece != 0) {
    if (action.m_space == m_en_passant) {
      int rank = m_en_passant.rank > 4 ? 4 : 3;
      Space true_location = {rank, action.m_space.file};
      m_collision_map[true_location.rank][true_location.file] = 0;
      remove_piece(opponent_id, true_location);
    } else {
      remove_piece(opponent_id, action.m_space);
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
    bool rook_moved = false;
    for (auto &piece : m_player_pieces[player_id]) {
      if (piece.location == rook_start) {
        piece.location = rook_finish;
        assert(piece.type == 'R');
        rook_moved = true;
        break;
      }
    }
    assert(rook_moved);
  }

  // Debug: Assert loop invariant is satisfied
  /*
  for(const auto& piece : m_player_pieces[0])
    assert(toupper(m_collision_map[piece.location.rank][piece.location.file]) == piece.type);
  for(const auto& piece : m_player_pieces[1])
    assert(toupper(m_collision_map[piece.location.rank][piece.location.file]) == piece.type);
  */

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

  // Set up en passant target square for next move
  if (action.m_piece.type == 'P'
      and action.m_piece.location.rank == 1
      and action.m_space.rank == 3) {
    Space forward = {1, 0};
    m_en_passant = action.m_piece.location + forward;
    assert(m_collision_map[m_en_passant.rank][m_en_passant.file] == 0);
  } else if (action.m_piece.type == 'P'
      and action.m_piece.location.rank == 6
      and action.m_space.rank == 4) {
    Space forward = {-1, 0};
    m_en_passant = action.m_piece.location + forward;
    assert(m_collision_map[m_en_passant.rank][m_en_passant.file] == 0);
  } else {
    m_en_passant = NO_EN_PASSANT;
  }

  // Debug: Assert loop invariant is satisfied
  /*
  for (const auto &piece : m_player_pieces[0])
    assert(toupper(m_collision_map[piece.location.rank][piece.location.file]) == piece.type);
  for (const auto &piece : m_player_pieces[1])
    assert(toupper(m_collision_map[piece.location.rank][piece.location.file]) == piece.type);
  */
  // Swap active player
  m_active_player = (m_active_player == 0 ? 1 : 0);

  // Update the hash value because the state has changed
  m_last_move = action.m_space;
}

bool State::is_clear(const Space &space) const {
  if ((space.rank > 7) or (space.rank < 0)
      or (space.file > 7) or (space.file < 0)) {
    return false;
  } else {
    return m_collision_map[space.rank][space.file] == 0;
  }
}

bool State::has_opponent_piece(Space space, int player_id) const {
  if ((space.rank > 7) or (space.rank < 0)
      or (space.file > 7) or (space.file < 0)) {
    return false;
  } else {
    char piece_code = m_collision_map[space.rank][space.file];
    // Black's pieces are lowercase, White's pieces are uppercase
    if (player_id == 0) {
      return (('a' <= piece_code) and (piece_code <= 'z'));
    } else {
      return (('A' <= piece_code) and (piece_code <= 'Z'));
    }
  }
}

void State::straight_line_moves(const PieceModel &piece, std::vector<Space> directions, int player_id,
                                std::vector<Action> &actions) const {
  for (auto &direction : directions) {
    Space space = piece.location + direction;
    while (true) {
      if (is_clear(space) or has_opponent_piece(space, player_id)) {
        char target_piece = m_collision_map[space.rank][space.file];
        actions.push_back(Action(piece, this, space, target_piece));
      }
      if (!is_clear(space)) break;
      space = space + direction;
    }
  }
}

int State::get_active_player() const {
  return m_active_player;
}

void State::remove_piece(int player_id, const Space &location) {
  // If the move takes a piece, delete it from the list
  // The collision map doesn't store links,
  // so we have to iterate through the opponent's pieces until we find the
  // right one. In practice this isn't a problem because there are never more
  // than 16 opponent pieces, which is a small number.
  bool piece_taken = false;
  for (int i = 0; i < m_player_pieces[player_id].size(); i++) {
    auto &piece = m_player_pieces[player_id][i];
    if (piece.location == location) {
      // Delete the piece from the list
      m_player_pieces[player_id].erase(m_player_pieces[player_id].begin() + i);
      piece_taken = true;
      break;
    }
  }
  assert(piece_taken);
}

bool State::space_threatened(Space space, int attacking_player) const {
  char knight_code[] = {'N', 'n'};
  char pawn_code[] = {'P', 'p'};
  char rook_code[] = {'R', 'r'};
  char bishop_code[] = {'B', 'b'};
  char queen_code[] = {'Q', 'q'};
  char king_code[] = {'K', 'k'};

  Space PAWN_ATTACKS[2][2] = {
      {{-1, 1}, {-1, -1}},
      {{1, 1}, {1, -1}}
  };

  for (auto move: KNIGHT_MOVES) {
    if (piece_at(space + move) == knight_code[attacking_player]) return true;
  }

  for (auto move: PAWN_ATTACKS[attacking_player]) {
    if (piece_at(space + move) == pawn_code[attacking_player]) return true;
  }

  for (auto move: BISHOP_MOVES) {
    auto space_considered = space + move;
    char s = piece_at(space_considered);
    if ((s == bishop_code[attacking_player])
        || s == queen_code[attacking_player]
        || s == king_code[attacking_player])
      return true;
    while (in_board(space_considered)) {
      s = piece_at(space_considered);
      if ((s == bishop_code[attacking_player])
          || s == queen_code[attacking_player])
        return true;
      if (s != 0) break;
      space_considered = space_considered + move;
    }
  }

  for (auto move: ROOK_MOVES) {
    auto space_considered = space + move;
    char s = piece_at(space_considered);
    if ((s == rook_code[attacking_player])
        || s == queen_code[attacking_player]
        || s == king_code[attacking_player])
      return true;
    while (in_board(space_considered)) {
      s = piece_at(space_considered);
      if ((s == rook_code[attacking_player])
          || s == queen_code[attacking_player])
        return true;
      if (s != 0) break;
      space_considered = space_considered + move;
    }
  }

  return false;
}

bool State::in_board(Space space) const {
  return !((space.rank > 7) or (space.rank < 0)
      or (space.file > 7) or (space.file < 0));
}

char State::piece_at(Space space) const {
  if (in_board(space)) return m_collision_map[space.rank][space.file];
  else return 0;
}
