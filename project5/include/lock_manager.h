#ifndef __LOCK_MANAGER_H__
#define __LOCK_MANAGER_H__

#include <unordered_map>
#include <cstdint>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "useful.h"
#include "lock.h"

#include "trx_manager.h"

class LockManager {
public:
	static LockManager& instance() {
        static LockManager instance_;
        return instance_;
    }

    TrxFlag lock_acquire(int table_id, int64_t key, int trx_id, LockMode lock_mode);
    int lock_release(lock_t* lock_obj);
    int lock_release_without_lock(lock_t* lock_obj);
    void manager_lock();
    void manager_unlock();

private:
    std::condition_variable condition_var_;
    std::mutex lock_table_latch_;
    std::unordered_map<std::pair<int, int64_t>, lock_list_t> hash_table_;

	bool checkExclusiveCanWake(lock_t* target);
    bool checkSharedCanWake(lock_t* target);
    void findNeedToWait(std::vector<int>& wait_list, lock_t* target);
};

#endif