#include "bpt.h"

#include <queue>
#include <iostream>

BPT::BPT() : is_db_initialized_(false),
             buffer_manager_(BufferManager::instance()) {    }

uint32_t BPT::cut(uint32_t length) {
    if (length % 2 == 0)
        return length / 2;
    else
        return length / 2 + 1;
}

// 성공 시 true, 실패시 false return
bool BPT::init_db(int buffer_size) {
    if (buffer_size <= 0) {
        #ifdef DEBUG_BPT
        perror("Wrong buffer size");
        #endif
        return false;
    }

    if (is_db_initialized_) {
        #ifdef DEBUG_BPT
        perror("DB already initialized");
        #endif
        return false;
    }

    if (buffer_manager_.init_buffer(buffer_size)) {
        is_db_initialized_ = true;
        table_id_list_.clear();
        return true;
    } else {
        is_db_initialized_ = false;
        return false;
    }
}

bool BPT::shutdown_db() {
    if (!is_db_initialized_) {
        #ifdef DEBUG_BPT
        perror("DB not initialized");
        #endif
        return false;
    }

    buffer_manager_.shutdown_buffer();
    table_id_list_.clear();
    is_db_initialized_ = false;
    return true;
}

bool BPT::is_initialized() {
    return is_db_initialized_;
}

// 성공 시 table_id, 실패시 -1 return
uint32_t BPT::open_table(const char* pathname) {
    if (!is_db_initialized_) {
        #ifdef DEBUG_BPT
        perror("DB not initialized");
        #endif
        return -1;
    }

    int table_id = buffer_manager_.open_table(pathname);
    if (table_id < 0) {
        return table_id;
    }

    table_id_list_.insert(table_id);
    return table_id;
}

bool BPT::close_table(uint32_t table_id) {
    if (!is_db_initialized_) {
        #ifdef DEBUG_BPT
        perror("DB not initialized");
        #endif
        return false;
    }

    if (!is_table_exist(table_id)) {
        #ifdef DEBUG_BPT
        fprintf(stderr, "Table id %d not exist", table_id);
        #endif
        return false;
    }

    int flag = buffer_manager_.close_table(table_id);
    if (flag < 0) {
        return false;
    }

    table_id_list_.erase(table_id);
    return true;
}

bool BPT::is_table_exist(uint32_t table_id) {
    if (!is_db_initialized_) {
        #ifdef DEBUG_BPT
        perror("DB not initialized");
        #endif
        return false;
    }

    if (table_id_list_.find(table_id) == table_id_list_.end()) {
        return false;
    } else {
        return true;
    }
}

void BPT::update_root_pagenum(uint32_t table_id, pagenum_t new_root) {
    HeaderNode header(table_id);
    header.root(new_root);
    header.end_use_node();
}

pagenum_t BPT::get_root(uint32_t table_id) {
    HeaderNode header(table_id);
    return header.root();
}

// 찾았을 때 true 트리가 없을 때 false return
bool BPT::find_leaf(uint32_t table_id, int64_t key, BPTNode<NodeType::LEAF>& leaf) {
    // 루트 페이지 읽어오기
    pagenum_t root_pagenum = get_root(table_id);
    if (root_pagenum == 0) {
        return false;
    }

    leaf.init_node(table_id, root_pagenum);

    while (!leaf.is_leaf()) {
        if (key < leaf.children()[0].key_) {
            leaf.init_node(table_id, leaf.spare_page());
        } else {
            int number_of_keys = static_cast<int>(leaf.num_keys());
            int i;
            for (i = 0; i < number_of_keys - 1; i++) {
                if (key < leaf.children()[i + 1].key_) {
                    break;
                }
            }
            leaf.init_node(table_id, leaf.children()[i].child_);
        }
    }

    return true;
}

// 찾았을 때 true 아닐 때 false return
bool BPT::find(uint32_t table_id, int64_t key, record_t& record) {
    BPTNode<NodeType::LEAF> leaf;
    if(!find_leaf(table_id, key, leaf)) {
        return false;
    }

    int number_of_keys = static_cast<int>(leaf.num_keys());
    for (int i = 0; i < number_of_keys; i++) {
        if (leaf.read_data()[i].key_ == key) {
            record = leaf.read_data()[i];
            return true;
        }
    }
    return false;
}

