#pragma once
#include "abstract_executor.hpp"
#include <vector>

class ProjectExecutor : public AbstractExecutor {
private:
    AbstractExecutor* child_;
    std::vector<int> col_indices_;  // qué columnas proyectar
    Tuple projected_;

public:
    ProjectExecutor(AbstractExecutor* child, std::vector<int> col_indices)
        : child_(child), col_indices_(std::move(col_indices)) {}

    void Open() override { child_->Open(); }

    Tuple* Next() override {
        Tuple* t = child_->Next();
        if (t == nullptr) return nullptr;

        projected_.columns.clear();
        projected_.page_id = t->page_id;
        projected_.slot_id = t->slot_id;

        for (int idx : col_indices_) {
            if (idx >= 0 && idx < (int)t->columns.size()) {
                projected_.columns.push_back(t->columns[idx]);
            } else {
                projected_.columns.push_back("NULL");
            }
        }
        return &projected_;
    }

    void Close() override { child_->Close(); }
};