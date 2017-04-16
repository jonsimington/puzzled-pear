#include "hash.hpp"

const std::map<char, int> HASH_INDICES{
    {'P', 0},
    {'R', 1},
    {'N', 2},
    {'B', 3},
    {'Q', 4},
    {'K', 5},
    {'p', 6},
    {'r', 7},
    {'n', 8},
    {'b', 9},
    {'q', 10},
    {'k', 11}
};

// Global hash table. Must be initialized with init_zobrist_hash_table
long ZOBRIST_HASH_TABLE[8][8][12];

long init_zobrist_hash_table()
{
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            for (int k = 0; k < 12; k++)
            {
                ZOBRIST_HASH_TABLE[i][j][k] = random();
            }
        }
    }
}

long State::hash() const
{
    long hash = 0;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            char piece = m_collision_map[i][j];
            if (piece != 0)
            {
                hash ^= ZOBRIST_HASH_TABLE[i][j][HASH_INDICES.at(piece)];
            }
        }
    }
    return hash;
}

long Action::hash() const
{
    return m_parent->hash() ^ ZOBRIST_HASH_TABLE[m_space.rank][m_space.file][HASH_INDICES.at(m_piece.type)];
}