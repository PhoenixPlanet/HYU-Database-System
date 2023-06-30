#ifndef __PAGE_H__
#define __PAGE_H__

#include "file.h"

struct record_t {
    int64_t key_;
    char value_[120];

    record_t();
    record_t(int64_t key, char value[]);
};

struct child_t {
    int64_t key_;
    pagenum_t child_;

    child_t();
    child_t(int64_t key, pagenum_t child);
};

struct node_t {
    struct header {
        pagenum_t parent;
        uint32_t is_leaf;
        uint32_t number_of_keys;
        char reserved[104];
        pagenum_t spare_page;
    } header_;
    union body
    {
        std::array<record_t, 31> records;
        std::array<child_t, 248> childeren;

        body();
    } body_;
};

#endif