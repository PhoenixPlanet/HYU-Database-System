#ifndef __LOCK_H__
#define __LOCK_H__

#include <condition_variable>
#include <cstdint>

enum class LockMode {
    SHARED = 0,
    EXCLUSIVE = 1
};

enum class TrxFlag {
	ABORT,
	SUCCESS,
	UNKNOWN_FAIL
};

struct lock_list_t;

struct lock_t {
	lock_t* prev_;
	lock_t* next_;
	lock_list_t* sentinel_;
	bool can_wake_;
    bool is_wake_;
    LockMode lock_mode_;
    lock_t* next_trx_lock_;
    lock_t* prev_trx_lock_;
    int trx_id_;

	lock_t(lock_t* prev, lock_t* next, lock_list_t* sentinel, LockMode lock_mode, int trx_id) : prev_(prev), 
																next_(next), 
																sentinel_(sentinel),
																can_wake_(false),
                                                                is_wake_(false),
                                                                lock_mode_(lock_mode),
                                                                next_trx_lock_(nullptr),
                                                                trx_id_(trx_id) {	}
};

struct lock_list_t
{
	int table_id_;
	int64_t record_id_;
	lock_t* tail_;
	lock_t* head_;
	std::unique_ptr<std::condition_variable> cv_;

	lock_list_t(int table_id, int64_t record_id) : table_id_(table_id), 
												   record_id_(record_id),
												   tail_(nullptr),
												   head_(nullptr),
												   cv_(new std::condition_variable()) {	};
	lock_list_t() : tail_(nullptr),
					head_(nullptr) {	} 
};

#endif