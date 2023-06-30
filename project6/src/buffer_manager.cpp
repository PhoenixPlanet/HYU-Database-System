#include "buffer_manager.h"

// 버퍼 초기화: 버퍼 capacity 설정
// 성공 시 true, 실패 시 false return
bool BufferManager::init_buffer(int capacity) {
    if (is_initialized_) {
        perror("Can't init buffer - already initialized buffer");
        return false;
    }

    capacity_ = capacity;
    size_ = 0;
    buffer_head_ = HEAD_IDX;
    buffer_tail_ = TAIL_IDX;
    buffer_ = new BufferBlock[capacity_];
    is_initialized_ = true;
    for (int i = 0; i < capacity_; i++) {
        unused_blocks_.push(i);
    }

    return true;
}

pagenum_t BufferManager::get_new_node(uint32_t table_id) {
    int header_block_idx;
    page_t& header = start_use_node(table_id, 0, header_block_idx).frame_;
    
    pagenum_t new_node_pagenum = file_manager_.get_new_node(table_id, header);
    
    int new_node_block_idx;
    page_t& new_node = start_use_node(table_id, new_node_pagenum, new_node_block_idx).frame_;

    header.page_.header_page_.free_page_num = new_node.page_.free_page_.next_free_page_num;
    memset(&new_node, 0, sizeof(page_t));

    end_use_node(header_block_idx, true);
    end_use_node(new_node_block_idx, true);

    return new_node_pagenum;
}

// TODO: 구현 필요
void BufferManager::delete_node(uint32_t table_id, pagenum_t pagenum) {
    int header_block_idx;
    page_t& header = start_use_node(table_id, 0, header_block_idx).frame_;

    int target_block_idx;
    page_t& target_node = start_use_node(table_id, pagenum, target_block_idx).frame_;

    memset(&target_node, 0, sizeof(page_t));
    target_node.page_.free_page_.next_free_page_num = header.page_.header_page_.free_page_num;
    header.page_.header_page_.free_page_num = pagenum;

    end_use_node(header_block_idx, true);
    end_use_node(target_block_idx, true);
}

BufferBlock& BufferManager::start_use_node(uint32_t table_id, pagenum_t pagenum, int& block_idx) {
    std::unique_lock<std::mutex> buffer_lock(buffer_manager_latch_);
    
    int target_idx;
    auto block_it = block_list_.find(std::make_pair(table_id, pagenum));

    if (block_it == block_list_.end()) {
        target_idx = get_page_from_file(table_id, pagenum);
    } else {
        target_idx = (*block_it).second;

        if (target_idx != buffer_head_) {
            buffer_[buffer_[target_idx].prev_block_].next_block_ = 
                buffer_[target_idx].next_block_;

            if (buffer_[target_idx].next_block_ == TAIL_IDX) {
                buffer_tail_ = buffer_[target_idx].prev_block_;
            } else {
                buffer_[buffer_[target_idx].next_block_].prev_block_ =
                    buffer_[target_idx].prev_block_;
            }

            buffer_[buffer_head_].prev_block_ = target_idx;
            buffer_[target_idx].next_block_ = buffer_head_;
            buffer_[target_idx].prev_block_ = HEAD_IDX;
            buffer_head_ = target_idx;
        }
    }

    //debug
    //assert(buffer_[target_idx].pin_count_ == 0);
    
    buffer_[target_idx].page_lock_.lock();
    block_idx = target_idx;
    return buffer_[target_idx];
}

void BufferManager::end_use_node(int block_idx, bool is_changed) {
    //debug
    //assert(buffer_[block_idx].pin_count_ == 1);
    //std::unique_lock<std::mutex> buffer_lock(buffer_manager_latch_);

    if (is_changed) {
        buffer_[block_idx].is_dirty_ = true;
    }

    buffer_[block_idx].page_lock_.unlock();
}

