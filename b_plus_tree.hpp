#pragma once 
#include "buffer_pool_manager.hpp"
#include "b_plus_tree_internal_page.hpp"
#include "b_plus_tree_leaf_page.hpp"
#include <vector>
#include <queue>

template<typename KeyType>
class BPlusTree{
    private: 
        BufferPoolManager *bpm_;
        int32_t root_page_id_;
        Page* FindLeafPage(const KeyType &key);

        bool AdjustRoot(BPlusTreePage *node);
        
        template <typename N>
        bool CoalesceOrRedistribute(N *node);

        template <typename N>
        bool Coalesce(N **neighbor_node, N **node, BPlusTreeInternalPage<KeyType> **parent, int index);

        template <typename N>
        void Redistribute(N *neighbor_node, N *node, int index);
    public:
        BPlusTree(BufferPoolManager *bpm): bpm_(bpm), root_page_id_(-1){}

        bool IsEmpty() const {return root_page_id_ == -1 ;}

        bool GetValue(const KeyType &key, RID *result);

        bool Insert(const KeyType &key, const RID &value);

        void Remove(const KeyType &key);

        void PrintTree();

        int32_t GetRootPageId() const { return root_page_id_; }
        void SetRootPageId(int32_t root_id) { root_page_id_ = root_id; }

};