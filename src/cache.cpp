///////////////////////////////////////////////////////////////////////////////
// You will need to modify this file to implement part A and, for extra      //
// credit, parts E and F.                                                    //
///////////////////////////////////////////////////////////////////////////////

// cache.cpp
// Defines the functions used to implement the cache.

#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// You may add any other #include directives you need here, but make sure they
// compile on the reference machine!

///////////////////////////////////////////////////////////////////////////////
//                    EXTERNALLY DEFINED GLOBAL VARIABLES                    //
///////////////////////////////////////////////////////////////////////////////

/**
 * The current clock cycle number.
 * 
 * This can be used as a timestamp for implementing the LRU replacement policy.
 */
extern uint64_t current_cycle;

/**
 * For static way partitioning, the quota of ways in each set that can be
 * assigned to core 0.
 * 
 * The remaining number of ways is the quota for core 1.
 * 
 * This is used to implement extra credit part E.
 */
extern unsigned int SWP_CORE0_WAYS;

extern uint64_t CACHE_LINESIZE;

extern unsigned int NUM_CORES;

///////////////////////////////////////////////////////////////////////////////
//                           FUNCTION DEFINITIONS                            //
///////////////////////////////////////////////////////////////////////////////

// As described in cache.h, you are free to deviate from the suggested
// implementation as you see fit.

// The only restriction is that you must not remove cache_print_stats() or
// modify its output format, since its output will be used for grading.

/**
 * Allocate and initialize a cache.
 * 
 * This is intended to be implemented in part A.
 *
 * @param size The size of the cache in bytes.
 * @param associativity The associativity of the cache.
 * @param line_size The size of a cache line in bytes.
 * @param replacement_policy The replacement policy of the cache.
 * @return A pointer to the cache.
 */
Cache *cache_new(uint64_t size, uint64_t associativity, uint64_t line_size,
                 ReplacementPolicy replacement_policy)
{
    // TODO: Allocate memory to the data structures and initialize the required
    //       fields. (You might want to use calloc() for this.)
    /////////////
    // Useful identities:
    //// nof_lines = size/CACHE_LINESIZE;
    //// nof_sets = nof_lines/associativity

    Cache *newCache = (Cache *)malloc(sizeof(Cache));

    newCache->nof_ways = associativity;
    newCache->rpl_pol = replacement_policy;
    newCache->nof_sets = (size / CACHE_LINESIZE) / associativity;
    newCache->stat_read_access = 0;
    newCache->stat_read_miss = 0;
    newCache->stat_write_access = 0;
    newCache->stat_write_miss = 0;
    newCache->stat_dirty_evicts = 0;
    #ifdef DEBUG
        printf("Creating cache (# sets: %d, # ways: %d)\n", newCache->nof_sets, newCache->nof_ways);
    #endif
    newCache->sets = (CacheSet *)calloc(newCache->nof_sets, sizeof(CacheSet));

    for (int16_t i = 0; i <= newCache->nof_sets - 1; i++) {
        newCache->sets[i].lines = (CacheLine *)calloc(newCache->nof_ways, sizeof(CacheLine));
    }

    return (newCache);
}

/**
 * Access the cache at the given address.
 * 
 * Also update the cache statistics accordingly.
 * 
 * This is intended to be implemented in part A.
 * 
 * @param c The cache to access.
 * @param line_addr The address of the cache line to access (in units of the
 *                  cache line size, i.e., excluding the line offset bits).
 * @param is_write Whether this access is a write.
 * @param core_id The CPU core ID that requested this access.
 * @return Whether the cache access was a hit or a miss.
 */
