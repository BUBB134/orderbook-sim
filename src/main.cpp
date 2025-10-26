#include <iostream>
#include <vector>
#include <iomanip>
#include "../include/order_book.hpp"

static void print_trades(const std::vector<Trade> &trades)
{
    if (trades.empty())
    {
        std::cout << "No trades\n";
        return;
    }
    for (const auto &t : trades)
    {
        std::cout << "TRADE taker=" << t.taker_id
                  << " maker=" << t.maker_id
                  << " side=" << (t.taker_side == Side::BUY ? "BUY" : "SELL")
                  << " px=" << t.price
                  << " qty=" << t.qty
                  << " ts=" << t.ts << "\n";
    }
}

static void print_book_state(const OrderBook &ob)
{
    auto bid = ob.best_bid();
    auto ask = ob.best_ask();

    std::cout << std::fixed <<std::setprecision(2);
    std::cout << "-------------------------------------------------\n";
    std::cout << " Best Bid: ";
    if (bid) std::cout << *bid;
    else std::cout << "(none)";
    std::cout << "  | Best Ask: ";
    if (ask) std::cout << *ask;
    else std::cout << "(none)";
    std::cout << "\n----------------------------------------------------";
}

// Simple demonstration of engine logic
int main() {
    OrderBook ob;

    std::cout << "=== Simple OrderBook Demo ===\n";

    // Add a few resting orders
    std::cout << "Add sell 101: 100 @ 10.10\n";
    ob.add_limit(101, Side::SELL, 1010, 100, 1);   // price in integer ticks
    std::cout << "Add buy  201: 50 @ 10.00\n";
    ob.add_limit(201, Side::BUY, 1000, 50, 2);

    print_book_state(ob);

    // Add a crossing buy (should match the 10.10 ask)
    std::cout << "\nAdd buy 202: 75 @ 10.15 (crossing)\n";
    auto trades = ob.add_limit(202, Side::BUY, 1015, 75, 3);
    print_trades(trades);
    print_book_state(ob);

    // Add a non-crossing sell
    std::cout << "\nAdd sell 103: 50 @ 10.20 (rests)\n";
    ob.add_limit(103, Side::SELL, 1020, 50, 4);
    print_book_state(ob);

    // Cancel an order
    std::cout << "\nCancel order 201\n";
    ob.cancel(201);
    print_book_state(ob);

    // Add a market buy
    std::cout << "\nMarket buy 104 qty=60\n";
    trades = ob.add_market(104, Side::BUY, 60, 5);
    print_trades(trades);
    print_book_state(ob);

    std::cout << "\nDemo complete.\n";
    return 0;
}