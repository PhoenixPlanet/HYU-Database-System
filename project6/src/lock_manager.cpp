#include "lock_manager.h"

// 앞에 자신과 같은 trx id를 가진 shared lock만 있을 때 true 반환
bool LockManager::checkExclusiveCanWake(lock_t* target) {
	for (lock_t* temp_lock = target->prev_; temp_lock != nullptr; temp_lock = temp_lock->prev_) {
		if (temp_lock->trx_id_ != target->trx_id_ || temp_lock->lock_mode_ == LockMode::EXCLUSIVE) {
			return false;
		}
	}
	return true;
}

// 앞에 s락만 있을 때 true 반환
bool LockManager::checkSharedCanWake(lock_t* target) {
	for (lock_t* temp_lock = target->prev_; temp_lock != nullptr; temp_lock = temp_lock->prev_) {
		if (temp_lock->lock_mode_ == LockMode::EXCLUSIVE) {
			return false;
		}
	}
	return true;
}

void LockManager::manager_lock() {
	lock_table_latch_.lock();
}

void LockManager::manager_unlock() {
	lock_table_latch_.unlock();
}

void LockManager::findNeedToWait(std::vector<int>& wait_list, lock_t* target) {
	if (target->lock_mode_ == LockMode::SHARED) {
		for (lock_t* temp_lock = target->prev_; temp_lock != nullptr; temp_lock = temp_lock->prev_) {
			if (temp_lock->lock_mode_ == LockMode::EXCLUSIVE) {
				wait_list.push_back(temp_lock->trx_id_);
				return;
			}
		}
	} else {
		lock_t* temp_lock = target->prev_;
		if (temp_lock != nullptr && temp_lock->trx_id_ == target->trx_id_) {
			temp_lock = temp_lock->prev_;
		}

		if (temp_lock != nullptr && temp_lock->lock_mode_ == LockMode::EXCLUSIVE) {
			wait_list.push_back(temp_lock->trx_id_);
			return;
		}

		for (; temp_lock != nullptr; temp_lock = temp_lock->prev_) {
			if (temp_lock->lock_mode_ == LockMode::EXCLUSIVE) {
				return;
			} else if (temp_lock->trx_id_ != target->trx_id_) {
				wait_list.push_back(temp_lock->trx_id_);
			}
		}
	}
}

TrxFlag LockManager::lock_acquire(int table_id, int64_t key, int trx_id, LockMode lock_mode) {
	std::unique_lock<std::mutex> lock_table(lock_table_latch_);
	
	std::pair<int, int64_t> key_pair = std::make_pair(table_id, key);

	if (hash_table_.find(key_pair) == hash_table_.end()) {
		hash_table_.emplace(key_pair, lock_list_t{table_id, key});
	}

	lock_list_t& lock_list = hash_table_[key_pair];

	std::vector<int> wait_list;

	// 아무 lock도 안걸려있는 경우 그냥 acquire
	if (lock_list.head_ == nullptr) {
		lock_t* new_lock = new lock_t(nullptr, nullptr, &lock_list, lock_mode, trx_id);
		lock_list.head_ = new_lock;
		lock_list.tail_ = new_lock;

		new_lock->can_wake_ = true;
        new_lock->is_wake_ = true;

		TransactionManager::instance().addLockToTrxList(trx_id, wait_list, new_lock);

		lock_table.unlock();
		return TrxFlag::SUCCESS;
	}

	if (lock_mode == LockMode::SHARED) { // s락인 경우 앞에 같은 trx id 가진 락 있을 때 aquire 허용
		for (lock_t* temp_lock = lock_list.tail_; temp_lock != nullptr; temp_lock = temp_lock->prev_) {
			if (temp_lock->trx_id_ == trx_id) {
				return TrxFlag::SUCCESS;
			}
		}
	} else { // x락인 경우 앞에 같은 trx id 가진 x락 있을 때 aquire 허용
		for (lock_t* temp_lock = lock_list.tail_; temp_lock != nullptr; temp_lock = temp_lock->prev_) {
			if (temp_lock->trx_id_ == trx_id && temp_lock->lock_mode_ == LockMode::EXCLUSIVE) {
				return TrxFlag::SUCCESS;
			}
		}
	}

	lock_t* new_lock = new lock_t(lock_list.tail_, nullptr, &lock_list, lock_mode, trx_id);
	lock_list.tail_->next_ = new_lock;
	lock_list.tail_ = new_lock;

	if (lock_mode == LockMode::EXCLUSIVE) { // 앞에 trx id가 같은 s락만 있을 때 깰 수 있음
		if (checkExclusiveCanWake(new_lock)) { 
			new_lock->can_wake_ = true;
			new_lock->is_wake_ = true;

			TransactionManager::instance().addLockToTrxList(trx_id, wait_list, new_lock);

			lock_table.unlock();
			return TrxFlag::SUCCESS;
		}
	} else if (lock_mode == LockMode::SHARED) { // 바로 앞에 s락만 있을 때만 깰 수 있음
		if (checkSharedCanWake(new_lock)) {
			new_lock->can_wake_ = true;
			new_lock->is_wake_ = true;

			TransactionManager::instance().addLockToTrxList(trx_id, wait_list, new_lock);

			lock_table.unlock();
			return TrxFlag::SUCCESS;
		}
	}

	findNeedToWait(wait_list, new_lock);

	if (TransactionManager::instance().addLockToTrxList(trx_id, wait_list, new_lock) == false) {
		return TrxFlag::ABORT;
	}

	lock_list.cv_.get()->wait(lock_table, [&] {
        if (new_lock->can_wake_) {
            new_lock->can_wake_ = true;
			new_lock->is_wake_ = true;
            return true;
        } else {
			return false;
		}
    });

	return TrxFlag::SUCCESS;
}

