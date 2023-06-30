#include "bpt_node.h"

HeaderNode::HeaderNode(uint32_t table_id) : buffer_manager_(BufferManager::instance()),
                                            is_changed_(false) {
    BufferBlock& block = buffer_manager_.start_use_node(table_id, 0, buffer_idx_);
    page_lock_ = &(block.page_lock_);
    page_t& header = block.frame_;
    header_page_ = &header;

    is_initialized_ = true;
    is_end_use_ = false;
}

void HeaderNode::init_node(uint32_t table_id) {
    if (is_initialized_ && !is_end_use_) {
        end_use_node();
    }
    
    BufferBlock& block = buffer_manager_.start_use_node(table_id, 0, buffer_idx_);
    page_lock_ = &(block.page_lock_);
    page_t& header = block.frame_;
    header_page_ = &header;

    is_initialized_ = true;
    is_end_use_ = false;
    is_changed_ = false;
}

HeaderNode::~HeaderNode() {
    if (is_initialized_ && !is_end_use_) {
        end_use_node();
    }
}

pagenum_t HeaderNode::root() {
    return header_page_->page_.header_page_.root_page_num;
}

void HeaderNode::root(const pagenum_t root_n) {
    header_page_->page_.header_page_.root_page_num = root_n;
    is_changed_ = true;
}

void HeaderNode::end_use_node() {
    if (is_initialized_ && !is_end_use_) {
        buffer_manager_.end_use_node(buffer_idx_, is_changed_);
        is_end_use_ = true;
    }
}