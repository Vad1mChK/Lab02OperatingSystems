//
// Created by vad1mchk on 15/12/24.
//

#ifndef CACHEPAGE_HPP
#define CACHEPAGE_HPP
#include <memory>
#include <sys/types.h>

#include "types.hpp"

// Represents a single page in the cache
struct CachePage {
    off_t file_offset;
    bool valid;
    bool dirty;
    bool reference;
    std::unique_ptr<char, decltype(&free)> data;
    int fd;

    CachePage(size_t page_size)
        : file_offset(-1), valid(false), dirty(false), reference(false), data(nullptr, free), fd(-1) {
        void* ptr;
        // Выравнивание буфера на границу страницы
        if (posix_memalign(&ptr, page_size, page_size) != 0) {
            throw std::bad_alloc();
        }
        data.reset(static_cast<char*>(ptr));
    }
};
#endif //CACHEPAGE_HPP
