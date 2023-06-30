#ifndef __BPT_API_H__
#define __BPT_API_H__

#define DEBUG_BPT

#include "bpt.h"

extern "C" {
    
int open_table(char* pathname);

int db_insert(int64_t key, char* value);

int db_find(int64_t key, char* ret_val);

int db_delete(int64_t key);

}

#ifdef DEBUG_BPT
void db_print_tree(std::ostream& out);
void db_print_leaf(std::ostream& out);
int set_db_file(uint32_t table_id);
#endif

#endif