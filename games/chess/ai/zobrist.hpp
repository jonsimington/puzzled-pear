//
// Created by owen on 4/16/17.
//

#ifndef CPP_CLIENT_ZOBRIST_HPP
#define CPP_CLIENT_ZOBRIST_HPP

#include <map>

extern long ZOBRIST_HASH_TABLE[8][8][12];

extern const std::map<char, int> HASH_INDICES;

long init_zobrist_hash_table();

#endif //CPP_CLIENT_ZOBRIST_HPP