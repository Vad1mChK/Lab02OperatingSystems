#include "BlockCache.hpp"

#include <cstdlib> // for posix_memalign
#include <cstring> // for memset
#include <unistd.h>

BlockCache::BlockCache(size_t num_blocks, size_t block_size)
    : block_size(block_size), blocks(num_blocks) {
    for (auto& block : blocks) {
        posix_memalign(reinterpret_cast<void**>(&block.data), block_size, block_size);
        block.valid = false;
        block.dirty = false;
    }
}

BlockCache::~BlockCache() {
    for (auto& block : blocks) {
        free(block.data);
    }
}

char* BlockCache::read(fd_t fd, off_t offset, size_t size) {
    off_t block_offset = (offset / block_size) * block_size; // Align to block boundary
    int block_index = map.count(block_offset) ? map[block_offset] : -1;

    if (block_index == -1) {
        // Cache miss: Load block from disk
        block_index = fifo_queue.front();
        fifo_queue.pop();

        Block& block = blocks[block_index];
        if (block.valid && block.dirty) {
            // Write dirty block back to disk
            pwrite(fd, block.data, block_size, block.offset);
        }

        block.valid = true;
        block.dirty = false;
        block.offset = block_offset;

        // Read the block from disk
        pread(fd, block.data, block_size, block_offset);

        // Update cache mapping
        map[block_offset] = block_index;
        fifo_queue.push(block_index);
    }

    // Return the pointer to the requested data
    Block& block = blocks[block_index];
    return block.data + (offset % block_size);
}

void BlockCache::write(fd_t fd, off_t offset, const void* buf, size_t size) {
    off_t block_offset = (offset / block_size) * block_size; // Align to block boundary
    int block_index = map.count(block_offset) ? map[block_offset] : -1;

    if (block_index == -1) {
        // Cache miss: Load block from disk
        block_index = fifo_queue.front();
        fifo_queue.pop();

        Block& block = blocks[block_index];
        if (block.valid && block.dirty) {
            // Write dirty block back to disk
            pwrite(fd, block.data, block_size, block.offset);
        }

        block.valid = true;
        block.dirty = false;
        block.offset = block_offset;

        // Read the block from disk
        pread(fd, block.data, block_size, block_offset);

        // Update cache mapping
        map[block_offset] = block_index;
        fifo_queue.push(block_index);
    }

    // Write to the cache block
    Block& block = blocks[block_index];
    block.dirty = true;
    std::memcpy(block.data + (offset % block_size), buf, size);
}

void BlockCache::flush(fd_t fd) {
    for (auto& block : blocks) {
        if (block.valid && block.dirty) {
            pwrite(fd, block.data, block_size, block.offset);
            block.dirty = false;
        }
    }
}