CacheResult cache_access(Cache *c, uint64_t line_addr, bool is_write,
                         unsigned int core_id)
{
    // TODO: Return HIT if the access hits in the cache, and MISS otherwise.
    // TODO: If is_write is true, mark the resident line as dirty.
    // TODO: Update the appropriate cache statistics.

    CacheLocStats lineStats = findTagAngIndex(c, line_addr);
    CacheLine tempLine;
    uint8_t offset;

    for (uint8_t i = 0; i < c->nof_ways; i++) {
        if (c->sets[lineStats.index].lines[i].tag == lineStats.tag) {
            tempLine = c->sets[lineStats.index].lines[i];
            offset = i;
        } else {
            tempLine.dirty = true;
            tempLine.valid = false;
            offset = 0;
        }
    }

    #ifdef DEBUG
        printf("\t\tindex: %ld, tag: %ld, is_write: %d, core_id: %d\n", lineStats.index, lineStats.tag, is_write, core_id);
    #endif

    if (tempLine.valid) {
        if (is_write) {
            c->sets[lineStats.index].lines[offset].dirty = true;
            c->stat_write_access++;
        } else {
            c->stat_read_access++;
        }
        c->sets[lineStats.index].lines[offset].LAT = current_cycle;

        #ifdef DEBUG
            printf("\t\tHit in the cache --> is_write: %d\n", is_write);
        #endif

        return HIT;
    } else {
        if (is_write) {
            c->stat_write_access++;
            c->stat_write_miss++;
        } else {
            c->stat_read_access++;
            c->stat_read_miss++;
        }

        #ifdef DEBUG
            printf("\t\tMISS!\n");
        #endif

        return MISS;
    }
    

}

/**
 * Install the cache line with the given address.
 * 
 * Also update the cache statistics accordingly.
 * 
 * This is intended to be implemented in part A.
 * 
 * @param c The cache to install the line into.
 * @param line_addr The address of the cache line to install (in units of the
 *                  cache line size, i.e., excluding the line offset bits).
 * @param is_write Whether this install is triggered by a write.
 * @param core_id The CPU core ID that requested this access.
 */
void cache_install(Cache *c, uint64_t line_addr, bool is_write,
                   unsigned int core_id)
{
    // TODO: Use cache_find_victim() to determine the victim line to evict.
    // TODO: Copy it into a last_evicted_line field in the cache in order to
    //       track writebacks.
    // TODO: Initialize the victim entry with the line to install.
    // TODO: Update the appropriate cache statistics.
    CacheLocStats lineStats = findTagAngIndex(c, line_addr);

    #ifdef DEBUG
        printf("\t\tInstalling into a cache line addr: %ld(index: %ld)\n", line_addr, lineStats.index);
    #endif


    uint64_t coreReplacement = cache_find_victim(c, lineStats.index, core_id);


    if (c->sets[lineStats.index].lines[coreReplacement].valid) {
        if (c->sets[lineStats.index].lines[coreReplacement].dirty & c->sets[lineStats.index].lines[coreReplacement].valid) {
            c->LEL = c->sets[lineStats.index].lines[coreReplacement];
            #ifdef DEBUG
                printf("\t\tVictim was dirty!\n");
            #endif
        }
        #ifdef DEBUG
            if (NUM_CORES > 1) {
                printf("\t\tCore 1 LRU index: %ld, Core 2 LRU index: %ld\n", coreReplacement, coreReplacement);
            }
            
            printf("\t\tFound a victim (policy_num: %d, idx: %ld)\n", c->rpl_pol, coreReplacement);
        #endif
    }

    c->sets[lineStats.index].lines[coreReplacement].valid = true;
    c->sets[lineStats.index].lines[coreReplacement].core_id = core_id;
    c->sets[lineStats.index].lines[coreReplacement].dirty = false;
    c->sets[lineStats.index].lines[coreReplacement].LAT = current_cycle;
    c->sets[lineStats.index].lines[coreReplacement].tag = lineStats.tag;
    
    #ifdef DEBUG
        printf("\t\tNew cache line installed (dirty: %d, tag: %lld, core_id: %d, last_access_time: %ld)\n", 
                    c->sets[lineStats.index].lines[coreReplacement].dirty,
                    c->sets[lineStats.index].lines[coreReplacement].tag,
                    c->sets[lineStats.index].lines[coreReplacement].core_id,
                    c->sets[lineStats.index].lines[coreReplacement].LAT);
        
    #endif

}

