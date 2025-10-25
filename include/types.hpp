#pragma once
#include <cstdint>
#include <string>
#include <optional>

// ===================
// Basic Type Aliases
// ===================
using Price = int64_t;      // Price in smallest currency unit (e.g., cents)
using Qty = int64_t;        // Quantity of shares/contracts
using OrderId = uint64_t;   // Unique identifier for an order
using TimeNs = uint64_t;    // Timestamp in nanoseconds since epoch
using Symbol = std::string; // Trading symbol (e.g., "AAPL")

// ===================
// Enumerations
// ===================
enum class Side : uint8_t
{
    BUY = 0,
    SELL = 1
};

enum class OrderType : uint8_t
{
    LIMIT = 0,
    MARKET = 1
};

enum class EventType : uint8_t
{
    ADD = 0,
    CANCEL = 1,
    REPLACE = 2,
    TRADE = 3,
    SNAPSHOT = 4
};

// ===================
// Core Data Structs
// ===================

// Represents a trade resulting from a match
struct Trade
{
    OrderId taker_id; // Incoming order that triggered the match
    OrderId maker_id; // Resting order that was matched against
    Side taker_side;  // Buy or Sell
    Price price;      // Execution price
    Qty qty;          // Executed quantity
    TimeNs ts;        // Timestamp of the trade
};

// Represents an inbound order event (for replay or testing)
struct OrderEvent
{
    TimeNs ts;               // Event timestamp
    EventType type;          // Type of event
    OrderId id;              // Unique order identifier
    Side side;               // Buy or Sell
    OrderType order_type;    // Market or Limit
    std::optional<Price> px; // Price (if limit order)
    std::optional<Qty> qty;  // Quantity (if applicable)
};

// ===================
// Helper Utilities
// ===================

inline const char *side_to_str(Side s)
{
    return s == Side::BUY ? "BUY" : "SELL";
}

inline Side opposite(Side s){
 return s == Side::BUY ? Side::SELL : Side::BUY;
}