#ifndef __PAGE_H__
#define __PAGE_H__

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <array>

typedef uint64_t pagenum_t;

constexpr int PAGE_BYTE = 4096;
constexpr int FREE_PAGE_GROWTH = 20;

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

#pragma pack(push, 1)
struct page_t {
    union page
    {
        uint8_t bytes[PAGE_BYTE];
        struct header_page
        {
            pagenum_t free_page_num;
            pagenum_t root_page_num;
            uint64_t number_of_pages;
        } header_page_;

        struct free_page
        {
            pagenum_t next_free_page_num;
        } free_page_;

        struct node_page
        {
            struct header {
                pagenum_t parent;
                uint32_t is_leaf;
                uint32_t number_of_keys;
                uint64_t reserved1;
                uint64_t page_lsn_;
                char reserved2[88];
                pagenum_t spare_page;
            } header_;
            union body
            {
                std::array<record_t, 31> records;
                std::array<child_t, 248> childeren;

                body();
            } body_;
        } node_page_;
    
        page();
    } page_;
};
#pragma pack(pop)

#endif