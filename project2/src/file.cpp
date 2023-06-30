#include "file.h"

int current_db_fd;

void file_write_page_array(pagenum_t pagenum, const page_t* src, int length) {
    // 파일 쓰기
    lseek(current_db_fd, PAGE_BYTE * pagenum, SEEK_SET);
    ssize_t write_return = write(current_db_fd, src, PAGE_BYTE * length);
    fsync(current_db_fd);

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

void file_add_free_list(pagenum_t start_page, int length) {
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

pagenum_t file_alloc_page() {
    // 헤더 페이지 읽기
    std::unique_ptr<page_t> header_page = std::make_unique<page_t>();
    file_read_page(0, header_page.get());

    // free page list 확인 후 없으면 새로 생성
    if (header_page->page_.header_page_.free_page_num == 0) {
        file_add_free_list(header_page->page_.header_page_.number_of_pages, FREE_PAGE_GROWTH);
        header_page->page_.header_page_.free_page_num = header_page->page_.header_page_.number_of_pages;
        header_page->page_.header_page_.number_of_pages += FREE_PAGE_GROWTH;
    }

    // 첫 번째 free 페이지 읽기
    std::unique_ptr<page_t> target_page = std::make_unique<page_t>();
    pagenum_t target_pagenum = header_page->page_.header_page_.free_page_num;
    file_read_page(target_pagenum, target_page.get());
    
    // free 페이지 리스트에서 빼내기
    header_page->page_.header_page_.free_page_num = target_page->page_.free_page_.next_free_page_num;
    memset(target_page.get(), 0, sizeof(page_t));

    // alloc된 새 페이지와 header 페이지 반영해주기
    file_write_page(0, header_page.get());
    file_write_page(target_pagenum, target_page.get());

    return target_pagenum;
}

void file_free_page(pagenum_t pagenum) {
    // 헤더 페이지 읽기
    std::unique_ptr<page_t> header_page = std::make_unique<page_t>();
    file_read_page(0, header_page.get());

    // free 페이지 해당 페이지에 쓰기
    std::unique_ptr<page_t> free_page = std::make_unique<page_t>();
    memset(free_page.get(), 0, sizeof(page_t));
    // 원래의 free page list 맨 앞에 끼워넣기
    free_page->page_.free_page_.next_free_page_num = header_page->page_.header_page_.free_page_num;
    file_write_page(pagenum, free_page.get());

    // 헤더 페이지에 반영
    header_page->page_.header_page_.free_page_num = pagenum;
    file_write_page(0, header_page.get());
}

void file_read_page(pagenum_t pagenum, page_t* dest) {
    // 파일 읽기
    lseek(current_db_fd, PAGE_BYTE * pagenum, SEEK_SET);
    ssize_t read_return = read(current_db_fd, dest, PAGE_BYTE);
    
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

void file_write_page(pagenum_t pagenum, const page_t* src) {
    file_write_page_array(pagenum, src, 1);
}

int file_open_db(const char* pathname) {
    int fd = open(pathname, O_RDWR | O_CREAT | O_SYNC, 0666);
    
    // 에러 발생 시 에러 출력
    if (fd < 0) {
        fprintf(stderr, "Open Failed - errno: %s", strerror(errno));
        return -1;
    }

    set_current_db_fd(fd);

    // 파일 크기 확인 후 0인 경우 헤더페이지 생성
    off_t f_size = lseek(fd, 0, SEEK_END);
    if (f_size == 0) {
        page_t* header_page = new page_t();
        memset(header_page, 0, sizeof(page_t));
        header_page->page_.header_page_.free_page_num = 0;
        header_page->page_.header_page_.number_of_pages = 1;
        header_page->page_.header_page_.root_page_num = 0;
        file_write_page(0, header_page);
        delete header_page;
    } else if (f_size < 4096) {
        printf("wrong file");
        exit(1);
    }

    return fd;
}

int file_close_db(int fd) {
    int flag = close(fd);
    
    // 에러 발생 시 에러 출력
    if (flag < 0) {
        fprintf(stderr, "Close Failed - errno: %s", strerror(errno));
    }

    return flag;
}

void set_current_db_fd(int fd) {
    current_db_fd = fd;
}
