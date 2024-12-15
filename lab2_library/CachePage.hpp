//
// Created by vad1mchk on 15/12/24.
//

#ifndef CACHEPAGE_HPP
#define CACHEPAGE_HPP
#include <sys/types.h>

#include "types.hpp"

// Represents a single page in the cache
struct CachePage {
    fd_t fd;
    off_t file_offset;   // Offset of the page in the file
    bool valid;          // Indicates if the page contains valid data
    bool dirty;          // Indicates if the page has been modified
    bool reference;      // Reference bit (used by the clock algorithm)
    char* data;          // Pointer to the actual page data
};

#endif //CACHEPAGE_HPP
