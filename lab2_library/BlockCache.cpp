//
// Created by vad1mchk on 16/12/24.
//

#include "BlockCache.hpp"

#include <cstdio>
#include <stdexcept>
#include <unistd.h>

Block* BlockCache::get_block(off_t offset, bool for_write) {
    // Check if the block is already in the cache
    for (auto& block : blocks) {
        if (block.valid && block.offset == offset) {
            block.reference = true; // Mark as recently used
            return &block;
        }
    }

    // Cache miss: Evict a block and load the new one
    evict_block();

    // Find the block to replace (it will now be invalid)
    auto& block = blocks[clock_hand];
    block.offset = offset;
    block.valid = true;
    block.dirty = for_write;
    block.reference = true;

    // Load the data from the file (for read operations)
    if (!for_write) {
        ssize_t bytes_read = pread(fd, block.data, block.size, offset);
        if (bytes_read < 0) {
            perror("pread");
            throw std::runtime_error("Failed to read block from file");
        }
    }

    return &block;
}

void BlockCache::evict_block() {
    while (true) {
        auto& block = blocks[clock_hand];
        if (!block.reference) {
            if (block.valid && block.dirty) {
                // Write back dirty block to the file
                ssize_t bytes_written = pwrite(fd, block.data, block.size, block.offset);
                if (bytes_written < 0) {
                    perror("pwrite");
                    throw std::runtime_error("Failed to write block to file");
                }
            }

            // Invalidate the block
            block.valid = false;
            block.dirty = false;
            block.offset = -1;
            return; // Eviction complete
        }

        // Reset reference bit and move to the next block
        block.reference = false;
        clock_hand = (clock_hand + 1) % blocks.size();
    }
}

void BlockCache::flush_all() {
    for (auto& block : blocks) {
        if (block.valid && block.dirty) {
            ssize_t bytes_written = pwrite(fd, block.data, block.size, block.offset);
            if (bytes_written < 0) {
                perror("pwrite");
                throw std::runtime_error("Failed to flush block to file");
            }
            block.dirty = false; // Mark as clean
        }
    }
}