// 파일에서 해당하는 페이지를 읽어와 버퍼에 넣음
// 해당하는 버퍼의 idx를 반환
int BufferManager::get_page_from_file(uint32_t table_id, pagenum_t pagenum) {
    int target_idx;
    if (capacity_ <= size_) {
        target_idx = evict_buffer_block();
    } else {
        target_idx = unused_blocks_.front();
        unused_blocks_.pop();
    }

    file_manager_.get_node(table_id, pagenum, buffer_[target_idx].frame_);
    buffer_[target_idx].init(table_id, pagenum);
    if (size_ == 0) {
        buffer_[target_idx].next_block_ = TAIL_IDX;
        buffer_[target_idx].prev_block_ = HEAD_IDX;
        buffer_tail_ = target_idx;
        buffer_head_ = target_idx;
    } else {
        buffer_[buffer_head_].prev_block_ = target_idx;
        buffer_[target_idx].next_block_ = buffer_head_;
        buffer_[target_idx].prev_block_ = HEAD_IDX;
        buffer_head_ = target_idx;
    }

    size_++;
    block_list_[std::make_pair(table_id, pagenum)] = target_idx;
    return target_idx;
}

// pin count가 0인 블록 중에서 LRU policy에 따라 블록 하나를 추방한다.
// 해당하는 버퍼의 idx를 반환
int BufferManager::evict_buffer_block() {
    int target_idx = buffer_tail_;

    erase_buffer_block(target_idx);
    return target_idx;
}

void BufferManager::erase_buffer_block(int target_idx) {
    std::unique_lock<std::mutex> page_lock(buffer_[target_idx].page_lock_);

    if (buffer_[target_idx].is_dirty_) {
        set_page_to_file(target_idx);
    }

    if (buffer_[target_idx].prev_block_ == HEAD_IDX) {
        buffer_head_ = buffer_[target_idx].next_block_;
    } else {
        buffer_[buffer_[target_idx].prev_block_].next_block_ = 
            buffer_[target_idx].next_block_;
    }

    if (buffer_[target_idx].next_block_ == TAIL_IDX) {
        buffer_tail_ = buffer_[target_idx].prev_block_;
    } else {
        buffer_[buffer_[target_idx].next_block_].prev_block_ =
            buffer_[target_idx].prev_block_;
    }

    size_--;
    block_list_.erase(std::make_pair(buffer_[target_idx].table_id_, 
                                     buffer_[target_idx].pagenum_));
}

void BufferManager::set_page_to_file(int target_idx) {
    if (LogManager::instance().isPageExistInLog(target_idx)) {
        LogManager::instance().force();
    }
    file_manager_.write_node(buffer_[target_idx].table_id_, 
                             buffer_[target_idx].pagenum_,
                             buffer_[target_idx].frame_);
}

// 성공시 table id, 실패시 -1 return, 
int BufferManager::open_table(const char* pathname) {
    int new_table_id = file_manager_.open_db_file(pathname);
    if (new_table_id == 0) {
        return -1;
    } else {
        return new_table_id;
    }
}

// 실패 시 0보다 작은 값 return
int BufferManager::close_table(uint32_t table_id) {
    int next_block;
    for (int i = buffer_head_; i != TAIL_IDX && i != HEAD_IDX; i = next_block) {
        next_block = buffer_[i].next_block_;
        if (buffer_[i].table_id_ == table_id) {
            erase_buffer_block(i);
            unused_blocks_.push(i);
        }
    }

    return file_manager_.close_db_file(table_id);
}

void BufferManager::shutdown_buffer() {
    for (int i = buffer_head_; i != TAIL_IDX && i != HEAD_IDX; i = buffer_[i].next_block_) {
        erase_buffer_block(i);
    }

    file_manager_.close_all_db_file();
    delete[] buffer_;
    
    std::queue<int> empty;
    std::swap(unused_blocks_, empty);
    block_list_.clear();

    is_initialized_ = false;
}