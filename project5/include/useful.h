#ifndef __USEFUL_H__
#define __USEFUL_H__

#include <unordered_map>

namespace std {
template <class T1, class T2>
struct hash<std::pair<T1, T2>> {
    size_t operator() (const std::pair<T1, T2>& p) const {
        auto a = std::hash<T1>{}(p.first);
        auto b = std::hash<T2>{}(p.second);
        
        return a ^ b;
    }
};
}

#endif