bool BPT::find(uint32_t table_id, int64_t key, char* ret_val) {
    if (!is_db_initialized_) {
        #ifdef DEBUG_BPT
        perror("DB not initialized");
        #endif
        return false;
    }

    if (!is_table_exist(table_id)) {
        #ifdef DEBUG_BPT
        fprintf(stderr, "Table id %d not exist", table_id);
        #endif
        return false;
    }

    record_t rec;

    bool flag = find(table_id, key, rec);
    if (flag) {
        strncpy(ret_val, rec.value_, 119);
        ret_val[119] = '\0';
    }

    return flag;
}

// 찾았을 때 true 아닐 때 false return
bool BPT::is_key_exist(uint32_t table_id, int64_t key) {
    BPTNode<NodeType::LEAF> leaf;
    if(!find_leaf(table_id, key, leaf)) {
        return false;
    }

    int number_of_keys = static_cast<int>(leaf.num_keys());
    for (int i = 0; i < number_of_keys; i++) {
        if (leaf.read_data()[i].key_ == key) {
            return true;
        }
    }
    return false;
}

int BPT::get_target_index(BPTNode<NodeType::INTERNAL>& parent, pagenum_t target) {
    if (parent.spare_page() == target) {
        return -1;
    }

    int left_index = 0;
    for (; left_index <= static_cast<int>(parent.num_keys()) 
            && parent.read_data()[left_index].child_ != target; left_index++);
    return left_index;
}

// 삽입 성공했을 때 true 아닐 때 false return
bool BPT::insert_key(uint32_t table_id, int64_t key, char* value) {
    if (!is_db_initialized_) {
        #ifdef DEBUG_BPT
        perror("DB not initialized");
        #endif
        return false;
    }

    if (!is_table_exist(table_id)) {
        #ifdef DEBUG_BPT
        fprintf(stderr, "Table id %d not exist", table_id);
        #endif
        return false;
    }

    if (is_key_exist(table_id, key)) {
        return false;
    }

    record_t record(key, value);

    if (get_root(table_id) == 0) {
        start_new_tree(table_id, record);
        return true;
    }

    BPTNode<NodeType::LEAF> leaf;
    find_leaf(table_id, key, leaf);

    if (leaf.num_keys() < LEAF_ORDER - 1) {
        insert_into_leaf(leaf, record);
        return true;
    }

    insert_into_leaf_after_splitting(table_id, leaf, record);
    return true;
}

void BPT::insert_into_leaf(BPTNode<NodeType::LEAF>& leaf, const record_t& record) {
    int number_of_keys = static_cast<int>(leaf.num_keys());
    int insertion_point = 0;

    for (; insertion_point < number_of_keys && 
           leaf.read_data()[insertion_point].key_ < record.key_;
           insertion_point++);
    
    for (int i = number_of_keys; i > insertion_point; i--) {
        leaf.write_data()[i] = leaf.read_data()[i - 1];
    }
    leaf.write_data()[insertion_point] = record;

    leaf.increase_num_keys(1);
    leaf.end_use_node();
}

void BPT::insert_into_node(BPTNode<NodeType::INTERNAL>& parent, int left_index, int64_t key, pagenum_t right) {
    //debug
    /*if (parent.pagenum_ == 3915) {
        printf("error");
    }*/
    int number_of_keys = static_cast<int>(parent.num_keys());

    for (int i = number_of_keys; i > left_index + 1; i--) {
        parent.write_data()[i] = parent.read_data()[i - 1];
    }
    child_t right_child(key, right);
    parent.write_data()[left_index + 1] = right_child; 
    parent.increase_num_keys(1);
    parent.end_use_node();
}

