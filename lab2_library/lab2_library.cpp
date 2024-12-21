#include "lab2_library.hpp"

#include "constants.hpp"
#include "todo.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>

#include "Block.hpp"
#include "BlockCache.hpp"

BlockCache* cache;

fd_t lab2_open(const std::string &path) {
    fd_t fd = ::open(path.c_str(), O_RDWR | O_DIRECT | O_CREAT, 0644);
    if (fd < 0) {
        perror("open");
        throw std::runtime_error("Failed to open file");
    }

    cache = new BlockCache(fd, LAB2_BLOCK_COUNT, LAB2_BLOCK_SIZE);

    if (cache == nullptr) {
        throw std::runtime_error("Could not create block cache... Dear god.");
    }

    if (cache->get_block_size() == 0) {
        throw std::runtime_error("Block size is zero in lab2_open");
    }

    return fd;
}

ssize_t lab2_read(fd_t fd, void *buf, size_t count) {
    char* out_ptr = static_cast<char*>(buf);
    ssize_t total_read = 0;
    size_t block_size = cache->get_block_size();
    off_t position = cache->get_position();

    if (block_size == 0) {
        throw std::runtime_error("Block size is zero in lab2_read");
    }

    while (count > 0) {
        off_t block_offset = (position / block_size) * block_size;
        size_t offset_in_block = position % block_size;
        size_t to_read = std::min(block_size - offset_in_block, count);

        Block* block = cache->get_block(block_offset, false); // Read block
        std::memcpy(out_ptr, block->data + offset_in_block, to_read);

        out_ptr += to_read;
        count -= to_read;
        total_read += to_read;
        position += to_read;
    }

    cache->set_position(position); // Update position in cache
    return total_read;
}

ssize_t lab2_write(fd_t fd, const void *buf, size_t count) {
    const char* in_ptr = static_cast<const char*>(buf);
    ssize_t total_written = 0;
    size_t block_size = cache->get_block_size();
    off_t position = cache->get_position();

    if (block_size == 0) {
        throw std::runtime_error("Block size is zero in lab2_write");
    }

    while (count > 0) {
        off_t block_offset = (position / block_size) * block_size;
        size_t offset_in_block = position % block_size;
        size_t to_write = std::min(block_size - offset_in_block, count);

        Block* block = cache->get_block(block_offset, true); // Write block
        std::memcpy(block->data + offset_in_block, in_ptr, to_write);
        block->dirty = true; // Mark block as dirty

        in_ptr += to_write;
        count -= to_write;
        total_written += to_write;
        position += to_write;
    }

    cache->set_position(position); // Update position in cache
    return total_written;
}

off_t lab2_lseek(fd_t fd, off_t offset, int whence) {
    off_t new_position;

    if (whence == SEEK_SET) {
        new_position = offset; // Set absolute position
    } else if (whence == SEEK_CUR) {
        new_position = cache->get_position() + offset; // Adjust relative to current position
    } else if (whence == SEEK_END) {
        off_t file_size = ::lseek(fd, 0, SEEK_END);
        if (file_size < 0) {
            perror("lseek");
            throw std::runtime_error("Failed to get file size");
        }
        new_position = file_size + offset;
    } else {
        throw std::invalid_argument("Invalid whence value for lseek");
    }

    if (new_position < 0) {
        throw std::invalid_argument("Invalid seek position");
    }

    cache->set_position(new_position); // Update position in cache
    return new_position;
}

int lab2_fsync(fd_t fd) {
    try {
        cache->flush_all();
        if (::fsync(fd) != 0) {
            perror("fsync");
            return -1;
        }
        return 0;
    } catch (const std::exception& e) {
        fprintf(stderr, "fsync error: %s\n", e.what());
        return -1;
    }
}

int lab2_close(fd_t fd) {
    try {
        // Ensure all dirty blocks are flushed to disk
        if (cache) {
            cache->flush_all();
            delete cache; // Free the cache memory
            cache = nullptr;
        }

        // Close the file descriptor
        if (::close(fd) != 0) {
            perror("close");
            return -1;
        }

        return 0; // Success
    } catch (const std::exception& e) {
        fprintf(stderr, "close error: %s\n", e.what());
        return -1;
    }
}
