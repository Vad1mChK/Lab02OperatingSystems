//
// Created by vad1mchk on 09/12/24.
//

#ifndef BLOCKCACHE_H
#define BLOCKCACHE_H

#include <vector>
#include <queue>
#include <unordered_map>

#include "Block.hpp"

typedef int fd_t;
constexpr auto LAB2_DEFAULT_BLOCK_SIZE = 4096;

class BlockCache {
public:
    explicit BlockCache(size_t num_blocks, size_t block_size = LAB2_DEFAULT_BLOCK_SIZE);
    ~BlockCache();

    // Cache operations
    char* read(fd_t fd, off_t offset, size_t size);
    void write(fd_t fd, off_t offset, const void* buf, size_t size);
    void flush(fd_t fd); // Flush all dirty blocks to disk

private:
    size_t block_size;
    std::vector<Block> blocks;     // Cache storage
    std::queue<int> fifo_queue;         // FIFO queue for block eviction
    std::unordered_map<off_t, int> map; // Maps file offsets to cache block indices
};


#endif //BLOCKCACHE_H