void BPT::insert_into_leaf_after_splitting(uint32_t table_id, BPTNode<NodeType::LEAF>& leaf, const record_t& record) {
    pagenum_t new_leaf_pagenum = make_node(table_id, true);
    BPTNode<NodeType::LEAF> new_leaf(table_id, new_leaf_pagenum);
   
    int split = static_cast<int>(cut(LEAF_ORDER - 1));
    int insertion_point = 0;

    for (; insertion_point < static_cast<int>(LEAF_ORDER) - 1 && 
           leaf.read_data()[insertion_point].key_ < record.key_;
           insertion_point++);
    
    bool inserted_left = false;
    record_t temp_record;
    if (insertion_point < split) {
        inserted_left = true;
        temp_record = leaf.read_data()[split - 1];
        for (int i = split - 1; i > insertion_point; i--) {
            leaf.write_data()[i] = leaf.read_data()[i - 1];
        }
        leaf.write_data()[insertion_point] = record;
    }

    if (inserted_left) {
        insertion_point = 0;
    } else {
        insertion_point -= split;
    }

    for (int i = split, j = 0; i < static_cast<int>(LEAF_ORDER) - 1; i++, j++) {
        if (j == insertion_point) {
            j++;
        }
        new_leaf.write_data()[j] = leaf.read_data()[i];
    }

    if (inserted_left) {
        new_leaf.write_data()[insertion_point] = temp_record;
    } else {
        new_leaf.write_data()[insertion_point] = record;
    }

    leaf.num_keys(split);
    new_leaf.num_keys(LEAF_ORDER - split);

    new_leaf.spare_page(leaf.spare_page());
    leaf.spare_page(new_leaf.get_pagenum());

    new_leaf.parent(leaf.parent());

    for (uint32_t i = leaf.num_keys(); i < LEAF_ORDER - 1; i++) {
        memset(&(leaf.write_data()[i]), 0, sizeof(record_t));
    }
    
    for (uint32_t i = new_leaf.num_keys(); i < LEAF_ORDER - 1; i++) {
        memset(&(new_leaf.write_data()[i]), 0, sizeof(record_t));
    }

    int64_t new_key = new_leaf.read_data()[0].key_;

    pagenum_t leaf_parent = leaf.parent();
    leaf.end_use_node();
    new_leaf.end_use_node();
    insert_into_parent(table_id, leaf_parent, leaf.get_pagenum(), new_leaf.get_pagenum(), new_key);
}

void BPT::insert_into_parent(uint32_t table_id, pagenum_t parent, pagenum_t left, pagenum_t right, int64_t key) {
    if (parent == 0) {
        insert_into_new_root(table_id, left, right, key);
        return;
    }

    BPTNode<NodeType::INTERNAL> parent_node(table_id, parent);
    int left_index = get_target_index(parent_node, left);

    if (parent_node.num_keys() < INTERNAL_ORDER - 1) {
        insert_into_node(parent_node, left_index, key, right);
        return;
    }

    insert_into_node_after_splitting(table_id, parent_node, left_index, key, right);
}

void BPT::insert_into_new_root(uint32_t table_id, pagenum_t left, pagenum_t right, int64_t key) {
    pagenum_t root_pagenum = make_node(table_id, false);
    BPTNode<NodeType::INTERNAL> root(table_id, root_pagenum);
    root.spare_page(left);
    child_t child(key, right);
    root.write_data()[0] = child;
    root.increase_num_keys(1);
    
    BPTNode<NodeType::INTERNAL> left_node(table_id, left);
    BPTNode<NodeType::INTERNAL> right_node(table_id, right);

    left_node.parent(root_pagenum);
    right_node.parent(root_pagenum);

    root.end_use_node();
    left_node.end_use_node();
    right_node.end_use_node();

    update_root_pagenum(table_id, root_pagenum);
}

