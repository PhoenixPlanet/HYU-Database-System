#include "bpt.h"

#include <queue>
#include <iostream>

BPT::BPT() : file_manager_(DBFileUtils::instance()), is_open_(false) {    }

uint32_t BPT::cut(uint32_t length) {
    if (length % 2 == 0)
        return length / 2;
    else
        return length / 2 + 1;
}

bool BPT::is_open() {
    return is_open_;
}

uint32_t BPT::open_table(const char* pathname) {
    int table_id = file_manager_.open_db_file(pathname);
    if (table_id < 0) {
        return table_id;
    }
    table_id_ = static_cast<uint32_t>(table_id);

    root_pagenum_ = file_manager_.get_root();

    is_open_ = true;
    return table_id_;
}

uint32_t BPT::set_table(uint32_t table_id) {
    int table_id_return = file_manager_.set_db_file(table_id);
    if (table_id_return == -1) {
        return -1;
    }
    table_id_ = static_cast<uint32_t>(table_id_return);
    root_pagenum_ = file_manager_.get_root();

    is_open_ = true;
    return table_id_;
}

void BPT::update_root_pagenum(pagenum_t new_root) {
    root_pagenum_ = new_root;
    file_manager_.set_root(root_pagenum_);
}

// 찾았을 때 true 트리가 없을 때 false return
bool BPT::find_leaf(int64_t key, bpt_node& leaf) {
    // 루트 페이지 읽어오기
    if (root_pagenum_ == 0) {
        return false;    
    }
    file_manager_.get_node(root_pagenum_, leaf);

    while (!leaf.page_.header_.is_leaf) {
        if (key < leaf.page_.body_.childeren[0].key_) {
            file_manager_.get_node(leaf.page_.header_.spare_page, leaf);
        } else {
            int number_of_keys = static_cast<int>(leaf.page_.header_.number_of_keys);
            int i;
            for (i = 0; i < number_of_keys - 1; i++) {
                if (key < leaf.page_.body_.childeren[i + 1].key_) {
                    break;
                }
            }
            file_manager_.get_node(leaf.page_.body_.childeren[i].child_, leaf);
        }
    }

    return true;
}

// 찾았을 때 true 아닐 때 false return
bool BPT::find(int64_t key, record_t& record) {
    bpt_node leaf;
    if(!find_leaf(key, leaf)) {
        return false;
    }

    int number_of_keys = static_cast<int>(leaf.page_.header_.number_of_keys);
    for (int i = 0; i < number_of_keys; i++) {
        if (leaf.page_.body_.records[i].key_ == key) {
            record = leaf.page_.body_.records[i];
            return true;
        }
    }
    return false;
}

bool BPT::find(int64_t key, char* ret_val) {
    record_t rec;

    bool flag = find(key, rec);
    if (flag) {
        strncpy(ret_val, rec.value_, 119);
        ret_val[119] = '\0';
    }

    return flag;
}

// 찾았을 때 true 아닐 때 false return
bool BPT::is_key_exist(int64_t key) {
    bpt_node leaf;
    if(!find_leaf(key, leaf)) {
        return false;
    }

    int number_of_keys = static_cast<int>(leaf.page_.header_.number_of_keys);
    for (int i = 0; i < number_of_keys; i++) {
        if (leaf.page_.body_.records[i].key_ == key) {
            return true;
        }
    }
    return false;
}

int BPT::get_target_index(bpt_node& parent, pagenum_t target) {
    if (parent.page_.header_.spare_page == target) {
        return -1;
    }

    int left_index = 0;
    for (; left_index <= static_cast<int>(parent.page_.header_.number_of_keys) 
            && parent.page_.body_.childeren[left_index].child_ != target; left_index++);
    return left_index;
}

// 삽입 성공했을 때 true 아닐 때 false return
bool BPT::insert_key(int64_t key, char* value) {
    if (is_key_exist(key)) {
        return false;
    }

    record_t record(key, value);

    if (root_pagenum_ == 0) {
        start_new_tree(record);
        return true;
    }

    bpt_node leaf;
    find_leaf(key, leaf);

    if (leaf.page_.header_.number_of_keys < LEAF_ORDER - 1) {
        insert_into_leaf(leaf, record);
        return true;
    }

    insert_into_leaf_after_splitting(leaf, record);
    return true;
}

