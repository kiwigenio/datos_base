#pragma once 
#include <cstdint> 

struct RID{
    int32_t page_id;
    int32_t slot_id;
    RID(): page_id(-1), slot_id(-1) {}

    RID(int32_t page_id, int32_t slot_id): page_id(page_id), slot_id(slot_id){}
}; 