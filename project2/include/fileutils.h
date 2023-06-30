#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__

#include <climits>
#include <vector>
#include <tuple>
#include <algorithm>
#include <string>

#include "file.h"
#include "page.h"

struct bpt_node {
    pagenum_t pagenum_;
    node_t page_;

    bpt_node();
    
    bpt_node(pagenum_t pagenum, bool auto_get=false, bool auto_write=false);

    void write_to_file();

    ~bpt_node();

private:
    bool auto_write_;
};

class DBFileUtils {
public:
    static DBFileUtils& instance() {
        static DBFileUtils instance;
        return instance;
    }

    int open_db_file(const char* pathname);
    int set_db_file(uint32_t table_id);
    int close_db_file(uint32_t table_id);

    void get_node(pagenum_t pagenum, bpt_node& node);
    void write_node(bpt_node& node);
    void get_node(pagenum_t pagenum, node_t& node);
    void write_node(pagenum_t pagenum, node_t& node);

    pagenum_t get_new_node();
    void delete_node(pagenum_t pagenum);

    pagenum_t get_root();
    void set_root(pagenum_t new_pagenum);

    ~DBFileUtils() {
        for (auto& table : table_list) {
            close_db_file(std::get<0>(table));
        }
    }
private:
    DBFileUtils() {
        open_table_num_ = 0;
    }

    int close_db_fd(int fd);

    void get_header_page();
    void update_header_to_file();

    // 현재 테이블 id
    uint32_t current_table_id_;
    // 현재 열려있는 테이블 개수
    uint32_t open_table_num_;
    // 테이블 리스트 - vector<tuple<table_id, pathname, fd>> 
    std::vector<std::tuple<uint32_t, std::string, int>> table_list;
    // 헤더 페이지
    page_t header_page_;
};

#endif