void BPT::insert_into_leaf(bpt_node& leaf, const record_t& record) {
    int number_of_keys = static_cast<int>(leaf.page_.header_.number_of_keys);
    int insertion_point = 0;

    for (; insertion_point < number_of_keys && 
           leaf.page_.body_.records[insertion_point].key_ < record.key_;
           insertion_point++);
    
    for (int i = number_of_keys; i > insertion_point; i--) {
        leaf.page_.body_.records[i] = leaf.page_.body_.records[i - 1];
    }
    leaf.page_.body_.records[insertion_point] = record;
    leaf.page_.header_.number_of_keys++;
    file_manager_.write_node(leaf);
}

void BPT::insert_into_leaf_after_splitting(bpt_node& leaf, const record_t& record) {
    pagenum_t new_leaf_pagenum = make_node(true);
    bpt_node new_leaf(new_leaf_pagenum, true);
    int split = static_cast<int>(cut(LEAF_ORDER - 1));
    int insertion_point = 0;

    for (; insertion_point < static_cast<int>(LEAF_ORDER) - 1 && 
           leaf.page_.body_.records[insertion_point].key_ < record.key_;
           insertion_point++);
    
    bool inserted_left = false;
    record_t temp_record;
    if (insertion_point < split) {
        inserted_left = true;
        temp_record = leaf.page_.body_.records[split - 1];
        for (int i = split - 1; i > insertion_point; i--) {
            leaf.page_.body_.records[i] = leaf.page_.body_.records[i - 1];
        }
        leaf.page_.body_.records[insertion_point] = record;
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
        new_leaf.page_.body_.records[j] = leaf.page_.body_.records[i];
    }

    if (inserted_left) {
        new_leaf.page_.body_.records[insertion_point] = temp_record;
    } else {
        new_leaf.page_.body_.records[insertion_point] = record;
    }

    leaf.page_.header_.number_of_keys = split;
    new_leaf.page_.header_.number_of_keys = LEAF_ORDER - split;

    new_leaf.page_.header_.spare_page = leaf.page_.header_.spare_page;
    leaf.page_.header_.spare_page = new_leaf.pagenum_;

    new_leaf.page_.header_.parent = leaf.page_.header_.parent;

    for (uint32_t i = leaf.page_.header_.number_of_keys; i < LEAF_ORDER - 1; i++) {
        memset(&(leaf.page_.body_.records[i]), 0, sizeof(record_t));
    }
    
    for (uint32_t i = new_leaf.page_.header_.number_of_keys; i < LEAF_ORDER - 1; i++) {
        memset(&(new_leaf.page_.body_.records[i]), 0, sizeof(record_t));
    }

    leaf.write_to_file();
    new_leaf.write_to_file();

    int64_t new_key = new_leaf.page_.body_.records[0].key_;
    insert_into_parent(leaf.page_.header_.parent, leaf.pagenum_, new_leaf.pagenum_, new_key);
}

void BPT::insert_into_parent(pagenum_t parent, pagenum_t left, pagenum_t right, int64_t key) {
    if (parent == 0) {
        insert_into_new_root(left, right, key);
        return;
    }

    bpt_node parent_node(parent, true);
    int left_index = get_target_index(parent_node, left);

    if (parent_node.page_.header_.number_of_keys < INTERNAL_ORDER - 1) {
        insert_into_node(parent_node, left_index, key, right);
        return;
    }

    insert_into_node_after_splitting(parent_node, left_index, key, right);
}

