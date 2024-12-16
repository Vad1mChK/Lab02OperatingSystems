//
// Created by vad1mchk on 16/12/24.
//

#ifndef BLOCKCACHE_HPP
#define BLOCKCACHE_HPP
#include <vector>

#include "Block.hpp"

class BlockCache {
private:
    std::vector<Block> blocks;
    fd_t fd;
    off_t position = 0;
    size_t block_size = 0;
    size_t clock_hand = 0;
public:
    BlockCache(fd_t fd, size_t block_count, size_t block_size): fd(fd), block_size(block_size) {
        blocks.reserve(block_count); // Reserve memory for blocks

        for (size_t i = 0; i < block_count; ++i) {
            blocks.emplace_back(fd, -1, block_size); // Add invalid blocks to the cache
        }
    }

    ~BlockCache() {
        flush_all();
    }

    Block* get_block(off_t offset, bool for_write);
    off_t get_position() const {
        return position;
    }
    void set_position(off_t new_position) {
        position = new_position;
    }
    size_t get_block_size() const {
        return block_size;
    }

    void evict_block();
    void flush_all();
};

#endif //BLOCKCACHE_HPP
