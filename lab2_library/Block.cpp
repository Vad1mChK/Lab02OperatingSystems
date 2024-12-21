#include "Block.hpp"
#include <cstdlib>      // posix_memalign, free
#include <stdexcept>    // runtime_error
#include <cstring>      // memset

Block::Block(std::size_t blockSize, off_t blockIndex)
    : blockIndex_(blockIndex)
    , dirty_(false)
    , referenceBit_(false)
    , data_(nullptr, &Block::deleter)
{
    // Common alignment for direct I/O is 4096.
    // If your FS is different, adjust accordingly.
    const std::size_t alignment = 4096;

    void* rawPtr = nullptr;
    int ret = ::posix_memalign(&rawPtr, alignment, blockSize);
    if (ret != 0) {
        throw std::runtime_error("posix_memalign failed in Block constructor.");
    }

    data_.reset(reinterpret_cast<unsigned char*>(rawPtr));
    // Optional: zero-initialize
    std::memset(data_.get(), 0, blockSize);
}

Block::~Block()
{
    // data_ is automatically freed by the unique_ptr with our custom deleter.
}
