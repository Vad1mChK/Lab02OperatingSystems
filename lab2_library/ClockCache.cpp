//
// Created by vad1mchk on 15/12/24.
//

#include "ClockCache.hpp"

#include <iostream>
#include <cstring>
#include <unistd.h>

ClockCache::ClockCache(size_t num_pages, size_t page_size)
    : page_size(page_size), clock_hand(0) {
    // Initialize the cache pages
    pages.resize(num_pages);
    for (auto& page : pages) {
        page.valid = false;
        page.dirty = false;
        page.reference = false;
        page.file_offset = -1;  // Invalid offset
        posix_memalign(reinterpret_cast<void**>(&page.data), page_size, page_size);
    }
}

ClockCache::~ClockCache() {
    for (auto& page : pages) {
        free(page.data);
    }
}

// Evict a page using the Clock Algorithm
void ClockCache::evict_page() {
    while (true) {
        CachePage& page = pages[clock_hand];

        if (!page.reference) { // If the page's reference bit is 0, evict it
            if (page.dirty) {
                // Simulate writing the page back to disk
                ssize_t written = pwrite(page.fd, page.data, page_size, page.file_offset);
                if (written < 0) {
                    perror("Error writing dirty page to disk");
                } else {
                    std::cout << "Wrote dirty page at offset " << page.file_offset << " to disk\n";
                }
                std::cout << "Writing dirty page at offset " << page.file_offset << " to disk\n";
                page.dirty = false;
            }

            // Mark the page as invalid
            std::cout << "Evicting page at offset " << page.file_offset << "\n";
            page.valid = false;
            page.file_offset = -1;
            return;
        }

        // Otherwise, clear the reference bit and move the clock hand
        page.reference = false;
        clock_hand = (clock_hand + 1) % pages.size();
    }
}

// Get a page by file offset, loading it if necessary
char* ClockCache::get_page(fd_t fd, off_t file_offset) {
    // Check if the page is already in the cache
    for (auto& page : pages) {
        if (page.valid && page.file_offset == file_offset && page.fd == fd) {
            page.reference = true; // Mark the page as recently accessed
            return page.data;
        }
    }

    // If the page is not in the cache, evict a page and load the new one
    evict_page();

    // Use the current clock hand position to load the new page
    CachePage& new_page = pages[clock_hand];
    new_page.file_offset = file_offset;
    new_page.fd = fd;
    new_page.valid = true;
    new_page.reference = true;
    new_page.dirty = false;

    // Simulate reading the page from disk
    std::cout << "Loading page at offset " << file_offset << " from disk\n";
    ssize_t read_bytes = pread(fd, new_page.data, page_size, file_offset);
    if (read_bytes < 0) {
        perror("Error reading page from disk");
        std::memset(new_page.data, 0, page_size); // Clear the page on failure
    }
    clock_hand = (clock_hand + 1) % pages.size();
    return new_page.data;
}

void ClockCache::mark_page_dirty(off_t file_offset) {
    for (auto& page : pages) {
        if (page.valid && page.file_offset == file_offset) {
            page.dirty = true;
            return;
        }
    }
    throw std::runtime_error("Page not found in cache");
}

void ClockCache::invoke_for_each_dirty_page(const std::function<void(CachePage &)> &callback) {
    for (auto& page : pages) {
        if (page.valid && page.dirty) {
            callback(page);
        }
    }
}
