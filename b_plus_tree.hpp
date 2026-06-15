#pragma once 
#include "buffer_pool_manager.hpp"
#include "b_plus_tree_internal_page.hpp"
#include "b_plus_tree_leaf_page.hpp"
#include <vector>

template<typename KeyType>
class BPlusTree{
    private: 
        BufferPoolManager *bpm_;
        int32_t root_page_id_;
    public:
        BPlusTree(BufferPoolManager *bpm): bpm_(bpm), root_page_id_(-1){}

        bool IsEmpty() const {return root_page_id_ == -1 ;}

        bool GetValue(const KeyType &key, RID *result);

        bool Insert(const KeyType &key, const RID &value);
    private: 
        Page* FindLeafPage(const KeyType &key);
};