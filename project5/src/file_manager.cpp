#include "file_manager.h"

// db 파일 열기
// 오류 발생 시 0 return, 아닐 때 table id return
uint32_t FileManager::open_db_file(const char* pathname) {
    // 파일이 존재하는지 확인
    if (is_file_exist(pathname)) {
        // 존재한다면 이미 열어봤던 파일인지 확인
        std::string real_path = get_real_path(pathname);
        auto file_info_it = pathname_list_.find(real_path); 
        if (file_info_it != pathname_list_.end()) {
            if ((*file_info_it).second.first) { // is open: 이미 열려있는 파일일 때
                #ifdef DEBUG_BPT
                perror("already open given file\n");
                #endif
                return (*file_info_it).second.second; // table id
            } else { // 닫혀있는 파일일 때
                return open_db_file((*file_info_it).first, // pathname
                                    (*file_info_it).second.second); // talbe id
            }
        }
    }

    if (opened_file_num_ >= 10) {
        #ifdef DEBUG_BPT
        perror("aleady opened 10 files\n");
        #endif
        return 0;
    }

    // 열려있지 않거나 존재하지 않는 파일이라면 새로운 db file 열고 추가
    TableFile tablefile;
    int fd = open_file_and_get_fd(pathname, tablefile);
    if (fd < 0) { // 파일 열기 오류
        return 0;
    }
    std::string real_path = get_real_path(pathname);
    tablefile.pathname(real_path);

    // 새로 연 파일 추가
    current_open_table_num_++;
    opened_file_num_++;

    table_list_[opened_file_num_] = tablefile;
    pathname_list_[real_path] = std::make_pair(true, opened_file_num_);

    return opened_file_num_;
}

// 이미 한 번 열었던 파일의 경우
// 오류 발생 시 0 return, 아닐 때 table id return
uint32_t FileManager::open_db_file(std::string pathname, uint32_t table_id) {
    if (table_id <= 0) {
        #ifdef DEBUG_BPT
        perror("open db: wrong table id\n");
        #endif
        return 0;
    }

    TableFile tablefile;
    int fd = open_file_and_get_fd(pathname.c_str(), tablefile);
    if (fd < 0) { // 파일 열기 오류
        return 0;
    }

    tablefile.pathname(pathname);

    current_open_table_num_++;
    table_list_[table_id] = tablefile;
    pathname_list_[pathname].first = true;

    return table_id;
}

int FileManager::open_file_and_get_fd(const char* pathname, TableFile& tablefile) {
    int fd = open(pathname, O_RDWR | O_CREAT | O_SYNC, 0666);
    
    // 에러 발생 시 에러 출력
    if (fd < 0) {
        fprintf(stderr, "Open Failed - errno: %s\n", strerror(errno));
        return -1;
    }
    
    tablefile.fd(fd);
    // 파일 크기 확인 후 0인 경우 헤더페이지 생성
    off_t f_size = lseek(fd, 0, SEEK_END);
    if (f_size == 0) {
        page_t* header_page = new page_t();
        memset(header_page, 0, sizeof(page_t));
        header_page->page_.header_page_.free_page_num = 0;
        header_page->page_.header_page_.number_of_pages = 1;
        header_page->page_.header_page_.root_page_num = 0;
        tablefile.file_write_page(0, header_page);
        delete header_page;
    } else if (f_size % PAGE_BYTE != 0) {
        fprintf(stderr, "Open Failed - wrong format(pages are not %d byte)\n", PAGE_BYTE);
        return -1;
    }

    return fd;
}

bool FileManager::is_file_exist(const char* pathname) {
    if (access(pathname, F_OK) == 0) {
        return true;
    } else {
        return false;
    }
}

std::string FileManager::get_real_path(const char* pathname) {
    char* real_path = NULL;

    real_path = realpath(pathname, NULL);
    if (real_path == NULL) {
        fprintf(stderr, "Get real path Failed - errno: %s\n", strerror(errno));
        // TODO: throw error
        return std::string("error");
    }

    std::string result(real_path);
    free(real_path);
    return result;
}

int FileManager::close_db_file(uint32_t table_id) {
    auto it = table_list_.find(table_id);

    if (it == table_list_.end()) {
        fprintf(stderr, "Tried to close wrong table id - %d\n", table_id);
        return -1;
    }

    int flag = (*it).second.file_close_db();
    pathname_list_[(*it).second.pathname()].first = false;
    table_list_.erase(it);

    current_open_table_num_--;
    return flag;
}

int FileManager::close_all_db_file() {
    for (auto& it : pathname_list_) {
        if (it.second.first){
            close_db_file(it.second.second);
        }
    }
    
    pathname_list_.clear();
    table_list_.clear();
    current_open_table_num_ = 0;
    opened_file_num_ = 0;

    return 0;
}

void FileManager::get_node(uint32_t table_id, pagenum_t pagenum, page_t& page) {
    //debug
    /*if (pagenum == 0) {
        perror("tried to read page from header\n");
    }*/
    
    auto it = table_list_.find(table_id);

    if (it == table_list_.end()) {
        fprintf(stderr, "Tried to read from wrong table id - %d\n", table_id);
        return;
    }

    (*it).second.file_read_page(pagenum, &page);
}

void FileManager::write_node(uint32_t table_id, pagenum_t pagenum, page_t& page) {
    //debug
    /*if (pagenum == 0) {
        perror("tried to write page on header\n");
    }*/

    auto it = table_list_.find(table_id);

    if (it == table_list_.end()) {
        fprintf(stderr, "Tried to read from wrong table id - %d\n", table_id);
        return;
    }

    (*it).second.file_write_page(pagenum, &page);
}

void FileManager::get_header_page(uint32_t table_id, page_t& header_page) {
    auto it = table_list_.find(table_id);

    if (it == table_list_.end()) {
        fprintf(stderr, "Tried to read from wrong table id - %d\n", table_id);
        return;
    }

    (*it).second.file_read_page(0, &header_page);
}

void FileManager::write_header_to_file(uint32_t table_id, page_t& header_page) {
    auto it = table_list_.find(table_id);

    if (it == table_list_.end()) {
        fprintf(stderr, "Tried to read from wrong table id - %d\n", table_id);
        return;
    }

    (*it).second.file_write_page(0, &header_page);
}

// TODO: 버퍼 매니저와 연계가 필요함 (헤더에 핀 걸어주기)
pagenum_t FileManager::get_new_node(uint32_t table_id, page_t& header_page) {
    auto it = table_list_.find(table_id);

    if (it == table_list_.end()) {
        fprintf(stderr, "Tried to read from wrong table id - %d\n", table_id);
        return 0;
    }

    pagenum_t new_pagenum = (*it).second.file_alloc_page(header_page);
    return new_pagenum;
}

// TODO: 버퍼 매니저와 연계가 필요함 (헤더에 핀 걸어주기)
// TODO: 해당 함수 전체 버퍼 매니저에서 구현 가능 - 삭제됨
/*void FileManager::delete_node(uint32_t table_id, pagenum_t pagenum, page_t& header_page) {
    auto it = table_list_.find(table_id);

    if (it == table_list_.end()) {
        fprintf(stderr, "Tried to read from wrong table id - %d\n", table_id);
        return;
    }
    
    (*it).second.file_free_page(pagenum, header_page);
}*/