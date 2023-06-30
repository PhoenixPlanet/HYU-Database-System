#ifndef __FILE_H__
#define __FILE_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>

extern "C" {

typedef uint64_t pagenum_t;

constexpr int PAGE_BYTE = 4096;

constexpr int FREE_PAGE_GROWTH = 20;

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
    } page_;
};

pagenum_t file_alloc_page();

void file_free_page(pagenum_t pagenum);

void file_read_page(pagenum_t pagenum, page_t* dest);

void file_write_page(pagenum_t pagenum, const page_t* src);

int file_open_db(const char* pathname);

int file_close_db(int fd);

void set_current_db_fd(int fd);

}

#endif