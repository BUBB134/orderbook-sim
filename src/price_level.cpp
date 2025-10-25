#include "price_level.hpp"

void PriceLevel::push_back(Order* o) {
    o->prev = tail;
    o->next = nullptr;
    o->level = this;
    if (tail) tail->next = o;
    tail = o;
    total_qty += o->qty;
}

void PriceLevel::remove(Order* o){
    if(o->prev) o->prev->next = o->next;
    else head = o->next;
    if(o->next) o->next->prev = o->prev;
    else tail = o->prev;
    total_qty -= o->qty;
    o->prev = o->next = nullptr;
    o->level = nullptr;
}