void BPT::insert_into_new_root(pagenum_t left, pagenum_t right, int64_t key) {
    pagenum_t root_pagenum = make_node(false);
    bpt_node root(root_pagenum, true);
    root.page_.header_.spare_page = left;
    child_t child(key, right);
    root.page_.body_.childeren[0] = child;
    root.page_.header_.number_of_keys++;
    
    bpt_node left_node(left, true);
    bpt_node right_node(right, true);

    left_node.page_.header_.parent = root_pagenum;
    right_node.page_.header_.parent = root_pagenum;

    root.write_to_file();
    left_node.write_to_file();
    right_node.write_to_file();

    update_root_pagenum(root_pagenum);
}

void BPT::insert_into_node(bpt_node& parent, int left_index, int64_t key, pagenum_t right) {
    //debug
    /*if (parent.pagenum_ == 3915) {
        printf("error");
    }*/
    int number_of_keys = static_cast<int>(parent.page_.header_.number_of_keys);

    for (int i = number_of_keys; i > left_index + 1; i--) {
        parent.page_.body_.childeren[i] = parent.page_.body_.childeren[i - 1];
    }
    child_t right_child(key, right);
    parent.page_.body_.childeren[left_index + 1] = right_child; 
    parent.page_.header_.number_of_keys++;
    parent.write_to_file();
}

void BPT::insert_into_node_after_splitting(bpt_node& parent, int left_index, int64_t key, pagenum_t right) {
    pagenum_t new_parent_pagenum = make_node(false);
    //debug
    /*if (new_parent_pagenum == 3915 || parent.pagenum_ == 3915) {
        printf("error");
    }*/
    
    bpt_node new_parent(new_parent_pagenum, true);
    int split = static_cast<int>(cut(INTERNAL_ORDER - 1));
    int insertion_point = left_index + 1;

    child_t right_child(key, right);
    bool inserted_left = false;
    child_t temp_child;
    if (insertion_point < split) {
        inserted_left = true;
        temp_child = parent.page_.body_.childeren[split - 1];
        for (int i = split - 1; i > insertion_point; i--) {
            parent.page_.body_.childeren[i] = parent.page_.body_.childeren[i - 1];
        }
        parent.page_.body_.childeren[insertion_point] = right_child;
    }

    int start = split;
    if (!inserted_left) {
        insertion_point -= split + 1;
        if (insertion_point < 0) {
            temp_child = right_child;
        } else {
            temp_child = parent.page_.body_.childeren[split];
            start = split + 1;
        }
    }

    for (int i = start, j = 0; i < static_cast<int>(INTERNAL_ORDER) - 1; i++, j++) {
        if (j == insertion_point && !inserted_left) {
            new_parent.page_.body_.childeren[j] = right_child;
            bpt_node right_child_node(right_child.child_, true);
            right_child_node.page_.header_.parent = new_parent.pagenum_;
            right_child_node.write_to_file();
            j++;
        }
        new_parent.page_.body_.childeren[j] = parent.page_.body_.childeren[i];
        bpt_node child_temp(parent.page_.body_.childeren[i].child_, true);
        child_temp.page_.header_.parent = new_parent.pagenum_;
        child_temp.write_to_file();
    }
    if (static_cast<int>(INTERNAL_ORDER - split - 2) == insertion_point && !inserted_left) {
        new_parent.page_.body_.childeren[insertion_point] = right_child;
        bpt_node right_child_node(right_child.child_, true);
        right_child_node.page_.header_.parent = new_parent.pagenum_;
        right_child_node.write_to_file();
    }

    parent.page_.header_.number_of_keys = split;
    new_parent.page_.header_.number_of_keys = INTERNAL_ORDER - split - 1;

    new_parent.page_.header_.spare_page = temp_child.child_;
    bpt_node spare_child_node(temp_child.child_, true);
    spare_child_node.page_.header_.parent = new_parent.pagenum_;
    spare_child_node.write_to_file();
    
    new_parent.page_.header_.parent = parent.page_.header_.parent;

    for (uint32_t i = parent.page_.header_.number_of_keys; i < INTERNAL_ORDER - 1; i++) {
        memset(&(parent.page_.body_.childeren[i]), 0, sizeof(child_t));
    }

    for (uint32_t i = new_parent.page_.header_.number_of_keys; i < INTERNAL_ORDER - 1; i++) {
        memset(&(new_parent.page_.body_.childeren[i]), 0, sizeof(child_t));
    }

    parent.write_to_file();
    new_parent.write_to_file();

    insert_into_parent(parent.page_.header_.parent, parent.pagenum_, new_parent.pagenum_, temp_child.key_);
}

