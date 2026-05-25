#ifndef FRAME_HPP
#define FRAME_HPP

#include "page.hpp"

struct Frame {
    Page page;
    int pin_count = 0;
    bool is_dirty = false;

    void Reset(int32_t new_page_id = 0) {
        pin_count = 0;
        is_dirty = false;

        page = Page(new_page_id); 
    }
};

#endif // FRAME_HPP