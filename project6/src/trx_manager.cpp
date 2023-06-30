#include <cstring>

#include "trx_manager.h"
#include "lock_manager.h"

#include "bpt_node.h"

TransactionManager::TransactionManager() : global_trx_id_(0) {  }

bool TransactionManager::isTrxExistWithLock(int trx_id) {
    std::unique_lock<std::mutex> trx_lock(trx_table_lock_);
    if (transaction_table_.find(trx_id) == transaction_table_.end()) {
        return false;
    } else {
        return true;
    }
}

bool TransactionManager::isTrxExist(int trx_id) {
    if (transaction_table_.find(trx_id) == transaction_table_.end()) {
        return false;
    } else {
        return true;
    }
}

void TransactionManager::setPrevLSN(int trx_id, int64_t prev_lsn) {
    std::unique_lock<std::mutex> trx_lock(trx_table_lock_);
    transaction_table_[trx_id].prev_lsn_ = prev_lsn;
}

int64_t TransactionManager::getPrevLSN(int trx_id) {
    std::unique_lock<std::mutex> trx_lock(trx_table_lock_);
    return transaction_table_[trx_id].prev_lsn_;
}

int TransactionManager::startNewTrx() {
    std::unique_lock<std::mutex> trx_lock(trx_table_lock_);
    
    int new_id = ++global_trx_id_;
    
    transaction_table_.emplace(new_id, trx_t(new_id));
    
    wait_for_graph_.emplace(new_id, std::unordered_map<int, bool>());
    for (auto& i : transaction_table_) {
        wait_for_graph_[new_id][i.first] = false;
    }
    for (auto& i : wait_for_graph_) {
        i.second[new_id] = false;
    }

    visited_[new_id] = false;
    finished_[new_id] = false;

    CommonLog log;
    log.prev_lsn_ = -1;
    log.trx_id_ = new_id;
    log.type_ = LogType::BEGIN;

    transaction_table_[new_id].prev_lsn_ = LogManager::instance().addCommonLog(log);

    return new_id;
}

void TransactionManager::addLog(int trx_id, TrxLog log) {
    std::unique_lock<std::mutex> trx_lock(trx_table_lock_);
    transaction_table_[trx_id].log_.push_back(log);
}

// dfs cycle 찾기 알고리즘을 참고하였습니다.
void TransactionManager::findCycle(int trx_id) {
    visited_[trx_id] = true;
    for (auto& i : transaction_table_) {
        if (wait_for_graph_[trx_id][i.first]) {
            if (!(visited_[i.first])) {
                findCycle(i.first);
            } else if (!(finished_[i.first])) {
                has_cycle_ = true;
            }
        }
    }
    finished_[trx_id] = true;
}

bool TransactionManager::isDeadlock(int trx_id) {
    has_cycle_ = false;
    
    for (auto& i : visited_) {
        i.second = false;
    }
    for (auto& i : finished_) {
        i.second = false;
    }

    findCycle(trx_id);

    return has_cycle_;
}

bool TransactionManager::addLockToTrxList(int trx_id, std::vector<int>& wait_list, lock_t* target) {
    std::unique_lock<std::mutex> trx_lock(trx_table_lock_);

    if (!isTrxExist(trx_id)) {
        trx_lock.unlock();
        return false;
    }

    lock_t*& tail = transaction_table_[trx_id].tail_;
    lock_t*& head = transaction_table_[trx_id].head_;

    if (tail == nullptr) {
        head = target;
        tail = target;
        target->next_trx_lock_ = nullptr;
        target->prev_trx_lock_ = nullptr;
    } else {
        target->next_trx_lock_ = nullptr;
        target->prev_trx_lock_ = tail;
        tail->next_trx_lock_ = target;
        tail = target;
    }

    if (wait_list.size() != 0) {
        for (int i : wait_list) {
            wait_for_graph_[trx_id][i] = true;
        }

        if (isDeadlock(trx_id)) {
            rollback(trx_id);
            endTrx(trx_id, false);

            return false;
        }
    }

    return true;
}

