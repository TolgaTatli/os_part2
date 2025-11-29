#include <iostream>
#include <omp.h>
#include <unistd.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <queue>
#include <chrono>
#include <memory>

const int INVENTORY_LIMIT = 12;
const int ASSEMBLY_LINES = 3;
const int DISPATCH_TEAMS = 2;
const int PRODUCTION_TARGET = 30;

class ThreadSafeWarehouse {
private:
    std::queue<int> stock_queue;
    int capacity_limit;
    omp_lock_t warehouse_access;
    
public:
    ThreadSafeWarehouse(int max_capacity) : capacity_limit(max_capacity) {
        omp_init_lock(&warehouse_access);
    }
    
    ~ThreadSafeWarehouse() {
        omp_destroy_lock(&warehouse_access);
    }
    
    bool store_item(int product_code, int assembly_line_id) {
        omp_set_lock(&warehouse_access);
        
        if (static_cast<int>(stock_queue.size()) < capacity_limit) {
            stock_queue.push(product_code);
            std::cout << "[Assembly-Line-" << assembly_line_id 
                      << "] Stored item #" << product_code 
                      << " | Inventory: " << stock_queue.size() 
                      << "/" << capacity_limit << std::endl;
            omp_unset_lock(&warehouse_access);
            return true;
        }
        
        omp_unset_lock(&warehouse_access);
        return false;
    }
    
    bool retrieve_item(int& product_code, int dispatch_team_id) {
        omp_set_lock(&warehouse_access);
        
        if (!stock_queue.empty()) {
            product_code = stock_queue.front();
            stock_queue.pop();
            std::cout << "[Dispatch-Team-" << dispatch_team_id 
                      << "] Retrieved item #" << product_code 
                      << " | Remaining: " << stock_queue.size() << std::endl;
            omp_unset_lock(&warehouse_access);
            return true;
        }
        
        omp_unset_lock(&warehouse_access);
        return false;
    }
    
    bool has_inventory() {
        omp_set_lock(&warehouse_access);
        bool status = !stock_queue.empty();
        omp_unset_lock(&warehouse_access);
        return status;
    }
    
    int current_stock_level() {
        omp_set_lock(&warehouse_access);
        int level = static_cast<int>(stock_queue.size());
        omp_unset_lock(&warehouse_access);
        return level;
    }
};

ThreadSafeWarehouse central_warehouse(INVENTORY_LIMIT);
int items_assembled = 0;
int items_dispatched = 0;
bool assembly_completed = false;
omp_lock_t metrics_guard;

void assembly_line_operation(int line_id) {
    int units_per_line = PRODUCTION_TARGET / ASSEMBLY_LINES;
    int completed_units = 0;
    
    auto line_start_time = std::chrono::steady_clock::now();
    
    while (completed_units < units_per_line) {
        int product_serial = (line_id * 10000) + completed_units + (rand() % 999);
        
        if (central_warehouse.store_item(product_serial, line_id)) {
            omp_set_lock(&metrics_guard);
            items_assembled++;
            completed_units++;
            omp_unset_lock(&metrics_guard);
            
            int assembly_duration = 60000 + (rand() % 80000);
            usleep(assembly_duration);
        } else {
            usleep(15000);
        }
    }
    
    auto line_end_time = std::chrono::steady_clock::now();
    auto operation_time = std::chrono::duration_cast<std::chrono::milliseconds>(line_end_time - line_start_time);
    std::cout << "Assembly-Line-" << line_id << " finished in " << operation_time.count() << "ms" << std::endl;
}

void dispatch_team_operation(int team_id) {
    int shipments_per_team = PRODUCTION_TARGET / DISPATCH_TEAMS;
    int completed_shipments = 0;
    
    while (completed_shipments < shipments_per_team) {
        int retrieved_item;
        
        if (central_warehouse.retrieve_item(retrieved_item, team_id)) {
            omp_set_lock(&metrics_guard);
            items_dispatched++;
            completed_shipments++;
            omp_unset_lock(&metrics_guard);
            
            int dispatch_duration = 90000 + (rand() % 110000);
            usleep(dispatch_duration);
        } else {
            usleep(20000);
        }
    }
    
    std::cout << "Dispatch-Team-" << team_id << " completed all shipments!" << std::endl;
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)) + omp_get_thread_num());
    omp_init_lock(&metrics_guard);
    
    std::cout << "=== Industrial Assembly & Dispatch Center ===" << std::endl;
    std::cout << "Warehouse Capacity: " << INVENTORY_LIMIT << " units" << std::endl;
    std::cout << "Assembly Lines: " << ASSEMBLY_LINES << std::endl;
    std::cout << "Dispatch Teams: " << DISPATCH_TEAMS << std::endl;
    std::cout << "Production Goal: " << PRODUCTION_TARGET << " units" << std::endl << std::endl;
    
    omp_set_num_threads(ASSEMBLY_LINES + DISPATCH_TEAMS);
    
    auto system_start_time = std::chrono::steady_clock::now();
    
    #pragma omp parallel
    {
        int worker_id = omp_get_thread_num();
        
        if (worker_id < ASSEMBLY_LINES) {
            assembly_line_operation(worker_id + 1);
        } else {
            dispatch_team_operation(worker_id - ASSEMBLY_LINES + 1);
        }
    }
    
    auto system_end_time = std::chrono::steady_clock::now();
    auto total_runtime = std::chrono::duration_cast<std::chrono::milliseconds>(system_end_time - system_start_time);
    
    std::cout << std::endl << "OPERATION SUMMARY" << std::endl;
    std::cout << "Items Assembled: " << items_assembled << " units" << std::endl;
    std::cout << "Items Dispatched: " << items_dispatched << " units" << std::endl;
    std::cout << "Total Operation Time: " << total_runtime.count() << " milliseconds" << std::endl;
    std::cout << "Final Warehouse Stock: " << central_warehouse.current_stock_level() << " units" << std::endl;
    std::cout << "System Efficiency: " << (items_dispatched * 100.0 / PRODUCTION_TARGET) << "%" << std::endl;
    
    omp_destroy_lock(&metrics_guard);
    
    return 0;
}