#pragma once 
#include <cstdint>


enum class IndexPageType{
    INVALID_INDEX_PAGE = 0, 
    LEAF_PAGE = 1, 
    INTERNAL_PAGE = 2
};

class BPlusTreePage{
    protected: 
        IndexPageType page_type_; 
        int32_t size_; 
        int32_t max_size_; 
        int32_t parent_page_id_; 
        int32_t page_id_;
    public:
        bool IsLeafPage() const { return page_type_ == IndexPageType:: LEAF_PAGE;}
        bool IsRootPage() const { return parent_page_id_ == -1;}

        void SetPageType(IndexPageType page_type) { page_type_ = page_type;}

        int GetSize() const { return size_;}
        void SetSize(int size) { size_ = size;}
        void IncreaseSize(int amount) { size_ += amount;}

        int GetMaxSize() const {return max_size_;}
        void SetMaxSize(int max_size) { max_size_ = max_size;}

        int GetMinSize() const { 
            if ( IsRootPage()){ 
                return IsLeafPage() ? 1:2;
            }
            return max_size_ / 2; 
        }

        int32_t GetParentPageId() const { return parent_page_id_;}
        void SetParentPageId(int32_t parent_page_id) { parent_page_id_ = parent_page_id;}

        int32_t GetPageId() const { return page_id_;}
        void SetPageId(int32_t page_id ) { page_id_ = page_id;}

};