int LockManager::lock_release(lock_t* lock_obj) {
	std::unique_lock<std::mutex> lock_table(lock_table_latch_);

	if (lock_obj == nullptr) {
		return -1;
	}

	lock_list_t& lock_list = *(lock_obj->sentinel_);

	if (lock_obj->prev_ == nullptr) {
		lock_list.head_ = lock_obj->next_;
	} else {
		lock_obj->prev_->next_ = lock_obj->next_;
	}

	LockMode cur_lockmode = lock_obj->lock_mode_;
	lock_t* next_obj = lock_obj->next_;
	if (next_obj == nullptr) {
		lock_list.tail_ = lock_obj->prev_;
	} else {
		lock_obj->next_->prev_ = lock_obj->prev_;
	}

	delete lock_obj;

	if (next_obj != nullptr) {
		if (cur_lockmode == LockMode::EXCLUSIVE) {
			if (next_obj->lock_mode_ == LockMode::EXCLUSIVE) {
				next_obj->can_wake_ = true;
			} else {
				for (lock_t* temp = next_obj; temp != nullptr; temp = temp->next_) {
					if (temp->lock_mode_ == LockMode::SHARED) {
						temp->can_wake_ = true;
					} else {
						break;
					}
				}
			}
		} else {
			if (next_obj->lock_mode_ == LockMode::EXCLUSIVE) {
				if (checkExclusiveCanWake(next_obj)) {
					next_obj->can_wake_ = true;
				}
			} else if (next_obj->next_ != nullptr && next_obj->next_->lock_mode_ == LockMode::EXCLUSIVE) {
				if (checkExclusiveCanWake(next_obj->next_)) {
					next_obj->next_->can_wake_ = true;
				}
			}
		}
		
		lock_list.cv_.get()->notify_all();
	}

	return 0;
}

int LockManager::lock_release_without_lock(lock_t* lock_obj) {
	if (lock_obj == nullptr) {
		return -1;
	}

	lock_list_t& lock_list = *(lock_obj->sentinel_);

	if (lock_obj->prev_ == nullptr) {
		lock_list.head_ = lock_obj->next_;
	} else {
		lock_obj->prev_->next_ = lock_obj->next_;
	}

	LockMode cur_lockmode = lock_obj->lock_mode_;
	lock_t* next_obj = lock_obj->next_;
	if (next_obj == nullptr) {
		lock_list.tail_ = lock_obj->prev_;
	} else {
		lock_obj->next_->prev_ = lock_obj->prev_;
	}

	delete lock_obj;

	if (next_obj != nullptr) {
		if (cur_lockmode == LockMode::EXCLUSIVE) {
			if (next_obj->lock_mode_ == LockMode::EXCLUSIVE) {
				next_obj->can_wake_ = true;
			} else {
				for (lock_t* temp = next_obj; temp != nullptr; temp = temp->next_) {
					if (temp->lock_mode_ == LockMode::SHARED) {
						temp->can_wake_ = true;
					} else {
						break;
					}
				}
			}
		} else {
			if (next_obj->lock_mode_ == LockMode::EXCLUSIVE) {
				if (checkExclusiveCanWake(next_obj)) {
					next_obj->can_wake_ = true;
				}
			} else if (next_obj->next_ != nullptr && next_obj->next_->lock_mode_ == LockMode::EXCLUSIVE) {
				if (checkExclusiveCanWake(next_obj->next_)) {
					next_obj->next_->can_wake_ = true;
				}
			}
		}
		
		lock_list.cv_.get()->notify_all();
	}

	return 0;
}