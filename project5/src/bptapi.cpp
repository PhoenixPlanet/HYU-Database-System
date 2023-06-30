#include "bptapi.h"

BPT bpt = BPT();

int init_db(int num_buf) {
    if (bpt.init_db(num_buf)) {
        return 0;
    } else {
        return -1;
    }
}

int trx_begin(void) {
    return TransactionManager::instance().startNewTrx();
}

int trx_commit(int trx_id) {
    return TransactionManager::instance().commitTrx(trx_id);
}

int shutdown_db() {
    if (bpt.shutdown_db()) {
        return 0;
    } else {
        return -1;
    }
}

int open_table(char* pathname) {
    return bpt.open_table(pathname);
}

int close_table(int table_id) {
    if (bpt.close_table(table_id)) {
        return 0;
    } else {
        return -1;
    }
}

int db_insert(int table_id, int64_t key, char* value) {
    if (bpt.insert_key(static_cast<uint32_t>(table_id), key, value)) {
        return 0;
    } else {
        return -1;
    }
}

int db_find(int table_id, int64_t key, char* ret_val, int trx_id) {
    return (bpt.find(static_cast<uint32_t>(table_id), key, ret_val, trx_id));
}

int db_update(int table_id, int64_t key, char* values, int trx_id) {
    return (bpt.update(static_cast<uint32_t>(table_id), key, values, trx_id));
}

int db_delete(int table_id, int64_t key) {
    if (bpt.delete_key(static_cast<uint32_t>(table_id), key)) {
        return 0;
    } else {
        return -1;
    }
}

#ifdef DEBUG_BPT
void db_print_tree(int table_id, std::ostream& out) {
    bpt.print_tree(static_cast<uint32_t>(table_id), out);
}

void db_print_leaf(int table_id, std::ostream& out) {
    bpt.print_leaf(static_cast<uint32_t>(table_id), out);
}
#endif