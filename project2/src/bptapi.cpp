#include "bptapi.h"

BPT bpt = BPT();

int open_table(char* pathname) {
    return bpt.open_table(pathname);
}

int db_insert(int64_t key, char* value) {
    if (bpt.is_open()) {
        if (bpt.insert_key(key, value)) {
            return 0;
        } else {
            return -1;
        }
    } else {
        return -2;
    }
}

int db_find(int64_t key, char* ret_val) {
    if (bpt.is_open()) {
        if (bpt.find(key, ret_val)) {
            return 0;
        } else {
            return -1;
        }
    } else {
        return -2;
    }
}

int db_delete(int64_t key) {
    if (bpt.is_open()) {
        if (bpt.delete_key(key)) {
            return 0;
        } else {
            return -1;
        }
    } else {
        return -2;
    }
}

#ifdef DEBUG_BPT
void db_print_tree(std::ostream& out) {
    if (bpt.is_open()) {
        bpt.print_tree(out);
    }
}

void db_print_leaf(std::ostream& out) {
    if (bpt.is_open()) {
        bpt.print_leaf(out);
    }
}

int set_db_file(uint32_t table_id) {
    int table_return;
    if ((table_return = bpt.set_table(table_id)) >= 0) {
        return table_return;
    } else {
        return -1;
    }
}
#endif