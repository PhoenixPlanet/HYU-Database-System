#ifndef __BPT_H__
#define __BPT_H__

#include <iostream>
#include <fstream>
#include <set>

#include "buffer_manager.h"
#include "bpt_node.h"
#include "lock_manager.h"

class BPT {
public:
    static constexpr uint32_t INTERNAL_ORDER = 249;
    static constexpr uint32_t LEAF_ORDER = 32;

    BPT();
    
    bool init_db(int buffer_size);
    bool shutdown_db();

    uint32_t open_table(const char* pathname);
    bool close_table(uint32_t table_id);
    
    int find(uint32_t table_id, int64_t key, char* ret_val, int trx_id);
    int update(uint32_t table_id, int64_t key, char* values, int trx_id);
    bool insert_key(uint32_t table_id, int64_t key, char* value);
    bool delete_key(uint32_t table_id, int64_t key);
    void print_tree(uint32_t table_id, std::ostream& out);
    void print_leaf(uint32_t table_id, std::ostream& out);

    bool is_initialized();

private:
    bool is_table_exist(uint32_t table_id);

    void update_root_pagenum(uint32_t table_id, pagenum_t new_root);
    pagenum_t get_root(uint32_t table_id);

    uint32_t cut(uint32_t length);

    bool find_leaf(uint32_t table_id, int64_t key, BPTNode<NodeType::LEAF>& leaf);
    int find(uint32_t table_id, int64_t key, record_t& record, int trx_id);
    bool find(uint32_t table_id, int64_t key, record_t& record);
    bool is_key_exist(uint32_t table_id, int64_t key);
    int get_target_index(BPTNode<NodeType::INTERNAL>& parent, pagenum_t left);

    void insert_into_leaf(BPTNode<NodeType::LEAF>& leaf, const record_t& record);
    void insert_into_leaf_after_splitting(uint32_t table_id, BPTNode<NodeType::LEAF>& leaf, const record_t& record);
    void insert_into_parent(uint32_t table_id, pagenum_t parent, pagenum_t left, pagenum_t right, int64_t key);
    void insert_into_new_root(uint32_t table_id, pagenum_t left, pagenum_t right, int64_t key);
    void insert_into_node(BPTNode<NodeType::INTERNAL>& parent, int left_index, int64_t key, pagenum_t right);
    void insert_into_node_after_splitting(uint32_t table_id, BPTNode<NodeType::INTERNAL>& parent, int left_index, int64_t key, pagenum_t right);
    pagenum_t make_node(uint32_t table_id, bool is_leaf);
    void start_new_tree(uint32_t table_id, const record_t& record);

    void delete_leaf_entry(uint32_t table_id, BPTNode<NodeType::LEAF>& node, int64_t key);
    void delete_node_entry(uint32_t table_id, BPTNode<NodeType::INTERNAL>& node, int64_t key);
    void remove_entry_from_node(BPTNode<NodeType::INTERNAL>& node, int64_t key);
    void remove_entry_from_leaf(BPTNode<NodeType::LEAF>& node, int64_t key);
    void adjust_leaf_root(uint32_t table_id, BPTNode<NodeType::LEAF>& root);
    void adjust_node_root(uint32_t table_id, BPTNode<NodeType::INTERNAL>& root);
    void coalesce_leaf_nodes(uint32_t table_id, BPTNode<NodeType::LEAF>& node, BPTNode<NodeType::INTERNAL>& parent, int target_idx);
    void coalesce_internal_nodes(uint32_t table_id, BPTNode<NodeType::INTERNAL>& node, BPTNode<NodeType::INTERNAL>& parent, int target_idx);
    void redistribute_nodes(uint32_t table_id, BPTNode<NodeType::INTERNAL>& node, BPTNode<NodeType::INTERNAL>& neighbor, BPTNode<NodeType::INTERNAL>& parent, int target_idx, bool is_leftmost);

    std::set<uint32_t> table_id_list_;
    bool is_db_initialized_;
    BufferManager& buffer_manager_;
};

#endif
