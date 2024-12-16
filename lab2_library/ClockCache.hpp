//
// Created by vad1mchk on 16/12/24.
//

#ifndef CLOCKCACHE_HPP
#define CLOCKCACHE_HPP

#include <vector>
#include <sys/types.h>

#include "CachePage.hpp"
#include "types.hpp"

class ClockCache {
public:
    ClockCache(size_t num_pages, size_t page_size);
    ~ClockCache();

    ClockCache(const ClockCache&) = delete;
    ClockCache& operator=(const ClockCache&) = delete;

    // Move if necessary:
    ClockCache(ClockCache&&) = default;
    ClockCache& operator=(ClockCache&&) = default;

    char* get_page(int fd, off_t offset, bool for_write);
    void mark_page_dirty(off_t offset);
    void flush_all(int fd);

private:
    size_t page_size;
    size_t clock_hand;
    std::vector<CachePage> pages;

    void evict_page();
    void load_page(fd_t fd, off_t offset);
};

#endif //CLOCKCACHE_HPP
