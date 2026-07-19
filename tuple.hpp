#pragma once
#include <string>
#include <vector>
#include <iostream>

struct Tuple {
    std::vector<std::string> columns;
    int page_id  = -1;  // de dónde vino (para debug)
    int slot_id  = -1;

    Tuple() = default;
    Tuple(std::vector<std::string> cols, int pid = -1, int sid = -1)
        : columns(std::move(cols)), page_id(pid), slot_id(sid) {}

    // Parsea "kerin|larico|22" → ["kerin", "larico", "22"]
    static Tuple FromString(const std::string& raw, int pid, int sid) {
        std::vector<std::string> cols;
        std::string token;
        for (char c : raw) {
            if (c == '|') {
                cols.push_back(token);
                token.clear();
            } else {
                token += c;
            }
        }
        if (!token.empty()) cols.push_back(token);
        return Tuple(cols, pid, sid);
    }

    void Print() const {
        std::cout << "(";
        for (size_t i = 0; i < columns.size(); ++i) {
            std::cout << columns[i];
            if (i + 1 < columns.size()) std::cout << ", ";
        }
        std::cout << ")  [p" << page_id << ":s" << slot_id << "]";
    }
};