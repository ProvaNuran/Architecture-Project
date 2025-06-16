#include <stdio.h>
#include <stdlib.h>

#include <time.h>
#include <stdint.h>

#define NUM_CORES 4          
#define NUM_SETS 8           
#define NUM_WAYS 8           
#define CACHE_LINE_SIZE 64   
#define ITERATIONS 10000000  
#define BATCH_SIZE 10        


typedef struct {
    int valid;
    unsigned int tag;
    int LRU_counter;
} CacheLine;


CacheLine L3_cache[NUM_SETS][NUM_WAYS];


int partition[NUM_CORES] = {2, 2, 2, 2};


int hits[NUM_CORES] = {0};
int misses[NUM_CORES] = {0};


pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;


void* core_function(void* arg) {
    int core_id = (int)(uintptr_t)arg;
    int ways_start = core_id * partition[core_id];
    int ways_end = ways_start + partition[core_id];

    int local_hits = 0;
    int local_misses = 0;

    for (int i = 0; i < ITERATIONS / NUM_CORES; i += BATCH_SIZE) {
        for (int b = 0; b < BATCH_SIZE; b++) {
            unsigned int address = rand() % 1024;
            unsigned int tag = address / NUM_SETS;
            int set_index = address % NUM_SETS;

            int hit = 0;

            pthread_mutex_lock(&cache_mutex);
            
            for (int way = ways_start; way < ways_end; way++) {
                if (L3_cache[set_index][way].valid &&
                    L3_cache[set_index][way].tag == tag) {
                    local_hits++;
                    L3_cache[set_index][way].LRU_counter = 0;
                    hit = 1;
                    break;
                }
            }

            
            if (!hit) {
                local_misses++;
                int replace_way = -1;
                int max_LRU = -1;

                for (int way = ways_start; way < ways_end; way++) {
                    if (!L3_cache[set_index][way].valid) {
                        replace_way = way;
                        break;
                    }
                    if (L3_cache[set_index][way].LRU_counter > max_LRU) {
                        max_LRU = L3_cache[set_index][way].LRU_counter;
                        replace_way = way;
                    }
                }

                L3_cache[set_index][replace_way].valid = 1;
                L3_cache[set_index][replace_way].tag = tag;
                L3_cache[set_index][replace_way].LRU_counter = 0;
            }

            
            for (int way = 0; way < NUM_WAYS; way++) {
                if (L3_cache[set_index][way].valid) {
                    L3_cache[set_index][way].LRU_counter++;
                }
            }
            pthread_mutex_unlock(&cache_mutex);
        }
    }

    
    pthread_mutex_lock(&cache_mutex);
    hits[core_id] += local_hits;
    misses[core_id] += local_misses;
    pthread_mutex_unlock(&cache_mutex);

    return NULL;
}

int main() {
    pthread_t threads[NUM_CORES];

    
    for (int i = 0; i < NUM_SETS; i++) {
        for (int j = 0; j < NUM_WAYS; j++) {
            L3_cache[i][j].valid = 0;
            L3_cache[i][j].tag = 0;
            L3_cache[i][j].LRU_counter = 0;
        }
    }

    srand(time(NULL));

    
    for (int i = 0; i < NUM_CORES; i++) {
        pthread_create(&threads[i], NULL, core_function, (void*)(uintptr_t)i);
    }

    
    for (int i = 0; i < NUM_CORES; i++) {
        pthread_join(threads[i], NULL);
    }

    
    int total_hits = 0, total_misses = 0;
    for (int i = 0; i < NUM_CORES; i++) {
        total_hits += hits[i];
        total_misses += misses[i];
        printf("Core %d: Hits = %d, Misses = %d, Hit Rate = %.2f%%\n",
               i, hits[i], misses[i],
               (double)hits[i] / (hits[i] + misses[i]) * 100);
    }

    printf("Total: Hits = %d, Misses = %d, Overall Hit Rate = %.2f%%\n",
           total_hits, total_misses,
           (double)total_hits / (total_hits + total_misses) * 100);

    return 0;
}