void BPT::insert_into_node_after_splitting(uint32_t table_id, BPTNode<NodeType::INTERNAL>& parent, int left_index, int64_t key, pagenum_t right) {
    pagenum_t new_parent_pagenum = make_node(table_id, false);
    //debug
    /*if (new_parent_pagenum == 3915 || parent.pagenum_ == 3915) {
        printf("error");
    }*/
    
    BPTNode<NodeType::INTERNAL> new_parent(table_id, new_parent_pagenum);
    int split = static_cast<int>(cut(INTERNAL_ORDER - 1));
    int insertion_point = left_index + 1;

    child_t right_child(key, right);
    bool inserted_left = false;
    child_t temp_child;
    if (insertion_point < split) {
        inserted_left = true;
        temp_child = parent.read_data()[split - 1];
        for (int i = split - 1; i > insertion_point; i--) {
            parent.write_data()[i] = parent.read_data()[i - 1];
        }
        parent.write_data()[insertion_point] = right_child;
    }

    int start = split;
    if (!inserted_left) {
        insertion_point -= split + 1;
        if (insertion_point < 0) {
            temp_child = right_child;
        } else {
            temp_child = parent.read_data()[split];
            start = split + 1;
        }
    }

    for (int i = start, j = 0; i < static_cast<int>(INTERNAL_ORDER) - 1; i++, j++) {
        if (j == insertion_point && !inserted_left) {
            new_parent.write_data()[j] = right_child;
            BPTNode<NodeType::INTERNAL> right_child_node(table_id, right_child.child_);
            right_child_node.parent(new_parent.get_pagenum());
            right_child_node.end_use_node();
            j++;
        }
        new_parent.write_data()[j] = parent.read_data()[i];
        BPTNode<NodeType::INTERNAL> child_temp(table_id, parent.read_data()[i].child_);
        child_temp.parent(new_parent.get_pagenum());
        child_temp.end_use_node();
    }
    if (static_cast<int>(INTERNAL_ORDER - split - 2) == insertion_point && !inserted_left) {
        new_parent.write_data()[insertion_point] = right_child;
        BPTNode<NodeType::INTERNAL> right_child_node(table_id, right_child.child_);
        right_child_node.parent(new_parent.get_pagenum());
        right_child_node.end_use_node();
    }

    parent.num_keys(split);
    new_parent.num_keys(INTERNAL_ORDER - split - 1);

    new_parent.spare_page(temp_child.child_);
    BPTNode<NodeType::INTERNAL> spare_child_node(table_id, temp_child.child_);
    spare_child_node.parent(new_parent.get_pagenum());
    spare_child_node.end_use_node();
    
    new_parent.parent(parent.parent());

    for (uint32_t i = parent.num_keys(); i < INTERNAL_ORDER - 1; i++) {
        memset(&(parent.write_data()[i]), 0, sizeof(child_t));
    }

    for (uint32_t i = new_parent.num_keys(); i < INTERNAL_ORDER - 1; i++) {
        memset(&(new_parent.write_data()[i]), 0, sizeof(child_t));
    }
    
    pagenum_t parent_of_parent = parent.parent();

    parent.end_use_node();
    new_parent.end_use_node();

    insert_into_parent(table_id, parent_of_parent, parent.get_pagenum(), new_parent.get_pagenum(), temp_child.key_);
}

pagenum_t BPT::make_node(uint32_t table_id, bool is_leaf) {
    pagenum_t new_pagenum = buffer_manager_.get_new_node(table_id);
    
    if (is_leaf) {
        BPTNode<NodeType::LEAF> new_node(table_id, new_pagenum);
        new_node.set_new_node();
        new_node.end_use_node();
    } else {
        BPTNode<NodeType::INTERNAL> new_node(table_id, new_pagenum);
        new_node.set_new_node();
        new_node.end_use_node();
    }

    return new_pagenum;
}

void BPT::start_new_tree(uint32_t table_id, const record_t& record) {
    pagenum_t root_pagenum = make_node(table_id, true);
    BPTNode<NodeType::LEAF> root(table_id, root_pagenum);
    root.increase_num_keys(1);
    root.write_data()[0] = record;
    root.end_use_node();
    update_root_pagenum(table_id, root_pagenum);
}

bool BPT::delete_key(uint32_t table_id, int64_t key) {
    if (!is_db_initialized_) {
        #ifdef DEBUG_BPT
        perror("DB not initialized");
        #endif
        return false;
    }

    if (!is_table_exist(table_id)) {
        #ifdef DEBUG_BPT
        fprintf(stderr, "Table id %d not exist", table_id);
        #endif
        return false;
    }

    record_t key_record;

    if (!find(table_id, key, key_record)) {
        return false;
    }

    BPTNode<NodeType::LEAF> key_leaf;

    if (!find_leaf(table_id, key, key_leaf)) {
        return false;
    }

    delete_leaf_entry(table_id, key_leaf, key);
    return true;
}