bool TransactionManager::endTrx(int trx_id, bool with_lock) {
    if (!isTrxExist(trx_id)) {
        return false;
    }

    releaseAllLock(transaction_table_[trx_id], with_lock);

    for (auto& i : wait_for_graph_) {
        i.second.erase(trx_id);
    }
    wait_for_graph_.erase(trx_id);
    visited_.erase(trx_id);
    finished_.erase(trx_id);

    transaction_table_.erase(trx_id);

    return true;
}

int TransactionManager::commitTrx(int trx_id) {
    LockManager::instance().manager_lock();
    std::unique_lock<std::mutex> trx_lock(trx_table_lock_);

    if (!isTrxExist(trx_id)) {
        LockManager::instance().manager_unlock();
        return 0;
    } else {
        int64_t last_lsn = transaction_table_[trx_id].prev_lsn_;
        endTrx(trx_id, false);
        LockManager::instance().manager_unlock();

        CommonLog log;
        log.prev_lsn_ = last_lsn;
        log.trx_id_ = trx_id;
        log.type_ = LogType::COMMIT;

        LogManager::instance().addCommonLog(log);
        LogManager::instance().force();

        return trx_id;
    }
}

void TransactionManager::rollback(int trx_id) {
    std::vector<TrxLog> log(transaction_table_[trx_id].log_.rbegin(), transaction_table_[trx_id].log_.rend());
    for (auto& i : log) {
        if (i.log_type_ == LogType::UPDATE) {
            BPTNode<NodeType::LEAF> target(i.table_, i.page_);
            int number_of_keys = static_cast<int>(target.num_keys());
            for (int j = 0; j < number_of_keys; j++) {
                if (target.read_data()[j].key_ == i.key_) {
                    strcpy(target.write_data()[j].value_, i.before_.c_str());

                    // 로그 발급
                    CommonLog real_log;
                    real_log.prev_lsn_ = transaction_table_[trx_id].prev_lsn_;
                    real_log.trx_id_ = trx_id;
                    real_log.type_ = LogType::COMPENSATE;
                    
                    UpdateData data;
                    data.table_id_ = i.table_;
                    data.pagenum_ = i.page_;
                    data.offset_ = sizeof(page_t::page::node_page::header) + (sizeof(child_t) * j);
                    data.data_length_ = 120;
                    strcpy(data.old_image_, i.after_.c_str());
                    strcpy(data.new_image_, i.before_.c_str());

                    int64_t target_lsn = LogManager::instance().addUpdateLog(real_log, data);
                    target.lsn(target_lsn);

                    transaction_table_[trx_id].prev_lsn_ = target_lsn;

                    target.end_use_node();
                    break;
                }
            }
        }
    }

    CommonLog real_log;
    real_log.prev_lsn_ = transaction_table_[trx_id].prev_lsn_;
    real_log.trx_id_ = trx_id;
    real_log.type_ = LogType::ROLLBACK;

    LogManager::instance().addCommonLog(real_log);
    LogManager::instance().force();
}

bool TransactionManager::abortTrx(int trx_id) {
    LockManager::instance().manager_lock();
    std::unique_lock<std::mutex> trx_lock(trx_table_lock_);

    rollback(trx_id);
    bool flag = endTrx(trx_id, false);
    LockManager::instance().manager_unlock();
    return flag;
}

bool TransactionManager::releaseAllLock(trx_t& trx, bool with_lock) {
    lock_t* next;
    for (lock_t* target = trx.head_; target != nullptr;) {
        next = target->next_trx_lock_;
        if (with_lock) {
            LockManager::instance().lock_release(target);
        } else {
            LockManager::instance().lock_release_without_lock(target);
        }
        
        target = next;
    }
    return true;
}