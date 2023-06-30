#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include <climits>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <map>
#include <set>
#include <algorithm>
#include <string>

#include "file.h"

class FileManager {
public:
    static FileManager& instance() {
        static FileManager instance;
        return instance;
    }

    uint32_t open_db_file(const char* pathname);
    uint32_t open_db_file(std::string pathname, uint32_t table_id);
    int close_db_file(uint32_t table_id);
    int close_all_db_file();

    void get_node(uint32_t table_id, pagenum_t pagenum, page_t& page);
    void write_node(uint32_t table_id, pagenum_t pagenum, page_t& page);

    void get_header_page(uint32_t table_id, page_t& header_page);
    void write_header_to_file(uint32_t table_id, page_t& header_page);

    // TODO: 해당 함수들은 buffer manager와 연계가 필요함
    pagenum_t get_new_node(uint32_t table_id, page_t& header_page);
    // TODO: 해당 함수는 버퍼 매니저에서 구현될 수도 있음: 이러한 경우 삭제 필요 - 삭제됨
    // void delete_node(uint32_t table_id, pagenum_t pagenum, page_t& header_page);

    // TODO: 해당 함수들은 bpt 혹은 buffer에 구현되어야 함
    /*pagenum_t get_root();
    void set_root(pagenum_t new_pagenum);*/
    
private:
    FileManager() {
        current_open_table_num_ = 0;
        opened_file_num_ = 0;
    }

    // TODO: 해당 작업은 버퍼 매니저의 역할일 수 있음
    ~FileManager() {
        
    }

    std::string get_real_path(const char* pathname);
    int open_file_and_get_fd(const char* pathname, TableFile& tablefile);
    bool is_file_exist(const char* pathname);

    // 현재 열려있는 테이블 개수
    uint32_t current_open_table_num_;
    // 지금까지 열어본 테이블 개수
    uint32_t opened_file_num_;
    // 테이블 리스트 (table_id, TableFile object)
    std::unordered_map<uint32_t, TableFile> table_list_;
    // 파일 경로 리스트 (pathname, <is_open, table_id>)
    std::map<std::string, std::pair<bool, uint32_t>> pathname_list_;
};

#endif