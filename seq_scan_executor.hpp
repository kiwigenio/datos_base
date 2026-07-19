#pragma once
#include "abstract_executor.hpp"
#include "buffer_pool_manager.hpp"

class SeqScanExecutor : public AbstractExecutor {
private:
    BufferPoolManager* bpm_;
    int start_page_;
    int total_pages_;
    int curr_page_;
    int curr_slot_;
    Page* curr_page_ptr_;
    Tuple current_tuple_;

public:
    SeqScanExecutor(BufferPoolManager* bpm, int start_page = 0, int total_pages = 100)
        : bpm_(bpm), start_page_(start_page), total_pages_(total_pages),
          curr_page_(start_page), curr_slot_(0), curr_page_ptr_(nullptr) {}

    void Open() override;
    Tuple* Next() override;
    void Close() override;
};