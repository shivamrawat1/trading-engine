#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

#define MAX_TICKERS 1024
#define MAX_ORDERS 10000
#define MAX_TICKER_LENGTH 10

typedef enum {
    BUY,
    SELL
} OrderType;

typedef struct {
    OrderType type;
    char ticker[MAX_TICKER_LENGTH];
    int quantity;
    double price;
    int order_id;
    atomic_bool is_matched;
} Order;

// Lock-free order book
typedef struct {
    atomic_int count;
    Order orders[MAX_ORDERS];
} OrderBook;

OrderBook order_book = {0};

// Array of ticker symbols for simulation
const char* ticker_symbols[] = {"AAPL", "MSFT", "GOOGL", "AMZN", "META", "TSLA", "NVDA", "JPM", "BAC", "WMT"};
const int num_tickers = sizeof(ticker_symbols) / sizeof(ticker_symbols[0]);

// Function to add an order to the order book
int addOrder(OrderType type, const char* ticker, int quantity, double price) {
    if (strlen(ticker) >= MAX_TICKER_LENGTH) {
        printf("Ticker symbol too long\n");
        return -1;
    }
    
    // Get current count and increment atomically
    int current_index = atomic_fetch_add(&order_book.count, 1);
    
    if (current_index >= MAX_ORDERS) {
        printf("Order book is full\n");
        atomic_fetch_sub(&order_book.count, 1); // Revert the increment
        return -1;
    }
    
    // Fill in the order details
    order_book.orders[current_index].type = type;
    strcpy(order_book.orders[current_index].ticker, ticker);
    order_book.orders[current_index].quantity = quantity;
    order_book.orders[current_index].price = price;
    order_book.orders[current_index].order_id = current_index;
    atomic_store(&order_book.orders[current_index].is_matched, false);
    
    printf("Added %s order: %s, %d shares at $%.2f (ID: %d)\n", 
           type == BUY ? "BUY" : "SELL", 
           ticker, quantity, price, current_index);
    
    return current_index;
}

// Function to match buy and sell orders
void matchOrder() {
    int count = atomic_load(&order_book.count);
    
    for (int i = 0; i < count; i++) {
        // Skip already matched orders
        if (atomic_load(&order_book.orders[i].is_matched)) {
            continue;
        }
        
        // Process only buy orders
        if (order_book.orders[i].type == BUY) {
            char* buy_ticker = order_book.orders[i].ticker;
            double buy_price = order_book.orders[i].price;
            int buy_quantity = order_book.orders[i].quantity;
            
            // Find the best (lowest price) matching sell order
            int best_sell_index = -1;
            double best_sell_price = __DBL_MAX__;
            
            for (int j = 0; j < count; j++) {
                if (j != i && 
                    !atomic_load(&order_book.orders[j].is_matched) && 
                    order_book.orders[j].type == SELL &&
                    strcmp(order_book.orders[j].ticker, buy_ticker) == 0 &&
                    order_book.orders[j].price <= buy_price &&
                    order_book.orders[j].price < best_sell_price) {
                    
                    best_sell_index = j;
                    best_sell_price = order_book.orders[j].price;
                }
            }
            
            // If we found a matching sell order
            if (best_sell_index != -1) {
                int sell_quantity = order_book.orders[best_sell_index].quantity;
                int matched_quantity = (buy_quantity < sell_quantity) ? buy_quantity : sell_quantity;
                
                // Update quantities
                order_book.orders[i].quantity -= matched_quantity;
                order_book.orders[best_sell_index].quantity -= matched_quantity;
                
                printf("MATCH: BUY #%d and SELL #%d for %s: %d shares at $%.2f\n",
                       order_book.orders[i].order_id,
                       order_book.orders[best_sell_index].order_id,
                       buy_ticker,
                       matched_quantity,
                       best_sell_price);
                
                // Mark orders as matched if fully executed
                if (order_book.orders[i].quantity == 0) {
                    atomic_store(&order_book.orders[i].is_matched, true);
                }
                
                if (order_book.orders[best_sell_index].quantity == 0) {
                    atomic_store(&order_book.orders[best_sell_index].is_matched, true);
                }
            }
        }
    }
}

// Wrapper function to simulate random orders
void* simulateOrders(void* arg) {
    int num_orders = *((int*)arg);
    
    for (int i = 0; i < num_orders; i++) {
        OrderType type = (rand() % 2 == 0) ? BUY : SELL;
        const char* ticker = ticker_symbols[rand() % num_tickers];
        int quantity = (rand() % 100) + 1;  // 1-100 shares
        double price = (rand() % 10000) / 100.0;  // $0.00-$100.00
        
        addOrder(type, ticker, quantity, price);
        
        // Occasionally match orders
        if (i % 5 == 0) {
            matchOrder();
        }
        
        // Small delay to simulate real-world timing
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = (rand() % 10 + 1) * 1000000; // 1-10 milliseconds
        nanosleep(&ts, NULL);
    }
    
    return NULL;
}

int main() {
    srand(time(NULL));
    
    // Create multiple threads to simulate concurrent order placement
    pthread_t threads[4];
    int orders_per_thread = 50;
    
    for (int i = 0; i < 4; i++) {
        pthread_create(&threads[i], NULL, simulateOrders, &orders_per_thread);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Final matching pass
    matchOrder();
    
    printf("Simulation complete. Total orders: %d\n", atomic_load(&order_book.count));
    
    return 0;
}
