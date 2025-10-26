#include "order_book.hpp"
#include <algorithm>
#include <cassert>
#include <optional>

OrderBook::~OrderBook()
{
    // Clean up all allocated orders
    for (auto &[_, lvl] : buy_)
    {
        for (Order *o = lvl.head; o;)
        {
            Order *nxt = o->next;
            destroy_order(o);
            o = nxt;
        }
    }
    for (auto &[_, lvl] : sell_)
    {
        for (Order *o = lvl.head; o;)
        {
            Order *nxt = o->next;
            destroy_order(o);
            o = nxt;
        }
    }
    order_index_.clear();
}

// -----Public API-----

std::vector<Trade> OrderBook::add_limit(OrderId id, Side side, Price px, Qty qty, TimeNs ts)
{
    std::vector<Trade> trades;
    if (qty <= 0)
        return trades;

    if (side == Side::BUY)
    {
        //  Cross as long as best ask <= buy price
        match_buy_against_sell(qty, px, ts, trades);
        if (qty > 0)
        {
            // Rest remaining
            Order *o = create_order(id, side, px, qty, ts);
            PriceLevel &lvl = get_or_create_level(Side::BUY, px);
            lvl.push_back(o);
            index_insert(o);
        }
    }
    else
    {
        // Sell
        match_sell_against_buy(qty, px, ts, trades);
        if (qty > 0)
        {
            Order *o = create_order(id, side, px, qty, ts);
            PriceLevel &lvl = get_or_create_level(Side::SELL, px);
            lvl.push_back(o);
            index_insert(o);
        }
    }
#ifndef NDEBUG
    validate();
#endif
    return trades;
}

std::vector<Trade> OrderBook::add_market(OrderId id, Side side, Qty qty, TimeNs ts)
{
    std::vector<Trade> trades;
    if (qty <= 0)
        return trades;

    // MARKET price treates as fully aggressive: taker_price = +inf (buy) -inf (sell)
    if (side == Side::BUY)
    {
        // Use a very large price so any ask is crossable
        match_buy_against_sell(qty, std::numeric_limits<Price>::max(), ts, trades);
    }
    else
    {
        match_sell_against_buy(qty, std::numeric_limits<Price>::min(), ts, trades);
    }
#ifndef NDEBUG
    validate();
#endif
    return trades;
}

bool OrderBook::cancel(OrderId id)
{
    auto it = order_index_.find(id);
    if (it == order_index_.end())
        return false; // Not found

    Order *o = it->second;
    PriceLevel *lvl = o->level;
    assert(lvl != nullptr);

    Price px = lvl->price;
    Side s = o->side;

    lvl->remove(o);
    index_erase(o);
    destroy_order(o);

    if (lvl->empty())
    {
        erase_level_if_empty(s, px);
    }
#ifndef NDEBUG
    validate();
#endif
    return true;
}

bool OrderBook::replace(OrderId id, std::optional<Price> new_px, std::optional<Qty> new_qty, TimeNs ts)
{
    auto it = order_index_.find(id);
    if (it == order_index_.end())
        return false; // Not found

    Order *o = it->second;
    Side s = o->side;
    Price old_px = o->px;
    Qty old_qty = o->qty;

    bool price_change = new_px.has_value() && new_px.value() != old_px;
    bool qty_increase = new_qty.has_value() && new_qty.value() > old_qty;

    if (!price_change && new_qty.value() < old_qty)
    {
        PriceLevel *lvl = o->level;
        assert(lvl != nullptr);
        Qty delta = old_qty - new_qty.value();
        o->qty = new_qty.value();
        lvl->total_qty -= delta;
#ifndef NDEBUG
        validate();
#endif
        return true;
    }
    // For price changes or qty increase, do cancel + re-add (simpler, preserves FIFO rules)
    bool ok = cancel(id);
    if (!ok)
        return false;

    Price px = price_change ? new_px.value() : old_px;
    Qty q = new_qty.has_value() ? new_qty.value() : old_qty;

    if (q <= 0)
        return true; // Fully cancelled
    if (s == Side::BUY)
    {
        auto _ = add_limit(id, s, px, q, ts);
        (void)_;
    }
    else
    {
        auto _ = add_limit(id, s, px, q, ts);
        (void)_;
    }
#ifndef NDEBUG
    validate();
#endif
    return trades;
}

// -----Queries-----

std::optional<Price> OrderBook::best_bid() const
{
    return best_price(Side::BUY);
}
std::optional<Price> OrderBook::best_ask() const
{
    return best_price(Side::SELL);
}
Qty OrderBook::depth_at(Side s, Price px) const
{
    if (s == Side::BUY)
    {
        auto it = buy_.find(px);
        return it == buy_.end() ? 0 : it->second.total_qty;
    }
    else
    {
        auto it = sell_.find(px);
        return it == sell_.end() ? 0 : it->second.total_qty;
    }
}

// -----Private Helpers-----

Order *OrderBook::create_order(OrderId id, Side side, Price px, Qty qty, TimeNs ts)
{
    auto *o = new Order();
    o->id = id;
    o->side = side;
    o->px = px;
    o->qty = qty;
    o->ts = ts;
    o->prev = o->next = nullptr;
    o->level = nullptr;
    return o;
}

void OrderBook::destroy_order(Order *o)
{
    delete o;
}

