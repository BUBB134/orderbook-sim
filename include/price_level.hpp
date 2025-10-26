#pragma once
#include "types.hpp"
#include <cstddef>

struct Order {
    OrderId id;
    Side side;
    Price px;
    Qty qty;
    TimeNs ts;
    Order* prev {nullptr};
    Order* next {nullptr};
    struct PriceLevel* level {nullptr};
};

struct PriceLevel {
    Price price {0};
    Order* head {nullptr};
    Order* tail {nullptr};
    Qty total_qty {0};

    void push_back(Order* o);
    void remove(Order* o);
    bool empty() const { return head == nullptr; }
};