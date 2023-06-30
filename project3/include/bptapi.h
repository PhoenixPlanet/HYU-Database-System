#ifndef __BPT_API_H__
#define __BPT_API_H__

#define DEBUG_BPT

#include "bpt.h"

extern "C" {

int init_db(int num_buf);

int close_table(int table_id);

int shutdown_db();

int open_table(char* pathname);

int db_insert(int table_id, int64_t key, char* value);

int db_find(int table_id, int64_t key, char* ret_val);

int db_delete(int table_id, int64_t key);

}

#ifdef DEBUG_BPT
void db_print_tree(int table_id, std::ostream& out);
void db_print_leaf(int table_id, std::ostream& out);
#endif

#endif