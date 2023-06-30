#ifndef __BPT_H__
#define __BPT_H__

#include <iostream>
#include <fstream>

#include "fileutils.h"

class BPT {
public:
    static constexpr uint32_t INTERNAL_ORDER = 249;
    static constexpr uint32_t LEAF_ORDER = 32;

    BPT();
    
    uint32_t open_table(const char* pathname);
    uint32_t set_table(uint32_t table_id);
    bool find(int64_t key, char* ret_val);
    bool insert_key(int64_t key, char* value);
    bool delete_key(int64_t key);
    void print_tree(std::ostream& out);
    void print_leaf(std::ostream& out);

    bool is_open();

private:
    void update_root_pagenum(pagenum_t new_root);
    uint32_t cut(uint32_t length);

    bool find_leaf(int64_t key, bpt_node& leaf);
    bool find(int64_t key, record_t& record);
    bool is_key_exist(int64_t key);
    int get_target_index(bpt_node& parent, pagenum_t left);

    void insert_into_leaf(bpt_node& leaf, const record_t& record);
    void insert_into_leaf_after_splitting(bpt_node& leaf, const record_t& record);
    void insert_into_parent(pagenum_t parent, pagenum_t left, pagenum_t right, int64_t key);
    void insert_into_new_root(pagenum_t left, pagenum_t right, int64_t key);
    void insert_into_node(bpt_node& parent, int left_index, int64_t key, pagenum_t right);
    void insert_into_node_after_splitting(bpt_node& parent, int left_index, int64_t key, pagenum_t right);
    pagenum_t make_node(bool is_leaf);
    void start_new_tree(const record_t& record);

    void delete_entry(bpt_node& node, int64_t key);
    void remove_entry_from_node(bpt_node& node, int64_t key);
    void remove_entry_from_leaf(bpt_node& node, int64_t key);
    void adjust_root(bpt_node& root);
    void coalesce_nodes(bpt_node& node, bpt_node& parent, int target_idx);
    void redistribute_nodes(bpt_node& node, bpt_node& neighbor, bpt_node& parent, int target_idx, bool is_leftmost);

    DBFileUtils file_manager_;
    uint32_t table_id_;
    pagenum_t root_pagenum_;
    bool is_open_;
};

#endif