pagenum_t BPT::make_node(bool is_leaf) {
    pagenum_t new_pagenum = file_manager_.get_new_node();
    bpt_node new_node(new_pagenum, true);
    new_node.page_.header_.is_leaf = is_leaf;
    new_node.page_.header_.number_of_keys = 0;
    new_node.page_.header_.parent = 0;
    new_node.page_.header_.spare_page = 0;
    new_node.write_to_file();
    return new_pagenum;
}

void BPT::start_new_tree(const record_t& record) {
    pagenum_t root_pagenum = make_node(true);
    bpt_node root(root_pagenum, true);
    root.page_.header_.number_of_keys++;
    root.page_.body_.records[0] = record;
    root.write_to_file();
    update_root_pagenum(root_pagenum);
}

bool BPT::delete_key(int64_t key) {
    record_t key_record;
    bpt_node key_leaf;

    if (!find_leaf(key, key_leaf) || !find(key, key_record)) {
        return false;
    }

    delete_entry(key_leaf, key);
    return true;
}

void BPT::delete_entry(bpt_node& node, int64_t key) {
    //debug
    /*if (node.page_.header_.spare_page == 1 && node.pagenum_ == root_pagenum_) {
        printf("error\n");
    }*/

    if (node.page_.header_.is_leaf) {
        remove_entry_from_leaf(node, key);
    } else {
        remove_entry_from_node(node, key);
    }

    if (root_pagenum_ == node.pagenum_) {
        adjust_root(node);
        return;
    }

    if (node.page_.header_.number_of_keys > 0) {
        node.write_to_file();
        return;
    }

    bpt_node parent(node.page_.header_.parent, true);
    int target_idx = get_target_index(parent, node.pagenum_);

    coalesce_nodes(node, parent, target_idx);
}

// 나중에 확인 필요
void BPT::remove_entry_from_node(bpt_node& node, int64_t key) {
    //debug
    /*if (node.pagenum_ == 252 && node.page_.header_.number_of_keys < 10) {
        printf("error\n");
    }*/

    int i;
    bool start_delete = false;
    for (i = 0; i < static_cast<int>(node.page_.header_.number_of_keys); i++) {
        if (node.page_.body_.childeren[i].key_ == key) {
            start_delete = true;
        }
        if (start_delete) {
            if (i == static_cast<int>(node.page_.header_.number_of_keys) - 1) {
                memset(&(node.page_.body_.childeren[i]), 0, sizeof(child_t));
            } else {
                node.page_.body_.childeren[i] = node.page_.body_.childeren[i + 1];
            }
        }
    }
    node.page_.header_.number_of_keys--;
}

void BPT::remove_entry_from_leaf(bpt_node& node, int64_t key) {
    int i;
    bool start_delete = false;
    for (i = 0; i < static_cast<int>(node.page_.header_.number_of_keys); i++) {
        if (node.page_.body_.records[i].key_ == key) {
            start_delete = true;
        }
        if (start_delete) {
            if (i == static_cast<int>(node.page_.header_.number_of_keys) - 1) {
                memset(&(node.page_.body_.records[i]), 0, sizeof(record_t));
            } else {
                node.page_.body_.records[i] = node.page_.body_.records[i + 1];
            }
        }
    }
    node.page_.header_.number_of_keys--;
}

void BPT::adjust_root(bpt_node& root) {
    if (root.page_.header_.number_of_keys > 0) {
        root.write_to_file();
        return;
    }

    if (root.page_.header_.is_leaf) {
        file_manager_.delete_node(root_pagenum_);
        update_root_pagenum(0);
        return;
    }

    //debug
    /*if (root.page_.header_.spare_page == 1) {
        printf("error\n");
    }*/

    file_manager_.delete_node(root_pagenum_);
    update_root_pagenum(root.page_.header_.spare_page);
    bpt_node new_root(root.page_.header_.spare_page, true);
    new_root.page_.header_.parent = 0;
    new_root.write_to_file();
}


