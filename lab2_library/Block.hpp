#ifndef BLOCK_HPP
#define BLOCK_HPP
#include <new>
#include <stdlib.h>
#include <sys/types.h>

#include "constants.hpp"

struct Block {
    fd_t fd;
    off_t offset;
    size_t size;
    bool reference;
    bool dirty;
    bool valid;
    char* data = nullptr;

    Block(fd_t fd, off_t offset, size_t size):
        fd(fd), offset(offset), size(size), reference(false), dirty(false), valid(false) {
        if (posix_memalign(reinterpret_cast<void**>(&data), LAB2_BLOCK_SIZE, size) != 0) {
            throw std::bad_alloc();
        }
    }

    ~Block() {
        if (data) {
            free(data);
            data = nullptr;
        }
    }

    // Delete copy constructor and copy assignment operator
    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;

    // Allow move constructor
    Block(Block&& other) noexcept:
        fd(other.fd), offset(other.offset), size(other.size),
        reference(other.reference), dirty(other.dirty), valid(other.valid), data(other.data) {
        other.data = nullptr;  // Transfer ownership of data
    }

    // Allow move assignment operator
    Block& operator=(Block&& other) noexcept {
        if (this != &other) {
            free(data);  // Free existing data
            fd = other.fd;
            offset = other.offset;
            size = other.size;
            reference = other.reference;
            dirty = other.dirty;
            valid = other.valid;
            data = other.data;
            other.data = nullptr;  // Transfer ownership of data
        }
        return *this;
    }
};

#endif //BLOCK_HPP