/**
 * Find which way in a given cache set to replace when a new cache line needs
 * to be installed. This should be chosen according to the cache's replacement
 * policy.
 * 
 * The returned victim can be valid (non-empty), in which case the calling
 * function is responsible for evicting the cache line from that victim way.
 * 
 * This is intended to be initially implemented in part A and, for extra
 * credit, extended in parts E and F.
 * 
 * @param c The cache to search.
 * @param set_index The index of the cache set to search.
 * @param core_id The CPU core ID that requested this access.
 * @return The index of the victim way.
 */
unsigned int cache_find_victim(Cache *c, unsigned int set_index,
                               unsigned int core_id)
{
    // TODO: Find a victim way in the given cache set according to the cache's
    //       replacement policy.
    // TODO: In part A, implement the LRU and random replacement policies.
    // TODO: In part E, for extra credit, implement static way partitioning.
    // TODO: In part F, for extra credit, implement dynamic way partitioning.

    #ifdef DEBUG
        printf("\t\tLooking for victim to evict (policy: %d)...\n", c->rpl_pol);
    #endif


    if (c->rpl_pol == LRU) {
        unsigned int least_recent = 999;
        uint64_t cycle_accessed = 0xFFFFFFFFFFFFFFFF;
        for (u_int8_t i = 0; i < c->nof_ways; i++) {
            if (!c->sets[set_index].lines[i].valid) {
                #ifdef DEBUG
                    printf("\t\tFound a naive victim (valid bit not set, idx: %d)\n", i);
                #endif
                // Return an invalid spot if possible
                least_recent = i;
                cycle_accessed = 0;
                return least_recent;
            } else if (c->sets[set_index].lines[i].LAT < cycle_accessed) {
                // Else grab the least recently used
                least_recent = i;
                cycle_accessed = c->sets[set_index].lines[i].LAT;
            }
        }
        if (least_recent == 999) {
            return least_recent;
        }
        if (c->sets[set_index].lines[least_recent].dirty) {
            c->stat_dirty_evicts++;
        }
        return least_recent;

    } else if (c->rpl_pol == RANDOM) {
        return 0;
    }

    return 0;
}

/**
 * Print the statistics of the given cache.
 * 
 * This is implemented for you. You must not modify its output format.
 * 
 * @param c The cache to print the statistics of.
 * @param label A label for the cache, which is used as a prefix for each
 *              statistic.
 */
void cache_print_stats(Cache *c, const char *header)
{
    double read_miss_percent = 0.0;
    double write_miss_percent = 0.0;

    if (c->stat_read_access)
    {
        read_miss_percent = 100.0 * (double)(c->stat_read_miss) /
                            (double)(c->stat_read_access);
    }

    if (c->stat_write_access)
    {
        write_miss_percent = 100.0 * (double)(c->stat_write_miss) /
                             (double)(c->stat_write_access);
    }

    printf("\n");
    printf("%s_READ_ACCESS     \t\t : %10llu\n", header, c->stat_read_access);
    printf("%s_WRITE_ACCESS    \t\t : %10llu\n", header, c->stat_write_access);
    printf("%s_READ_MISS       \t\t : %10llu\n", header, c->stat_read_miss);
    printf("%s_WRITE_MISS      \t\t : %10llu\n", header, c->stat_write_miss);
    printf("%s_READ_MISS_PERC  \t\t : %10.3f\n", header, read_miss_percent);
    printf("%s_WRITE_MISS_PERC \t\t : %10.3f\n", header, write_miss_percent);
    printf("%s_DIRTY_EVICTS    \t\t : %10llu\n", header, c->stat_dirty_evicts);
}


CacheLocStats findTagAngIndex(Cache *c, uint64_t line_addr) {
    CacheLocStats lineStats;
    uint64_t index_mask = (1 << (u_int64_t)log2(c->nof_ways)) - 1;

    lineStats.index = line_addr & index_mask;
    lineStats.tag = line_addr >> (u_int64_t)log2(c->nof_sets);

    return lineStats;
}