//
// Created by vad1mchk on 15/12/24.
//

#ifndef CLOCKCACHE_HPP
#define CLOCKCACHE_HPP
#include <functional>
#include <vector>

#include "CachePage.hpp"

class ClockCache {
private:
    std::vector<CachePage> pages;            // Circular list of cache pages
    size_t page_size;                        // Size of each page
    size_t clock_hand;                       // Current position of the clock hand

public:
    explicit ClockCache(size_t num_pages, size_t page_size);
    ~ClockCache();

    char* get_page(fd_t fd, off_t file_offset);       // Get a page, possibly loading it
    void evict_page();                       // Evict a page using the Clock Algorithm
    void mark_page_dirty(off_t file_offset);
    void invoke_for_each_dirty_page(const std::function<void(CachePage&)>& callback);

    // Delete copy and assignment
    ClockCache(const ClockCache&) = delete;
    ClockCache& operator=(const ClockCache&) = delete;

    // If needed, allow moves
    ClockCache(ClockCache&&) = default;
    ClockCache& operator=(ClockCache&&) = default;
};

#endif //CLOCKCACHE_HPP
