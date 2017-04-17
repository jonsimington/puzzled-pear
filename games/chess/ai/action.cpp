//////////////////////////////////////////////////////////////////////
/// @file action.cpp
/// @author Owen Chiaventone
/// @brief Basic model definitions for chess
//////////////////////////////////////////////////////////////////////

#include "action.hpp"
#include <sstream>

std::map<std::string, char> PIECE_CODE_LOOKUP{
    {"Pawn", 'P'},
    {"Rook", 'R'},
    {"Knight", 'N'},
    {"Bishop", 'B'},
    {"Queen", 'Q'},
    {"King", 'K'}
};

const Space INVALID_SPACE = {-1, -1};

void Action::execute() {
  // Convert location back from zero-indexed
  auto file = std::string(1, 'a' + char(m_space.file));
  auto rank = m_space.rank + 1;
  m_piece.parent->move(file, rank, m_promotion);
}

bool operator==(const Action &lhs, const Action &rhs) {
  // The "from" and "to" squares are sufficient to uniquely identify an action
  return (lhs.m_piece.location == rhs.m_piece.location) && (lhs.m_space == rhs.m_space);
}

std::ostream &operator<<(std::ostream &os, const Action &rhs) {
  os << rhs.m_piece.parent->type
     << " at " << rhs.m_piece.parent->file << rhs.m_piece.parent->rank << " to ";
  if (rhs.m_target_piece != 0)
    os << "capture " << rhs.m_target_piece << " @ ";
  os << char(rhs.m_space.file + 'a') << rhs.m_space.rank + 1;
  return os;
}

Space operator+(const Space &lhs, const Space &rhs) {
  return {lhs.rank + rhs.rank, lhs.file + rhs.file};
}

bool operator==(const Space &lhs, const Space &rhs) {
  return lhs.rank == rhs.rank && lhs.file == rhs.file;
}

PieceModel::PieceModel(cpp_client::chess::Piece piece) {
  parent = &*piece; // The profiler showed a significant performance penalty for
  // making several million copies of a shared pointer
  // Because we know that this object won't be deallocated
  // as long as we're deciding on a move and we never modify
  // it, using an unmanaged pointer here is safe
  type = PIECE_CODE_LOOKUP[piece->type];
  // Convert location to 0-indexed
  auto rank = piece->rank;
  auto file = piece->file;
  location = {rank - 1, file[0] - 'a'};
}