///////////////////////////////////////////////////////////////////////////////
// You will need to modify this file to implement part A.                    //
///////////////////////////////////////////////////////////////////////////////

// cache.h
// Contains declarations of data structures and functions used to implement a
// cache.

#ifndef __CACHE_H__
#define __CACHE_H__

#include "types.h"
// You may add any other #include directives you need here, but make sure they
// compile on the reference machine!

///////////////////////////////////////////////////////////////////////////////
//                                 CONSTANTS                                 //
///////////////////////////////////////////////////////////////////////////////

/**
 * The maximum allowed number of ways in a cache set.
 * 
 * At runtime, the actual number of ways in each cache set is guaranteed to be
 * less than or equal to this value.
 */
#define MAX_WAYS_PER_CACHE_SET 16

///////////////////////////////////////////////////////////////////////////////
//                              DATA STRUCTURES                              //
///////////////////////////////////////////////////////////////////////////////

/** Possible replacement policies for the cache. */
typedef enum ReplacementPolicyEnum
{
    LRU = 0,    // Evict the least recently used line.
    RANDOM = 1, // Evict a random line.

    /**
     * Evict according to a static way partitioning policy.
     * Part E asks you to implement this policy for extra credit.
     */
    SWP = 2,

    /**
     * Evict according to a dynamic way partitioning policy.
     * Part F asks you to implement this policy for extra credit.
     */
    DWP = 3,
} ReplacementPolicy;

// TODO: Define any other data structures you need here.
// Refer to Appendix A for details on data structures you will need here.
typedef struct CacheLine {
    /**
     * Denotes if the cache line is indeed present in the Cache
     */
    bool valid;

    /*
    * Denotes if the latest data value is present only in the local Cache copy
    */
    bool dirty;

    /*
    * Denotes the conventional higher order PC bits beyond index
    */
    unsigned long long tag;

    /*
    * This shall be needed to identify the core to which a cache line (way) is
    * assigned to in a multicore scenario
    */
    uint8_t core_id;

    /*
    * Helps to keep track of individual last access times of the
    * cache lines, which helps identify the LRU way
    */
    uint64_t LAT;
} CacheLine;

typedef struct CacheSet {

    // Pointer to the allocated space (could be stack space too?)
    // replicated “# of Ways” times, as in an array/list
    // Pointer to list of pointers
    CacheLine *lines;
} CacheSet;

/** A single cache module. */
typedef struct Cache
{
    // TODO: Define any other fields you need here.
    // Refer to Appendix A for details on other fields you will need here.

    // (replicated “# of Sets” times, as in an array/list)
    // Pointer to allocated space pointer to list of pointers
    CacheSet *sets;

    uint8_t nof_ways;

    ReplacementPolicyEnum  rpl_pol;

    uint16_t nof_sets;

    // Last evicted line
    // To be passed on the next higher cache hierarchy
    // for an install if necessary
    CacheLine LEL;

    /**
     * The total number of times this cache was accessed for a read.
     * You should initialize this to 0 and update it for every read!
     */
    unsigned long long stat_read_access;

    /**
     * The total number of read accesses that missed this cache.
     * You should initialize this to 0 and update it for every read miss!
     */
    unsigned long long stat_read_miss;

    /**
     * The total number of times this cache was accessed for a write.
     * You should initialize this to 0 and update it for every write!
     */
    unsigned long long stat_write_access;

    /**
     * The total number of write accesses that missed this cache.
     * You should initialize this to 0 and update it for every write miss!
     */
    unsigned long long stat_write_miss;

    /**
     * The total number of times a dirty line was evicted from this cache.
     * You should initialize this to 0 and update it for every dirty eviction!
     */
    unsigned long long stat_dirty_evicts;
} Cache;

/** Holds the tag and index for a Cache Line candidate */
typedef struct CacheLocStats {

    /**
     * Tag, indicates which set the line is in
     */
    uint64_t tag;

    /**
     * Index, indicates which way the line is in
     */
    uint64_t index;
} CacheLocStats;

/** Whether a cache access is a hit or a miss. */
typedef enum CacheResultEnum
{
    HIT = 1,  // The access hit the cache.
    MISS = 0, // The access missed the cache.
} CacheResult;



///////////////////////////////////////////////////////////////////////////////
//                            FUNCTION PROTOTYPES                            //
///////////////////////////////////////////////////////////////////////////////

// Please note:
// Implementing the following functions as described will be useful in
// completing the lab.

// However, if you would like to deviate from the suggested implementation,
// you are free to do so by adding, removing, or modifying declarations as you
// see fit.

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
                 ReplacementPolicy replacement_policy);

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
                         unsigned int core_id);

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
                   unsigned int core_id);

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
                               unsigned int core_id);

/**
 * Print the statistics of the given cache.
 * 
 * This is implemented for you. You must not modify its output format.
 * 
 * @param c The cache to print the statistics of.
 * @param label A label for the cache, which is used as a prefix for each
 *              statistic.
 */
void cache_print_stats(Cache *c, const char *label);

/**
 * Gets the tag and index of a given address
 * 
 * 
 * 
 */
CacheLocStats findTagAngIndex(Cache *c, uint64_t line_addr);

#endif // __CACHE_H__