// 다시 짜자...
void BPT::coalesce_nodes(bpt_node& node, bpt_node& parent, int target_idx) {
    if (node.page_.header_.is_leaf) {
        int64_t k_prime;
        if (target_idx == -1) {
            bpt_node right_node(parent.page_.body_.childeren[0].child_, true);
            node.page_ = right_node.page_;
            node.write_to_file();
            file_manager_.delete_node(right_node.pagenum_);
            k_prime = parent.page_.body_.childeren[0].key_;
        } else {
            bpt_node left_node;
            if (target_idx == 0) {
                file_manager_.get_node(parent.page_.header_.spare_page, left_node);
            } else {
                file_manager_.get_node(parent.page_.body_.childeren[target_idx - 1].child_, left_node);
            }
            left_node.page_.header_.spare_page = node.page_.header_.spare_page;
            left_node.write_to_file();
            file_manager_.delete_node(node.pagenum_);
            k_prime = parent.page_.body_.childeren[target_idx].key_;
        }
        delete_entry(parent, k_prime);
    } else {
        int64_t k_prime;
        if (target_idx == -1) {
            bpt_node right_node(parent.page_.body_.childeren[0].child_, true);
            uint32_t right_key_number = right_node.page_.header_.number_of_keys;
            if (right_key_number < INTERNAL_ORDER - 1) {
                for (int i = 1; i < static_cast<int>(right_key_number) + 1; i++) {
                    node.page_.body_.childeren[i] = right_node.page_.body_.childeren[i - 1];
                    // fix debug
                    node.page_.header_.number_of_keys++;
                    // fix parent
                    bpt_node child_temp(node.page_.body_.childeren[i].child_, true);
                    child_temp.page_.header_.parent = node.pagenum_;
                    child_temp.write_to_file();
                }
                k_prime = parent.page_.body_.childeren[0].key_;
                node.page_.body_.childeren[0] = child_t(k_prime, right_node.page_.header_.spare_page);
                
                // fix parent
                bpt_node child_temp(right_node.page_.header_.spare_page, true);
                child_temp.page_.header_.parent = node.pagenum_;
                child_temp.write_to_file();

                node.page_.header_.number_of_keys++;
                node.write_to_file();
                file_manager_.delete_node(right_node.pagenum_);
                delete_entry(parent, k_prime);
            } else {
                // redistribution
                redistribute_nodes(node, right_node, parent, target_idx, true);
            }
        } else {
            bpt_node left_node;
            if (target_idx == 0) {
                file_manager_.get_node(parent.page_.header_.spare_page, left_node);
            } else {
                file_manager_.get_node(parent.page_.body_.childeren[target_idx - 1].child_, left_node);
            }
            uint32_t left_key_number = left_node.page_.header_.number_of_keys;
            if (left_key_number < INTERNAL_ORDER - 1) {
                k_prime = parent.page_.body_.childeren[target_idx].key_;
                left_node.page_.body_.childeren[left_key_number] = child_t(k_prime, node.page_.header_.spare_page);
                
                // fix parent
                bpt_node child_temp(node.page_.header_.spare_page, true);
                child_temp.page_.header_.parent = left_node.pagenum_;
                child_temp.write_to_file();

                left_node.page_.header_.number_of_keys++;
                left_node.write_to_file();
                file_manager_.delete_node(node.pagenum_);
                delete_entry(parent, k_prime);
            } else {
                // redistribution
                redistribute_nodes(node, left_node, parent, target_idx, false);
            }
        }
    }
}

