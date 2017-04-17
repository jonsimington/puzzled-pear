// Headers for zobrist hash algorithms
// implementation in hash.hpp and hash.cpp
#ifndef CPP_CLIENT_ZOBRIST_HPP
#define CPP_CLIENT_ZOBRIST_HPP

#include <map>

extern long ZOBRIST_HASH_TABLE[8][8][12];

extern const std::map<char, int> HASH_INDICES;

void init_zobrist_hash_table();

#endif //CPP_CLIENT_ZOBRIST_HPP
