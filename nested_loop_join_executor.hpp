#pragma once
#include "abstract_executor.hpp"
#include <vector>

class NestedLoopJoinExecutor : public AbstractExecutor {
private:
    AbstractExecutor* outer_;       // tabla izquierda
    AbstractExecutor* inner_;       // tabla derecha
    int outer_col_;                 // columna de join en outer
    int inner_col_;                 // columna de join en inner

    Tuple* curr_outer_;             // tupla outer actual
    std::vector<Tuple> inner_cache_;// caché de toda la tabla inner
    int inner_idx_;                 // posición actual en el caché
    Tuple joined_;                  // tupla resultado del join

    void CacheInner() {
        inner_cache_.clear();
        inner_->Open();
        Tuple* t;
        while ((t = inner_->Next()) != nullptr) {
            inner_cache_.push_back(*t);
        }
        inner_->Close();
    }

    Tuple MergeTouple(const Tuple& a, const Tuple& b) {
        Tuple result;
        result.page_id = a.page_id;
        result.slot_id = a.slot_id;
        for (auto& c : a.columns) result.columns.push_back(c);
        for (auto& c : b.columns) result.columns.push_back(c);
        return result;
    }

public:
    NestedLoopJoinExecutor(AbstractExecutor* outer, AbstractExecutor* inner,
                           int outer_col, int inner_col)
        : outer_(outer), inner_(inner),
          outer_col_(outer_col), inner_col_(inner_col),
          curr_outer_(nullptr), inner_idx_(0) {}

    void Open() override {
        outer_->Open();
        CacheInner();           // cargamos toda la inner en memoria una sola vez
        curr_outer_ = outer_->Next();
        inner_idx_ = 0;
    }

    Tuple* Next() override {
        while (curr_outer_ != nullptr) {
            while (inner_idx_ < (int)inner_cache_.size()) {
                Tuple& inner_t = inner_cache_[inner_idx_++];

                // verificamos que ambas tuplas tengan las columnas de join
                if (outer_col_ >= (int)curr_outer_->columns.size()) continue;
                if (inner_col_ >= (int)inner_t.columns.size()) continue;

                if (curr_outer_->columns[outer_col_] == inner_t.columns[inner_col_]) {
                    joined_ = MergeTouple(*curr_outer_, inner_t);
                    return &joined_;
                }
            }
            // agotamos el inner para esta tupla outer, avanzamos al siguiente outer
            curr_outer_ = outer_->Next();
            inner_idx_ = 0;
        }
        return nullptr;
    }

    void Close() override {
        outer_->Close();
        inner_cache_.clear();
        curr_outer_ = nullptr;
        inner_idx_ = 0;
    }
};