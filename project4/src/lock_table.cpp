#include <unordered_map>
#include <cstdint>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <cstdio>

#include <lock_table.h>

namespace std {
template <class T1, class T2>
struct hash<std::pair<T1, T2>> {
    size_t operator() (const std::pair<T1, T2>& p) const {
        auto a = std::hash<T1>{}(p.first);
        auto b = std::hash<T2>{}(p.second);
        
        return a ^ b;
    }
};
}

struct lock_list_t
{
	int table_id_;
	int64_t record_id_;
	lock_t* tail_;
	lock_t* head_;

	lock_list_t(int table_id, int64_t record_id) : table_id_(table_id), 
												   record_id_(record_id),
												   tail_(nullptr),
												   head_(nullptr) {	};
	lock_list_t() : tail_(nullptr),
					head_(nullptr) {	} 
};

struct lock_t {
	/* NO PAIN, NO GAIN. */
	lock_t* prev_;
	lock_t* next_;
	lock_list_t* sentinel_;
	bool can_wake_;

	lock_t(lock_t* prev, lock_t* next, lock_list_t* sentinel) : prev_(prev), 
																next_(next), 
																sentinel_(sentinel),
																can_wake_(false) {	}
};

std::mutex lock_table_latch;

//std::mutex record_mutex;
std::condition_variable condition_var;

std::unordered_map<std::pair<int, int64_t>, lock_list_t> hash_table;

int init_lock_table()
{
	/* DO IMPLEMENT YOUR ART !!!!! */
	for(auto [x,y] : hash_table){

	}
	return 0;
}

lock_t* lock_acquire(int table_id, int64_t key) {
	/* ENJOY CODING !!!! */
	std::unique_lock<std::mutex> lock_table(lock_table_latch);
	
	std::pair<int, int64_t> key_pair = std::make_pair(table_id, key);

	if (hash_table.find(key_pair) == hash_table.end()) {
		hash_table.emplace(key_pair, lock_list_t(table_id, key));
	}

	lock_list_t& lock_list = hash_table[key_pair];
	
	if (lock_list.head_ == nullptr) {
		lock_t* new_lock = new lock_t(nullptr, nullptr, &lock_list);
		lock_list.head_ = new_lock;
		lock_list.tail_ = new_lock;

		new_lock->can_wake_ = true;

		lock_table.unlock();
		return new_lock;
	}

	lock_t* new_lock = new lock_t(lock_list.tail_, nullptr, &lock_list);
	lock_list.tail_->next_ = new_lock;
	lock_list.tail_ = new_lock;

	condition_var.wait(lock_table, [&] { return new_lock->can_wake_; });

	return new_lock;
}

int lock_release(lock_t* lock_obj) {
	/* GOOD LUCK !!! */
	std::unique_lock<std::mutex> lock_table(lock_table_latch);

	if (lock_obj == nullptr) {
		lock_table.unlock();
		return -1;
	}

	lock_list_t& lock_list = *(lock_obj->sentinel_);

	if (lock_obj->prev_ == nullptr) {
		lock_list.head_ = lock_obj->next_;
	} else {
		lock_obj->prev_->next_ = lock_obj->next_;
	}

	lock_t* next_obj = lock_obj->next_;
	if (next_obj == nullptr) {
		lock_list.tail_ = lock_obj->prev_;
	} else {
		lock_obj->next_->prev_ = lock_obj->prev_;
	}

	delete lock_obj;

	if (next_obj != nullptr) {
		next_obj->can_wake_ = true;
		condition_var.notify_all();
	}

	return 0;
}