void BPT::delete_leaf_entry(uint32_t table_id, BPTNode<NodeType::LEAF>& node, int64_t key) {
    //debug
    /*if (node.page_.header_.spare_page == 1 && node.pagenum_ == root_pagenum_) {
        printf("error\n");
    }*/

    remove_entry_from_leaf(node, key);

    if (get_root(table_id) == node.get_pagenum()) {
        adjust_leaf_root(table_id, node);
        return;
    }

    if (node.num_keys() > 0) {
        node.end_use_node();
        return;
    }

    BPTNode<NodeType::INTERNAL> parent(table_id, node.parent());
    int target_idx = get_target_index(parent, node.get_pagenum());

    coalesce_leaf_nodes(table_id, node, parent, target_idx);
}

void BPT::delete_node_entry(uint32_t table_id, BPTNode<NodeType::INTERNAL>& node, int64_t key) {
    //debug
    /*if (node.page_.header_.spare_page == 1 && node.pagenum_ == root_pagenum_) {
        printf("error\n");
    }*/

    remove_entry_from_node(node, key);

    if (get_root(table_id) == node.get_pagenum()) {
        adjust_node_root(table_id, node);
        return;
    }

    if (node.num_keys() > 0) {
        node.end_use_node();
        return;
    }

    BPTNode<NodeType::INTERNAL> parent(table_id, node.parent());
    int target_idx = get_target_index(parent, node.get_pagenum());

    coalesce_internal_nodes(table_id, node, parent, target_idx);
}

// 나중에 확인 필요
void BPT::remove_entry_from_node(BPTNode<NodeType::INTERNAL>& node, int64_t key) {
    //debug
    /*if (node.pagenum_ == 252 && node.page_.header_.number_of_keys < 10) {
        printf("error\n");
    }*/

    int i;
    bool start_delete = false;
    for (i = 0; i < static_cast<int>(node.num_keys()); i++) {
        if (node.write_data()[i].key_ == key) {
            start_delete = true;
        }
        if (start_delete) {
            if (i == static_cast<int>(node.num_keys()) - 1) {
                memset(&(node.write_data()[i]), 0, sizeof(child_t));
            } else {
                node.write_data()[i] = node.read_data()[i + 1];
            }
        }
    }
    node.decrease_num_keys(1);
}

void BPT::remove_entry_from_leaf(BPTNode<NodeType::LEAF>& node, int64_t key) {
    int i;
    bool start_delete = false;
    for (i = 0; i < static_cast<int>(node.num_keys()); i++) {
        if (node.write_data()[i].key_ == key) {
            start_delete = true;
        }
        if (start_delete) {
            if (i == static_cast<int>(node.num_keys()) - 1) {
                memset(&(node.write_data()[i]), 0, sizeof(record_t));
            } else {
                node.write_data()[i] = node.read_data()[i + 1];
            }
        }
    }
    node.decrease_num_keys(1);
}

void BPT::adjust_leaf_root(uint32_t table_id, BPTNode<NodeType::LEAF>& root) {
    if (root.num_keys() > 0) {
        root.end_use_node();
        return;
    }

    buffer_manager_.delete_node(table_id, get_root(table_id));
    update_root_pagenum(table_id, 0);
    return;
}

void BPT::adjust_node_root(uint32_t table_id, BPTNode<NodeType::INTERNAL>& root) {
    if (root.num_keys() > 0) {
        root.end_use_node();
        return;
    }

    pagenum_t new_root_pagenum = root.spare_page();
    root.end_use_node();

    buffer_manager_.delete_node(table_id, get_root(table_id));
    update_root_pagenum(table_id, new_root_pagenum);

    BPTNode<NodeType::INTERNAL> new_root(table_id, new_root_pagenum);
    new_root.parent(0);
    new_root.end_use_node();
}

