# Stock Trading System

## Overview

This project implements a real-time stock trading engine that efficiently matches buy and sell orders. The system is designed to handle concurrent order placement from multiple traders while maintaining data integrity through lock-free data structures.

## Features

- Support for 1,024 different ticker symbols
- Lock-free concurrency using atomic operations
- O(n) time complexity for order matching
- Simulation of concurrent trading with multiple threads
- Price-prioritized matching (lowest sell price matched first)

## Requirements

- C compiler with C11 support (GCC or Clang)
- POSIX threads library
- Standard C libraries

## Implementation Details

### Key Components

1. **Order Structure**: Represents buy or sell orders with:
   - Order type (Buy/Sell)
   - Ticker symbol
   - Quantity
   - Price
   - Matching status

2. **Order Book**: A lock-free data structure storing all orders

3. **Key Functions**:
   - `addOrder`: Adds new orders to the book
   - `matchOrder`: Matches compatible buy and sell orders
   - `simulateOrders`: Simulates random order placement

### Matching Algorithm

The matching algorithm follows these steps:
1. Iterates through all buy orders
2. For each buy order, finds the lowest-priced matching sell order
3. Executes the match by updating quantities
4. Marks orders as matched when fully executed

### Lock-Free Design

The implementation uses atomic operations to ensure thread safety:
- Atomic counters for order IDs
- Atomic flags for order matching status
- Thread-safe order addition and matching



```bash
gcc -o stock_trading_system stock_trading_system.c -pthread
```

### Running

```bash
./stock_trading_system