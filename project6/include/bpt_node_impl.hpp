template <NodeType T>
BPTNode<T>::BPTNode(uint32_t table_id, pagenum_t pagenum) : buffer_manager_(BufferManager::instance()),
                                                            is_changed_(false),
                                                            pagenum_(pagenum) {
    BufferBlock& block = buffer_manager_.start_use_node(table_id, pagenum, buffer_idx_);
    page_lock_ = &(block.page_lock_);
    page_t& page = block.frame_;
    node_ = &page;

    is_initialized_ = true;
    is_end_use_ = false;
}

template <NodeType T>
BPTNode<T>::~BPTNode() {
    if (is_initialized_ && !is_end_use_) {
        end_use_node();
    }
}

template <NodeType T>
void BPTNode<T>::init_node(uint32_t table_id, pagenum_t pagenum) {
    if (is_initialized_ && !is_end_use_) {
        end_use_node();
    }
    
    BufferBlock& block = buffer_manager_.start_use_node(table_id, pagenum, buffer_idx_);
    page_lock_ = &(block.page_lock_);
    page_t& page = block.frame_;
    node_ = &page;
    
    pagenum_ = pagenum;

    is_initialized_ = true;
    is_end_use_ = false;
    is_changed_ = false;
}

template <NodeType T>
void BPTNode<T>::end_use_node() {
    if (is_initialized_ && !is_end_use_) {
        buffer_manager_.end_use_node(buffer_idx_, is_changed_);
        is_end_use_ = true;
    }
}

template <NodeType T>
pagenum_t BPTNode<T>::get_pagenum() {
    return pagenum_;
}

template <NodeType T>
void BPTNode<T>::set_page(const page_t& new_page) {
    *node_ = new_page;
    is_changed_ = true;
}

template <NodeType T>
const page_t& BPTNode<T>::get_page() {
    return *node_;
}

template <NodeType T>
void BPTNode<T>::set_new_node() {
    node_->page_.node_page_.header_.is_leaf = (T == NodeType::LEAF);
    node_->page_.node_page_.header_.number_of_keys = 0;
    node_->page_.node_page_.header_.parent = 0;
    node_->page_.node_page_.header_.spare_page = 0;

    is_changed_ = true;
}

template <NodeType T>
pagenum_t BPTNode<T>::parent() {
    return node_->page_.node_page_.header_.parent;
}

template <NodeType T>
void BPTNode<T>::parent(const pagenum_t parent_n) {
    node_->page_.node_page_.header_.parent = parent_n;
    is_changed_ = true;
}

template <NodeType T>
bool BPTNode<T>::is_leaf() {
    return node_->page_.node_page_.header_.is_leaf;
}

template <NodeType T>
uint32_t BPTNode<T>::num_keys() {
    return node_->page_.node_page_.header_.number_of_keys;
}

template <NodeType T>
void BPTNode<T>::num_keys(const uint32_t num_keys_n) {
    node_->page_.node_page_.header_.number_of_keys = num_keys_n;
    is_changed_ = true;
}

template <NodeType T>
void BPTNode<T>::increase_num_keys(const uint32_t i) {
    node_->page_.node_page_.header_.number_of_keys += i;
    is_changed_ = true;
}

template <NodeType T>
void BPTNode<T>::decrease_num_keys(const uint32_t i) {
    node_->page_.node_page_.header_.number_of_keys -= i;
    is_changed_ = true;
}

template <NodeType T>
pagenum_t BPTNode<T>::spare_page() {
    return node_->page_.node_page_.header_.spare_page;
}

template <NodeType T>
void BPTNode<T>::spare_page(const pagenum_t spare_n) {
    node_->page_.node_page_.header_.spare_page = spare_n;
    is_changed_ = true;
}

template <NodeType T>
int64_t BPTNode<T>::lsn() {
    return node_->page_.node_page_.header_.page_lsn_;
}

template <NodeType T>
void BPTNode<T>::lsn(const int64_t lsn_n) {
    node_->page_.node_page_.header_.page_lsn_ = lsn_n;
    is_changed_ = true;
}

template <NodeType T>
const children_t& BPTNode<T>::children() {
    return node_->page_.node_page_.body_.childeren;
}

template <NodeType T>
const records_t& BPTNode<T>::records() {
    return node_->page_.node_page_.body_.records;
}