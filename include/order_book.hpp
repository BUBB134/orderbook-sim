#pragma once
#include <map>
#include <unordered_map>
#include <vector>
#include <optional>
#include "types.hpp"
#include "price_level.hpp"

// OrderBook: single-symbol, single threaded matching engine
class OrderBook
{
public:
    OrderBook() = default;
    ~OrderBook();

    // -----Order Entry APIs-----
    // Adds a LIMIT order; returns any resulting trades
    std::vector<Trade> add_limit(OrderId id, Side side, Price px, Qty qty, TimeNs ts);

    // Adds a MARKET order (price ignored); returns any resulting trades
    std::vector<Trade> add_market(OrderId id, Side side, Qty qty, TimeNs ts);

    // Cancel by OrderId; returns true if found and removed
    bool cancel(OrderId id);

    // Replace: optionally change price/qty
    // If new_px has value and differsL cancel and re-add (time priority reset)
    // If onyl qty decreases: in place shrink
    // If qty increases: cancel and re-add (simplest to preserve invariants)
    bool replace(OrderId id, std::optional<Price> new_px, std::optional<Qty> new_qty, TimeNs ts);

    // -----Queries-----
    std::optional<Price> best_bid() const;
    std::optional<Price> best_ask() const;
    Qty depth_at(Side s, Price px) const;

#ifndef NDEBUG
    // Validate internal invariants (only use for testing, expensive)
    void validate() const;
#endif

private:
    // BUY ladder sorted by descending price, SELL ladder by ascending price
    using BuyLadder = std::map<Price, PriceLevel, std::greater<Price>>;
    using SellLadder = std::map<Price, PriceLevel, std::less<Price>>;

    BuyLadder buy_;
    SellLadder sell_;

    // Fast lookup for cancels/replaces: id -> Order*
    std::unordered_map<OrderId, Order *> order_index_;

    // Storage for Order objects; simple arena to avoid frequent new/delete
    //  For now, allocate individually; could be optimized later if needed
    Order *create_order(OrderId id, Side side, Price px, Qty qty, TimeNs ts);
    void destroy_order(Order *o);

    // Helpers to fetch/create a price level on a given side
    PriceLevel &get_or_create_level(Side side, Price px);
    PriceLevel *find_level(Side side, Price px);
    void erase_level_if_empty(Side side, Price px);

    // Core Matching paths
    void match_buy_against_sell(Qty& taker_qty, Price taker_price, TimeNs ts, std::vector<Trade>& out_trades);
    void match_sell_against_buy(Qty& taker_qty, Price taker_price, TimeNs ts, std::vector<Trade>& out_trades);

    // Common Utilities 
    std::optional<Price> best_price(Side s) const;
    void index_insert(Order* o);
    void index_erase(Order* o);
};