void BPT::coalesce_leaf_nodes(uint32_t table_id, BPTNode<NodeType::LEAF>& node, BPTNode<NodeType::INTERNAL>& parent, int target_idx) {
    int64_t k_prime;
    if (target_idx == -1) {
        BPTNode<NodeType::LEAF> right_node(table_id, parent.read_data()[0].child_);
        node.set_page(right_node.get_page());
        node.end_use_node();
        right_node.end_use_node();
        buffer_manager_.delete_node(table_id, right_node.get_pagenum());
        k_prime = parent.read_data()[0].key_;
    } else {
        BPTNode<NodeType::LEAF> left_node;
        if (target_idx == 0) {
            left_node.init_node(table_id, parent.spare_page());
        } else {
            left_node.init_node(table_id, parent.read_data()[target_idx - 1].child_);
        }
        left_node.spare_page(node.spare_page());
        left_node.end_use_node();
        node.end_use_node();
        buffer_manager_.delete_node(table_id, node.get_pagenum());
        k_prime = parent.read_data()[target_idx].key_;
    }
    delete_node_entry(table_id, parent, k_prime);
}

void BPT::coalesce_internal_nodes(uint32_t table_id, BPTNode<NodeType::INTERNAL>& node, BPTNode<NodeType::INTERNAL>& parent, int target_idx) {
    int64_t k_prime;
    if (target_idx == -1) {
        BPTNode<NodeType::INTERNAL> right_node(table_id, parent.read_data()[0].child_);
        uint32_t right_key_number = right_node.num_keys();
        if (right_key_number < INTERNAL_ORDER - 1) {
            for (int i = 1; i < static_cast<int>(right_key_number) + 1; i++) {
                node.write_data()[i] = right_node.read_data()[i - 1];
                // fix debug
                node.increase_num_keys(1);
                // fix parent
                BPTNode<NodeType::INTERNAL> child_temp(table_id, node.read_data()[i].child_);
                child_temp.parent(node.get_pagenum());
                child_temp.end_use_node();
            }
            k_prime = parent.read_data()[0].key_;
            node.write_data()[0] = child_t(k_prime, right_node.spare_page());
            
            // fix parent
            BPTNode<NodeType::INTERNAL> child_temp(table_id, right_node.spare_page());
            child_temp.parent(node.get_pagenum());
            child_temp.end_use_node();

            node.increase_num_keys(1);
            node.end_use_node();
            right_node.end_use_node();
            buffer_manager_.delete_node(table_id, right_node.get_pagenum());
            delete_node_entry(table_id, parent, k_prime);
        } else {
            // redistribution
            redistribute_nodes(table_id, node, right_node, parent, target_idx, true);
        }
    } else {
        BPTNode<NodeType::INTERNAL> left_node;
        if (target_idx == 0) {
            left_node.init_node(table_id, parent.spare_page());
        } else {
            left_node.init_node(table_id, parent.read_data()[target_idx - 1].child_);
        }
        uint32_t left_key_number = left_node.num_keys();
        if (left_key_number < INTERNAL_ORDER - 1) {
            k_prime = parent.read_data()[target_idx].key_;
            left_node.write_data()[left_key_number] = child_t(k_prime, node.spare_page());
            
            // fix parent
            BPTNode<NodeType::INTERNAL> child_temp(table_id, node.spare_page());
            child_temp.parent(left_node.get_pagenum());
            child_temp.end_use_node();

            left_node.increase_num_keys(1);
            left_node.end_use_node();
            node.end_use_node();
            buffer_manager_.delete_node(table_id, node.get_pagenum());
            delete_node_entry(table_id, parent, k_prime);
        } else {
            // redistribution
            redistribute_nodes(table_id, node, left_node, parent, target_idx, false);
        }
    }
}

