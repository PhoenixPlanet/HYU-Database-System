#include "file.h"

TableFile::TableFile() : fd_(-1) {    }

TableFile::TableFile(int fd, std::string pathname) : fd_(fd), pathname_(pathname) {    }

void TableFile::file_write_page_array(pagenum_t pagenum, const page_t* src, int length) {
    // 파일 쓰기
    lseek(fd_, PAGE_BYTE * pagenum, SEEK_SET);
    ssize_t write_return = write(fd_, src, PAGE_BYTE * length);
    fsync(fd_);

    // 예외 처리 (예외 출력)
    if (write_return != PAGE_BYTE * length) {
        if (write_return < 0) {
            fprintf(stderr, "Write Filed - pagenum: %lu, length: %d, errno: %s", pagenum, length, strerror(errno));
        } else {
            printf("Write Failed - write %ld bytes\n", write_return);
        }
        exit(1);
    }
}

void TableFile::file_add_free_list(pagenum_t start_page, int length) {
    // 페이지 리스트 만들기
    std::unique_ptr<page_t[]> free_list(new page_t[length]);
    memset(free_list.get(), 0, sizeof(page_t) * length);
    
    for (int i = 0; i < length; i++) {
        if (i != length - 1) {
            free_list[i].page_.free_page_.next_free_page_num = start_page + i + 1;
        } else {
            free_list[i].page_.free_page_.next_free_page_num = 0;
        }
    }

    // 파일에 쓰기
    file_write_page_array(start_page, free_list.get(), length);
}

// 파일 
pagenum_t TableFile::file_alloc_page(page_t& header_page) {
    // free page list 확인 후 없으면 새로 생성
    if (header_page.page_.header_page_.free_page_num == 0) {
        file_add_free_list(header_page.page_.header_page_.number_of_pages, FREE_PAGE_GROWTH);
        header_page.page_.header_page_.free_page_num = header_page.page_.header_page_.number_of_pages;
        header_page.page_.header_page_.number_of_pages += FREE_PAGE_GROWTH;
    }
    
    pagenum_t target_pagenum = header_page.page_.header_page_.free_page_num;

    return target_pagenum;
}

// TODO: 해당 부분 전체 버퍼매니저에서 구현가능 - 삭제됨
// void TableFile::file_free_page(pagenum_t pagenum, page_t& header_page) {}

void TableFile::file_read_page(pagenum_t pagenum, page_t* dest) {
    // 파일 읽기
    lseek(fd_, PAGE_BYTE * pagenum, SEEK_SET);
    ssize_t read_return = read(fd_, dest, PAGE_BYTE);
    
    // 예외 처리 (예외 출력)
    if (read_return != PAGE_BYTE) {
        if (read_return < 0) {
            fprintf(stderr, "Read Filed - pagenum: %lu, errno: %s", pagenum, strerror(errno));
        } else {
            printf("Read Failed - read %ld bytes\n", read_return);
        }
        exit(1);
    }
}

void TableFile::file_write_page(pagenum_t pagenum, const page_t* src) {
    file_write_page_array(pagenum, src, 1);
}

int TableFile::file_close_db() {
    int flag = close(fd_);
    
    // 에러 발생 시 에러 출력
    if (flag < 0) {
        fprintf(stderr, "Close Failed - errno: %s", strerror(errno));
    }

    return flag;
}

std::string TableFile::pathname() {
    return pathname_;
}

int TableFile::fd() {
    return fd_;
}

void TableFile::pathname(std::string pathname_n) {
    pathname_ = pathname_n;
}

void TableFile::fd(int fd_n) {
    fd_ = fd_n;
}
