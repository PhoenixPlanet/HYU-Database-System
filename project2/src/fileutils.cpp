#include "fileutils.h"

bpt_node::bpt_node() : auto_write_(false) {   }

bpt_node::bpt_node(pagenum_t pagenum, bool auto_get, bool auto_write) : pagenum_(pagenum), auto_write_(auto_write){
    if (auto_get) {
        DBFileUtils::instance().get_node(pagenum_, page_);
    }
}

void bpt_node::write_to_file() {
    DBFileUtils::instance().write_node(pagenum_, page_);
}

bpt_node::~bpt_node() {
    if (auto_write_) {
        write_to_file();
    }
}

int DBFileUtils::open_db_file(const char* pathname) {
    // 열려있지 않은 파일이라면 새로운 db file 열고 추가
    int fd = file_open_db(pathname);
    if (fd < 0) {
        return fd;
    }
    table_list.push_back(std::make_tuple(open_table_num_, std::string(pathname), fd));
    current_table_id_ = open_table_num_;
    open_table_num_++;

    get_header_page();

    return current_table_id_;
}

int DBFileUtils::set_db_file(uint32_t table_id) {
    auto it = std::find_if(table_list.begin(), table_list.end(), 
                            [table_id] (decltype(*table_list.begin()) e) 
                            { return std::get<0>(e) == table_id; });

    if (it == table_list.end()) {
        fprintf(stderr, "set - Table id %d not exist\n", table_id);
        return -1;
    }

    set_current_db_fd(std::get<2>(*it));
    current_table_id_ = (std::get<0>(*it));

    get_header_page();

    return current_table_id_;
}

int DBFileUtils::close_db_file(uint32_t table_id) {
    auto it = std::find_if(table_list.begin(), table_list.end(), 
                            [table_id] (decltype(*table_list.begin()) e) 
                            { return std::get<0>(e) == table_id; });

    if (it == table_list.end()) {
        fprintf(stderr, "close - Table id %d not exist\n", table_id);
        return -1;
    }

    int flag = close_db_fd(std::get<2>(*it));
    table_list.erase(it);
    return flag;
}

int DBFileUtils::close_db_fd(int fd) {
    return file_close_db(fd);
}

void DBFileUtils::get_node(pagenum_t pagenum, bpt_node& node) {
    page_t* page = new page_t;
    file_read_page(pagenum, page);
    memcpy(&(node.page_), page, PAGE_BYTE);
    node.pagenum_ = pagenum;
    delete page;
}

void DBFileUtils::get_node(pagenum_t pagenum, node_t& node) {
    page_t* page = new page_t;
    file_read_page(pagenum, page);
    memcpy(&(node), page, PAGE_BYTE);
    delete page;
}

void DBFileUtils::write_node(pagenum_t pagenum, node_t& node) {
    //debug
    if (pagenum == 0) {
        perror("tried to write page on header\n");
    }
    page_t* page = new page_t;
    memcpy(page, &(node), PAGE_BYTE);
    file_write_page(pagenum, page);
    delete page;
}

void DBFileUtils::write_node(bpt_node& node) {
    page_t* page = new page_t;
    memcpy(page, &(node.page_), PAGE_BYTE);
    file_write_page(node.pagenum_, page);
    delete page;
}

void DBFileUtils::get_header_page() {
    file_read_page(0, &header_page_);
}

void DBFileUtils::update_header_to_file() {
    file_write_page(0, &header_page_);
}

pagenum_t DBFileUtils::get_root() {
    return header_page_.page_.header_page_.root_page_num;
}

void DBFileUtils::set_root(pagenum_t new_pagenum) {
    header_page_.page_.header_page_.root_page_num = new_pagenum;
    update_header_to_file();
}

pagenum_t DBFileUtils::get_new_node() {
    pagenum_t new_pagenum = file_alloc_page();
    get_header_page();
    return new_pagenum;
}

void DBFileUtils::delete_node(pagenum_t pagenum) {
    file_free_page(pagenum);
    get_header_page();
}