PriceLevel &OrderBook::get_or_create_level(Side side, Price px)
{
    if (side == Side::BUY)
    {
        auto [it, inserted] = buy_.try_emplace(px);
        if (inserted)
            it->second.price = px;
        return it->second;
    }
    else
    {
        auto [it, inserted] = sell_.try_emplace(px);
        if (inserted)
            it->second.price = px;
        return it->second;
    }
}

PriceLevel *OrderBook::find_level(Side side, Price px)
{
    if (side == Side::BUY)
    {
        auto it = buy_.find(px);
        return it == buy_.end() ? nullptr : &it->second;
    }
    else
    {
        auto it = sell_.find(px);
        return it == sell_.end() ? nullptr : &it->second;
    }
}

void OrderBook::erase_level_if_empty(Side side, Price px)
{
    if (side == Side::BUY)
    {
        auto it = buy_.find(px);
        if (it != buy_.end() && it->second.empty())
        {
            buy_.erase(it);
        }
    }
    else
    {
        auto it = sell_.find(px);
        if (it != sell_.end() && it->second.empty())
        {
            sell_.erase(it);
        }
    }
}

std::optional<Price> OrderBook::best_price(Side side) const
{
    if (side == Side::BUY)
    {
        if (buy_.empty())
            return std::nullopt;
        return buy_.begin()->first;
    }
    else
    {
        if (sell_.empty())
            return std::nullopt;
        return sell_.begin()->first;
    }
}

void OrderBook::index_insert(Order *o)
{
    order_index_.emplace(o->id, o);
}
void OrderBook::index_erase(Order *o)
{
    order_index_.erase(o->id);
}

// Core matching routines
void OrderBook::match_buy_against_sell(Qty &taker_qty, Price taker_price, TimeNs ts, std::vector<Trade> &out_trades)
{
    while (taker_qty > 0)
    {
        if (sell_.empty())
            break;
        auto it = sell_.begin();
        PriceLevel &ask_lvl = it->second;
        if (ask_lvl.price > taker_price)
            break;

        while (taker_qty > 0 && ask_lvl.head)
        {
            Order *maker = ask_lvl.head;
            Qty traded = std::min(taker_qty, maker->qty);

            // Emit trade at makers price
            out_trades.push_back(Trade{
                /*taker_id =*/0,
                /*maker_id =*/maker->id,
                /*taker_side =*/Side::BUY,
                /*price =*/ask_lvl.price,
                /*qty =*/traded,
                /*ts =*/ts});

            // Update quantities
            maker->qty -= traded;
            ask_lvl.total_qty -= traded;
            taker_qty -= traded;

            if (maker->qty == 0)
            {
                // Remove maker order
                ask_lvl.remove(maker);
                index_erase(maker);
                destroy_order(maker);
            }
        }
        if (ask_lvl.empty())
        {
            sell_.erase(it);
        }
        else
        {
        }
    }
}

void OrderBook::match_sell_against_buy(Qty &taker_qty, Price taker_price, TimeNs ts, std::vector<Trade> &out_trades)
{
    while (taker_qty > 0)
    {
        if (buy_.empty())
            break;
        auto it = buy_.begin();
        PriceLevel &bid_lvl = it->second;
        if (bid_lvl.price < taker_price)
            break;

        while (taker_qty > 0 && bid_lvl.head)
        {
            Order *maker = bid_lvl.head;
            Qty traded = std::min(taker_qty, maker->qty);

            // Emit trade at makers price
            out_trades.push_back(Trade{
                /*taker_id =*/0,
                /*maker_id =*/maker->id,
                /*taker_side =*/Side::SELL,
                /*price =*/bid_lvl.price,
                /*qty =*/traded,
                /*ts =*/ts});

            // Update quantities
            maker->qty -= traded;
            bid_lvl.total_qty -= traded;
            taker_qty -= traded;

            if (maker->qty == 0)
            {
                // Remove maker order
                bid_lvl.remove(maker);
                index_erase(maker);
                destroy_order(maker);
            }
        }
        if (bid_lvl.empty())
        {
            buy_.erase(it);
        }
        else
        {
        }
    }
}

#ifndef NDEBUG
void OrderBook::validate() const
{
    // Check sortng and totals for buy ladder
    Price last = std::numeric_limits<Price>::max();
    for (const auto &[px, lvl] : buy_)
    {
        assert(px <= last);
        last = px;
        Qty sum = 0;
        for (const Order *o = lvl.head; o; o = o->next)
        {
            assert(o->level == &lvl);
            assert(o->side == Side::BUY);
            assert(o->px == px);
            sum += o->qty;
            if (o->next)
                assert(o->next->prev == o);
        }
        assert(sum == lvl.total_qty);
    }
    // Check sorting and totals for sell ladder
    last = std::numeric_limits<Price>::min();
    for (const auto &[px, lvl] : sell_)
    {
        assert(px >= last);
        last = px;
        Qty sum = 0;
        for (const Order *o = lvl.head; o; o = o->next)
        {
            assert(o->level == &lvl);
            assert(o->side == Side::SELL);
            assert(o->px == px);
            sum += o->qty;
            if (o->next)
                assert(o->next->prev == o);
        }
        assert(sum == lvl.total_qty);
    }
    // Index consistency
    for (const auto &[id, o] : order_index_)
    {
        (void)id;
        assert(o != nullptr);
        assert(o->level != nullptr);
    }
}
#endif