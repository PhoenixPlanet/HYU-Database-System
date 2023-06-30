#ifndef __BPT_NODE_H__
#define __BPT_NODE_H__

#include <mutex>
#include <type_traits>

#include "buffer_manager.h"

using children_t = std::array<child_t, 248>;
using records_t = std::array<record_t, 31>;

class HeaderNode {
public:
    HeaderNode() : buffer_manager_(BufferManager::instance()),
                   is_initialized_(false),
                   is_end_use_(false),
                   is_changed_(false) { }
    HeaderNode(uint32_t table_id);

    ~HeaderNode();

    void init_node(uint32_t table_id);
    void end_use_node();

    pagenum_t root();
    void root(const pagenum_t root_n);

private:
    page_t* header_page_;
    int buffer_idx_;

    BufferManager& buffer_manager_;
    bool is_initialized_;
    bool is_end_use_;
    bool is_changed_;

    std::mutex* page_lock_;
};

template <NodeType T>
class BPTNode {
public:
    using ChildType = typename std::conditional<T == NodeType::INTERNAL, children_t, records_t>::type;

    BPTNode() : buffer_manager_(BufferManager::instance()),
                is_initialized_(false),
                is_end_use_(false),
                is_changed_(false) { }

    BPTNode(uint32_t table_id, pagenum_t pagenum);

    ~BPTNode();

    void init_node(uint32_t table_id, pagenum_t pagenum);
    void end_use_node();

    void set_new_node();

    pagenum_t get_pagenum();

    void set_page(const page_t& new_page);
    const page_t& get_page();

    pagenum_t parent();
    void parent(const pagenum_t parent_n);

    bool is_leaf();

    uint32_t num_keys();
    void num_keys(const uint32_t num_keys_n);
    void increase_num_keys(const uint32_t i);
    void decrease_num_keys(const uint32_t i);

    pagenum_t spare_page();
    void spare_page(const pagenum_t spare_n);

    const ChildType& read_data() {
        if constexpr (std::is_same<ChildType, children_t>::value){
            return node_->page_.node_page_.body_.childeren;
        } else {
            return node_->page_.node_page_.body_.records;
        }
    }

    ChildType& write_data() {
        is_changed_ = true;
        if constexpr (std::is_same<ChildType, children_t>::value){
            return node_->page_.node_page_.body_.childeren;
        } else {
            return node_->page_.node_page_.body_.records;
        }
    }

    const children_t& children();
    const records_t& records();

private:
    page_t* node_;
    int buffer_idx_;

    BufferManager& buffer_manager_;
    bool is_initialized_;
    bool is_end_use_;
    bool is_changed_;

    pagenum_t pagenum_;
    std::mutex* page_lock_;
};

#include "bpt_node_impl.hpp"

#endif