void BPT::redistribute_nodes(bpt_node& node, bpt_node& neighbor, bpt_node& parent, int target_idx, bool is_leftmost) {
    if (is_leftmost) {
        bpt_node neighbor_left_child(neighbor.page_.header_.spare_page, true);
        child_t neighbor_first_child = neighbor.page_.body_.childeren[0];

        remove_entry_from_node(neighbor, neighbor.page_.body_.childeren[0].key_);
        neighbor.page_.header_.spare_page = neighbor_first_child.child_;
        
        int64_t k_prime = parent.page_.body_.childeren[0].key_;
        parent.page_.body_.childeren[0].key_ = neighbor_first_child.key_;

        neighbor_left_child.page_.header_.parent = node.pagenum_;
        node.page_.body_.childeren[0] = child_t(k_prime, neighbor_left_child.pagenum_);
        node.page_.header_.number_of_keys = 1;

        node.write_to_file();
        neighbor_left_child.write_to_file();
        neighbor.write_to_file();
        parent.write_to_file();
    } else {
        uint32_t neighbor_key_num = neighbor.page_.header_.number_of_keys;
        child_t neighbor_last = neighbor.page_.body_.childeren[neighbor_key_num - 1];
        memset(&(neighbor.page_.body_.childeren[neighbor_key_num - 1]), 0, sizeof(child_t));
        neighbor.page_.header_.number_of_keys--;

        bpt_node neighbor_last_node(neighbor_last.child_, true);
        neighbor_last_node.page_.header_.parent = node.pagenum_;

        int64_t k_prime = parent.page_.body_.childeren[target_idx].key_;
        parent.page_.body_.childeren[target_idx].key_ = neighbor_last.key_;
        
        node.page_.body_.childeren[0] = child_t(k_prime, node.page_.header_.spare_page);
        node.page_.header_.spare_page = neighbor_last.child_;
        node.page_.header_.number_of_keys = 1;

        node.write_to_file();
        neighbor.write_to_file();
        neighbor_last_node.write_to_file();
        parent.write_to_file();
    }
}

void BPT::print_tree(std::ostream& out) {
    std::queue<bpt_node> q;

    if (root_pagenum_ == 0) {
        return;
    }

    //debug
    bpt_node root(root_pagenum_, true);
    out << "root: " << root_pagenum_ << '\n';
    out << "root left: " << root.page_.header_.spare_page << '\n';
    
    q.push(bpt_node(root_pagenum_, true));

    std::vector<pagenum_t> left_most;
    bpt_node temp(root_pagenum_, true);
    while (!temp.page_.header_.is_leaf) {
        left_most.push_back(temp.pagenum_);
        temp = bpt_node(temp.page_.header_.spare_page, true);
    }
    left_most.push_back(temp.pagenum_);

    while (!q.empty()) {
        bpt_node n = q.front();
        q.pop(); 
        if (std::find(left_most.begin(), left_most.end(), n.pagenum_) != left_most.end()) {
            out << '\n';
        }

        if (!n.page_.header_.is_leaf) {
            q.push(bpt_node(n.page_.header_.spare_page, true));
        }
        for (int i = 0; i < static_cast<int>(n.page_.header_.number_of_keys); i++) {
            if (n.page_.header_.is_leaf) {
                out << n.page_.body_.records[i].key_ << " ";
            } else {
                out << n.page_.body_.childeren[i].key_ << " ";
                q.push(bpt_node(n.page_.body_.childeren[i].child_, true));
            }
        }
        out << "| ";
    }
    out << '\n';
}

void BPT::print_leaf(std::ostream& out) {
    bpt_node temp(root_pagenum_, true);
    while (!temp.page_.header_.is_leaf) {
        temp = bpt_node(temp.page_.header_.spare_page, true);
    }

    while (!(temp.page_.header_.spare_page == 0)) {
        out << "page: " << temp.pagenum_ << "\n";
        for (int i = 0; i < static_cast<int>(temp.page_.header_.number_of_keys); i++) {
            out << temp.page_.body_.records[i].key_ << ": " << temp.page_.body_.records[i].value_ << '\n';
        }
        temp = bpt_node(temp.page_.header_.spare_page, true);
    }
    for (int i = 0; i < static_cast<int>(temp.page_.header_.number_of_keys); i++) {
        out << temp.page_.body_.records[i].key_ << ": " << temp.page_.body_.records[i].value_ << '\n';
    }
}