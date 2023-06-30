#ifndef __BUFFER_MANAGER_H__
#define __BUFFER_MANAGER_H__

#include <unordered_map>
#include <queue>
#include <utility>

#include "file_manager.h"
#include "buffer.h"
#include "enums.h"

namespace std {
template <class T1, class T2>
struct hash<std::pair<T1, T2>> {
    size_t operator() (const std::pair<T1, T2>& p) const {
        auto a = std::hash<T1>{}(p.first);
        auto b = std::hash<T2>{}(p.second);
        
        return a ^ b;
    }
};
}

class BufferManager {
public:
    static BufferManager& instance() {
        static BufferManager instance_;
        return instance_;
    }

    bool init_buffer(int capacity);

    pagenum_t get_new_node(uint32_t table_id);
    void delete_node(uint32_t table_id, pagenum_t pagenum);

    int open_table(const char* pathname);
    int close_table(uint32_t table_id);
    void shutdown_buffer();

    page_t& start_use_node(uint32_t table_id, pagenum_t pagenum, int& block_idx);
    void end_use_node(int block_idx, bool is_changed);

private:
    BufferManager() : file_manager_(FileManager::instance()), 
                      capacity_(0), 
                      size_(0),
                      buffer_(nullptr),
                      is_initialized_(false) {    }

    ~BufferManager() {
        if (is_initialized_) {
            shutdown_buffer();
        }
    }

    FileManager& file_manager_;
    
    int capacity_;
    int size_;

    int evict_buffer_block();
    void erase_buffer_block(int target_idx);

    int get_page_from_file(uint32_t table_id, pagenum_t pagenum);
    void set_page_to_file(int target_idx);

    static constexpr int HEAD_IDX = -1;
    static constexpr int TAIL_IDX = -2;

    BufferBlock* buffer_;
    std::queue<int> unused_blocks_;
    std::unordered_map<std::pair<uint32_t, pagenum_t>, int> block_list_;
    int buffer_head_;
    int buffer_tail_;
    
    bool is_initialized_;
};

#endif