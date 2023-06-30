#include "page.h"

record_t::record_t() : key_(0) {    
    memset(value_, 0, 120);
}

record_t::record_t(int64_t key, char value[]) : key_(key) {   
    strncpy(value_, value, 119);
    value_[119] = '\0';
}

child_t::child_t() : key_(0), child_(0) { }

child_t::child_t(int64_t key, pagenum_t child) : key_(key), child_(child) {  }

page_t::page::node_page::body::body() {
    memset(this, 0, sizeof(body));
}

page_t::page::page() {
    memset(this, 0, sizeof(page));
}