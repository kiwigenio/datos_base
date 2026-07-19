#include "seq_scan_executor.hpp"

void SeqScanExecutor::Open() {
    curr_page_ = start_page_;  //  reinicia al inicio del rango
    curr_slot_ = 0;
    curr_page_ptr_ = nullptr;
}

Tuple* SeqScanExecutor::Next() {
    while (curr_page_ < start_page_ + total_pages_) {  //  respeta el rango
        if (curr_page_ptr_ == nullptr) {
            curr_page_ptr_ = bpm_->FetchPage(curr_page_);
            if (curr_page_ptr_ == nullptr) {
                curr_page_++;
                continue;
            }
        }

        while (curr_slot_ < curr_page_ptr_->header.num_slots) {
            std::string raw = curr_page_ptr_->get_registro(curr_slot_);
            int slot = curr_slot_;
            curr_slot_++;
            if (raw == "registro borrado" || raw == "Slot Id inválido") continue;
            current_tuple_ = Tuple::FromString(raw, curr_page_, slot);
            return &current_tuple_;
        }

        bpm_->UnpinPage(curr_page_, false);
        curr_page_ptr_ = nullptr;
        curr_slot_ = 0;
        curr_page_++;
    }
    return nullptr;
}

void SeqScanExecutor::Close() {
    if (curr_page_ptr_ != nullptr) {
        bpm_->UnpinPage(curr_page_, false);
        curr_page_ptr_ = nullptr;
    }
    curr_page_ = start_page_;
    curr_slot_ = 0;
}