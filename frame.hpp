#ifndef FRAME_HPP
#define FRAME_HPP

#include "page.hpp"

struct Frame {
    Page page;
    int pin_count = 0;
    bool is_dirty = false;

    void Reset() {
        pin_count = 0;
        is_dirty = false;

        memset(&page, 0, sizeof(Page)); 
    }
};

#endif // FRAME_HPP