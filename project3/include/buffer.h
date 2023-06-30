#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "page.h"

struct BufferBlock {
    page_t frame_;
    uint32_t table_id_;
    pagenum_t pagenum_;
    bool is_dirty_;
    int pin_count_;
    int next_block_;
    int prev_block_;

    void init(uint32_t table_id, pagenum_t pagenum) {
        table_id_ = table_id;
        pagenum_ = pagenum;
        is_dirty_ = false;
        pin_count_ = 0;
    }
};

#endif