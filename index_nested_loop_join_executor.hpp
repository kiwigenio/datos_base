#pragma once
#include "abstract_executor.hpp"
#include "b_plus_tree.hpp"
#include <vector>

class IndexNestedLoopJoinExecutor : public AbstractExecutor {
private:
    AbstractExecutor* outer_;
    BPlusTree<int>* index_;         // índice B+ sobre la inner
    BufferPoolManager* bpm_;
    int outer_col_;                 // columna de join en outer (debe ser int)
    Tuple* curr_outer_;
    Tuple joined_;

    Tuple MergeTuple(const Tuple& a, const Tuple& b) {
        Tuple result;
        result.page_id = a.page_id;
        result.slot_id = a.slot_id;
        for (auto& c : a.columns) result.columns.push_back(c);
        for (auto& c : b.columns) result.columns.push_back(c);
        return result;
    }

public:
    IndexNestedLoopJoinExecutor(AbstractExecutor* outer, BPlusTree<int>* index,
                                BufferPoolManager* bpm, int outer_col)
        : outer_(outer), index_(index), bpm_(bpm),
          outer_col_(outer_col), curr_outer_(nullptr) {}

    void Open() override {
        outer_->Open();
        curr_outer_ = outer_->Next();
    }

    Tuple* Next() override {
        while (curr_outer_ != nullptr) {
            if (outer_col_ < (int)curr_outer_->columns.size()) {
                try {
                    int key = std::stoi(curr_outer_->columns[outer_col_]);
                    RID rid;
                    if (index_->GetValue(key, &rid)) {
                        Page* p = bpm_->FetchPage(rid.page_id);
                        if (p != nullptr) {
                            std::string raw = p->get_registro(rid.slot_id);
                            bpm_->UnpinPage(rid.page_id, false);
                            if (raw != "registro borrado" && raw != "Slot Id inválido") {
                                Tuple inner_t = Tuple::FromString(raw, rid.page_id, rid.slot_id);
                                joined_ = MergeTuple(*curr_outer_, inner_t);
                                curr_outer_ = outer_->Next();
                                return &joined_;
                            }
                        }
                    }
                } catch (...) {}
            }
            curr_outer_ = outer_->Next();
        }
        return nullptr;
    }

    void Close() override {
        outer_->Close();
        curr_outer_ = nullptr;
    }
};