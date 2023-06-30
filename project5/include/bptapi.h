#ifndef __BPT_API_H__
#define __BPT_API_H__

#define DEBUG_BPT

#include "bpt.h"

/*#ifdef __cplusplus
extern "C" {
#endif*/

int init_db(int num_buf);

int close_table(int table_id);

int shutdown_db();

int open_table(char* pathname);

int db_insert(int table_id, int64_t key, char* value);

int db_find(int table_id, int64_t key, char* ret_val, int trx_id);

int db_update(int table_id, int64_t key, char* values, int trx_id);

int db_delete(int table_id, int64_t key);

int trx_begin(void);

int trx_commit(int trx_id);

/*#ifdef __cplusplus
}
#endif*/

#ifdef DEBUG_BPT
void db_print_tree(int table_id, std::ostream& out);
void db_print_leaf(int table_id, std::ostream& out);
#endif

#endif