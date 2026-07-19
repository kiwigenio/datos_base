#pragma once
#include "abstract_executor.hpp"
#include <functional>

class SelectExecutor : public AbstractExecutor {
private:
    AbstractExecutor* child_;
    std::function<bool(const Tuple&)> predicate_;

public:
    SelectExecutor(AbstractExecutor* child, std::function<bool(const Tuple&)> predicate)
        : child_(child), predicate_(predicate) {}

    void Open() override { child_->Open(); }

    Tuple* Next() override {
        Tuple* t;
        while ((t = child_->Next()) != nullptr) {
            if (predicate_(*t)) return t;
        }
        return nullptr;
    }

    void Close() override { child_->Close(); }
};