void BPT::redistribute_nodes(uint32_t table_id, BPTNode<NodeType::INTERNAL>& node, BPTNode<NodeType::INTERNAL>& neighbor, BPTNode<NodeType::INTERNAL>& parent, int target_idx, bool is_leftmost) {
    if (is_leftmost) {
        BPTNode<NodeType::INTERNAL> neighbor_left_child(table_id, neighbor.spare_page());
        child_t neighbor_first_child = neighbor.read_data()[0];

        remove_entry_from_node(neighbor, neighbor.read_data()[0].key_);
        neighbor.spare_page(neighbor_first_child.child_);
        
        int64_t k_prime = parent.read_data()[0].key_;
        parent.write_data()[0].key_ = neighbor_first_child.key_;

        neighbor_left_child.parent(node.get_pagenum());
        node.write_data()[0] = child_t(k_prime, neighbor_left_child.get_pagenum());
        node.num_keys(1);

        node.end_use_node();
        neighbor_left_child.end_use_node();
        neighbor.end_use_node();
        parent.end_use_node();
    } else {
        uint32_t neighbor_key_num = neighbor.num_keys();
        child_t neighbor_last = neighbor.read_data()[neighbor_key_num - 1];
        memset(&(neighbor.write_data()[neighbor_key_num - 1]), 0, sizeof(child_t));
        neighbor.decrease_num_keys(1);

        BPTNode<NodeType::INTERNAL> neighbor_last_node(table_id, neighbor_last.child_);
        neighbor_last_node.parent(node.get_pagenum());

        int64_t k_prime = parent.read_data()[target_idx].key_;
        parent.write_data()[target_idx].key_ = neighbor_last.key_;
        
        node.write_data()[0] = child_t(k_prime, node.spare_page());
        node.spare_page(neighbor_last.child_);
        node.num_keys(1);

        node.end_use_node();
        neighbor.end_use_node();
        neighbor_last_node.end_use_node();
        parent.end_use_node();
    }
}

void BPT::print_tree(uint32_t table_id, std::ostream& out) {
    if (!is_db_initialized_) {
        #ifdef DEBUG_BPT
        perror("DB not initialized");
        #endif
        return;
    }

    if (!is_table_exist(table_id)) {
        #ifdef DEBUG_BPT
        fprintf(stderr, "Table id %d not exist", table_id);
        #endif
        return;
    }
    std::queue<pagenum_t> q;

    if (get_root(table_id) == 0) {
        return;
    }

    std::vector<pagenum_t> left_most;
    BPTNode<NodeType::INTERNAL> temp(table_id, get_root(table_id));
    while (!temp.is_leaf()) {
        left_most.push_back(temp.get_pagenum());
        temp.init_node(table_id, temp.spare_page());
    }
    left_most.push_back(temp.get_pagenum());
    temp.end_use_node();

    q.push(get_root(table_id));

    //debug
    BPTNode<NodeType::INTERNAL> root(table_id, q.front());
    out << "root: " << get_root(table_id) << '\n';
    out << "root left: " << root.spare_page() << '\n';
    root.end_use_node();

    BPTNode<NodeType::INTERNAL> n;
    while (!q.empty()) {
        n.init_node(table_id, q.front());
        q.pop();
        if (std::find(left_most.begin(), left_most.end(), n.get_pagenum()) != left_most.end()) {
            out << '\n';
        }

        if (!n.is_leaf()) {
            q.push(n.spare_page());
        }
        for (int i = 0; i < static_cast<int>(n.num_keys()); i++) {
            if (n.is_leaf()) {
                out << n.records()[i].key_ << " ";
            } else {
                out << n.children()[i].key_ << " ";
                q.push(n.children()[i].child_);
            }
        }
        out << "| ";
    }
    n.end_use_node();
    out << '\n';
}

void BPT::print_leaf(uint32_t table_id, std::ostream& out) {
    if (!is_db_initialized_) {
        #ifdef DEBUG_BPT
        perror("DB not initialized");
        #endif
        return;
    }

    if (!is_table_exist(table_id)) {
        #ifdef DEBUG_BPT
        fprintf(stderr, "Table id %d not exist", table_id);
        #endif
        return;
    }

    BPTNode<NodeType::INTERNAL> temp(table_id, get_root(table_id));
    while (!temp.is_leaf()) {
        temp.init_node(table_id, temp.spare_page());
    }

    while (!(temp.spare_page() == 0)) {
        out << "page: " << temp.get_pagenum() << "\n";
        for (int i = 0; i < static_cast<int>(temp.num_keys()); i++) {
            out << temp.records()[i].key_ << ": " << temp.records()[i].value_ << '\n';
        }
        temp.init_node(table_id, temp.spare_page());
    }
    for (int i = 0; i < static_cast<int>(temp.num_keys()); i++) {
        out << temp.records()[i].key_ << ": " << temp.records()[i].value_ << '\n';
    }
}