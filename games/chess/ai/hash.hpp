#ifndef CPP_CLIENT_HASH_HPP
#define CPP_CLIENT_HASH_HPP

#include <map>
#include "action.hpp"
#include "state.hpp"

// This stuff is just boilerplate to get std::unordered map
// to use the member hash functions
namespace std {
template<>
struct hash<State> {
  size_t operator()(const State &k) const { return k.hash(); }
};

template<>
struct hash<Action> {
  size_t operator()(const Action &k) const { return k.hash(); }
};
}

#endif //CPP_CLIENT_HASH_HPP
