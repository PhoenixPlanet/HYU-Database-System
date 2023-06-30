#ifndef __FILE_H__
#define __FILE_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>

#include "page.h"

class TableFile {
public:
    TableFile();
    TableFile(int fd, std::string pathname);

    pagenum_t file_alloc_page(page_t& header_page);
    // TODO: 해당 함수는 버퍼 매니저에서 구현될 수도 있음: 이러한 경우 삭제 필요 - 삭제됨
    // void file_free_page(pagenum_t pagenum, page_t& header_page);
    void file_read_page(pagenum_t pagenum, page_t* dest);
    void file_write_page(pagenum_t pagenum, const page_t* src);
    int file_close_db();

    std::string pathname();
    int fd();

    void pathname(std::string pathname_n);
    void fd(int fd_n);

private:
    int fd_;
    std::string pathname_;

    void file_write_page_array(pagenum_t pagenum, const page_t* src, int length);
    void file_add_free_list(pagenum_t start_page, int length);
};

#endif