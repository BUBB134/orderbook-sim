# orderbook-sim
<h2>What is an Order Book?</h2>

An order book is a real time data structure that matches buy and sell orders for a given financial instrument. </br>
It maintains all <b>limit orders</b> sorted by price and time - highest bids on one side, lowest asks on the other - and when new orders arrive, it either matches them immediately or <b>rests</b> them in the book for future trades.

<h2>Technical implementation</h2>
<h4> 1. Core concept </h4>
<li> The order book is basically two priority queues:
<ul> <li> A bid ladder (buyers, sorted descending by price) </li>
<li> An ask ladder (sellers, sorted ascending by price) </li> </ul>
Each price level holds a <b>FIFO queue</b> of orders at that price, preserving price-time priority.

<h4> 2. When a new order arrives </h4>





<h4> 3. Cancels and replaces </h4>





<h4> 4. Invariants & Queries </h4>




<h4> 5. Examples </h4>
=== Simple OrderBook Demo ===<br>
Add sell 101: 75 @ 10.10 <br>
Add buy  201 @ 10.00<br>
-------------------------------------------------<br>
 Best Bid: 1000  | Best Ask: 1010<br>
----------------------------------------------------<br>
Add buy 202: 75 @ 10.15 (crossing)<br>
TRADE taker=202 maker=101 side=BUY px=1010 qty=75 ts=3<br>
-------------------------------------------------<br>
 Best Bid: 1000  | Best Ask: 1010<br>
----------------------------------------------------<br>
Add sell 103: 50 @ 10.20 (rests)<br>
-------------------------------------------------<br>
 Best Bid: 1000  | Best Ask: 1010<br>
----------------------------------------------------<br>
Cancel order 201<br>
-------------------------------------------------<br>
 Best Bid: (none)  | Best Ask: 1010<br>
----------------------------------------------------<br>
Market buy 104 qty=60<br>
TRADE taker=104 maker=101 side=BUY px=1010 qty=25 ts=5<br>
TRADE taker=104 maker=103 side=BUY px=1020 qty=35 ts=5<br>
-------------------------------------------------<br>
 Best Bid: (none)  | Best Ask: 1020<br>
----------------------------------------------------<br>
Demo complete.