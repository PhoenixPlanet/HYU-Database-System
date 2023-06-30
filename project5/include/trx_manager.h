#ifndef __TRX_MANAGER_H__
#define __TRX_MANAGER_H__

#include <unordered_map>
#include <cstdint>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

#include "lock.h"
#include "page.h"

class LockManager;

enum class LogType {
    READ,
    UPDATE
};

struct TrxLog {
    LogType log_type_;
    uint32_t table_;
    pagenum_t page_;
    int64_t key_;
    std::string before_;
    std::string after_;

    TrxLog(LogType log_type, uint32_t table_id, pagenum_t pagenum, int64_t key, std::string before, std::string after)
        : log_type_(log_type),
          table_(table_id),
          page_(pagenum),
          key_(key),
          before_(before),
          after_(after) {   }

    TrxLog(LogType log_type, uint32_t table_id, pagenum_t pagenum, int64_t key)
        : log_type_(log_type),
          table_(table_id),
          page_(pagenum),
          key_(key) {   }
};

struct trx_t {
    int trx_id_;

    lock_t* head_;
    lock_t* tail_;

    std::vector<TrxLog> log_;

    trx_t(int trx_id) : trx_id_(trx_id),
                        head_(nullptr),
                        tail_(nullptr) {   }

    trx_t() {   }
};

class TransactionManager {
public:
    static TransactionManager& instance() {
        static TransactionManager instance_;
        return instance_;
    }

    bool addLockToTrxList(int trx_id, std::vector<int>& wait_list, lock_t* target);
    int startNewTrx();
    int commitTrx(int trx_id);
    bool abortTrx(int trx_id);
    bool isTrxExist(int trx_id);
    bool isTrxExistWithLock(int trx_id);
    void addLog(int trx_id, TrxLog log);

private:
    TransactionManager();
    bool endTrx(int trx_id, bool with_lock);
    bool releaseAllLock(trx_t& trx, bool with_lock);
    void rollback(int trx_id);
    bool isDeadlock(int trx_id);
    void findCycle(int trx_id);

    std::unordered_map<int, std::unordered_map<int, bool>> wait_for_graph_;
    std::unordered_map<int, bool> visited_;
    std::unordered_map<int, bool> finished_;
    bool has_cycle_;

    int global_trx_id_;
    std::unordered_map<int, trx_t> transaction_table_;
    std::mutex trx_table_lock_;
};

#endif