#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>

const int STORAGE_CAPACITY = 8;
const int FACTORY_COUNT = 3;
const int CUSTOMER_COUNT = 2;
const int TOTAL_ITEMS = 24;

struct CircularStorage {
    int items[STORAGE_CAPACITY];
    volatile int write_idx;
    volatile int read_idx;
    
    CircularStorage() : write_idx(0), read_idx(0) {}
} warehouse;

sem_t space_available;
sem_t items_ready;
pthread_mutex_t storage_guard;

struct ProductionStats {
    int manufacturies;
    int delivered;
    pthread_mutex_t lock;
    
    ProductionStats() : manufacturies(0), delivered(0) {
        pthread_mutex_init(&lock, nullptr);
    }
} metrics;

void* factory_worker(void* worker_data) {
    int factory_id = *static_cast<int*>(worker_data);
    int items_per_factory = TOTAL_ITEMS / FACTORY_COUNT;
    
    auto start_time = std::chrono::steady_clock::now();
    
    for (int batch = 0; batch < items_per_factory; batch++) {
        int product_value = (factory_id * 1000) + batch + (rand() % 50);
        
        sem_wait(&space_available);
        pthread_mutex_lock(&storage_guard);
        
        warehouse.items[warehouse.write_idx] = product_value;
        std::cout << "[Factory-" << factory_id << "] Created product #" 
                  << product_value << " -> slot[" << warehouse.write_idx << "]\n";
        
        warehouse.write_idx = (warehouse.write_idx + 1) % STORAGE_CAPACITY;
        
        pthread_mutex_lock(&metrics.lock);
        metrics.manufacturies++;
        pthread_mutex_unlock(&metrics.lock);
        
        pthread_mutex_unlock(&storage_guard);
        sem_post(&items_ready);
        
        int delay = 50000 + (rand() % 75000);
        usleep(delay);
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Factory-" << factory_id << " completed production in " << duration.count() << "ms\n";
    
    return nullptr;
}

void* customer_service(void* customer_data) {
    int customer_id = *static_cast<int*>(customer_data);
    int orders_to_fulfill = TOTAL_ITEMS / CUSTOMER_COUNT;
    int processed_orders = 0;
    
    while (processed_orders < orders_to_fulfill) {
        sem_wait(&items_ready);
        pthread_mutex_lock(&storage_guard);
        
        int received_product = warehouse.items[warehouse.read_idx];
        warehouse.read_idx = (warehouse.read_idx + 1) % STORAGE_CAPACITY;
        
        pthread_mutex_lock(&metrics.lock);
        metrics.delivered++;
        processed_orders++;
        pthread_mutex_unlock(&metrics.lock);
        
        pthread_mutex_unlock(&storage_guard);
        sem_post(&space_available);
        
        std::cout << "[Customer-" << customer_id << "] Received product #" 
                  << received_product << " (Order " << processed_orders << "/" 
                  << orders_to_fulfill << ")\n";
        
        int processing_time = 80000 + (rand() % 120000);
        usleep(processing_time);
    }
    
    std::cout << "Customer " << customer_id << " fulfilled all orders successfully!\n";
    return nullptr;
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)) ^ getpid());
    
    sem_init(&space_available, 0, STORAGE_CAPACITY);
    sem_init(&items_ready, 0, 0);
    pthread_mutex_init(&storage_guard, nullptr);
    
    pthread_t factory_threads[FACTORY_COUNT];
    pthread_t customer_threads[CUSTOMER_COUNT];
    int factory_ids[FACTORY_COUNT];
    int customer_ids[CUSTOMER_COUNT];
    
    std::cout << "Manufactring & Distrubution System" << std::endl;
    std::cout << "Warehouse Capacity: " << STORAGE_CAPACITY << std::endl;
    std::cout << "Active Factories: " << FACTORY_COUNT << std::endl;
    std::cout << "Customer Services " << CUSTOMER_COUNT << std::endl;
    std::cout << "Target Production: " << TOTAL_ITEMS << " unit" << std::endl << std::endl;
    
    auto system_start = std::chrono::steady_clock::now();
    
    for (int f = 0; f < FACTORY_COUNT; f++) {
        factory_ids[f] = f + 1;
        pthread_create(&factory_threads[f], nullptr, factory_worker, &factory_ids[f]);
    }
    
    for (int c = 0; c < CUSTOMER_COUNT; c++) {
        customer_ids[c] = c + 1;
        pthread_create(&customer_threads[c], nullptr, customer_service, &customer_ids[c]);
    }
    
    for (int f = 0; f < FACTORY_COUNT; f++) {
        pthread_join(factory_threads[f], nullptr);
    }
    
    for (int c = 0; c < CUSTOMER_COUNT; c++) {
        pthread_join(customer_threads[c], nullptr);
    }
    
    auto system_end = std::chrono::steady_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(system_end - system_start);
    
    std::cout << std::endl << "=== SYSTEM PERFORMANCE REPORT ===" << std::endl;
    std::cout << "Manufacturing Completed: " << metrics.manufacturies << " units" << std::endl;
    std::cout << "Orders Delivered: " << metrics.delivered << " units" << std::endl;
    std::cout << "System Runtime: " << total_duration.count() << " milliseconds" << std::endl;
    std::cout << "Efficiency Rate: " << (metrics.delivered * 100.0 / TOTAL_ITEMS) << "%" << std::endl;
    
    sem_destroy(&space_available);
    sem_destroy(&items_ready);
    pthread_mutex_destroy(&storage_guard);
    pthread_mutex_destroy(&metrics.lock);
    
    return 0;
}