#ifndef BLOCK_HPP
#define BLOCK_HPP
#include <sys/types.h>

struct Block {
    off_t offset;           // File offset of this block
    bool valid;             // Whether the block contains valid data
    bool dirty;             // Whether the block has been modified
    char* data;             // Pointer to the block data (aligned)
};


#endif //BLOCK_HPP
