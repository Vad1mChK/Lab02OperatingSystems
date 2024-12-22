#include "BlockCache.hpp"
#include <unistd.h>     // pread, pwrite
#include <fcntl.h>      // O_DIRECT, etc.
#include <cerrno>       // errno
#include <cstring>      // memset, memcpy
#include <iostream>     // debug printing, if needed
#include <stdexcept>    // runtime_error

BlockCache::BlockCache(std::size_t capacity, std::size_t blockSize)
    : capacity_(capacity)
    , blockSize_(blockSize)
    , clockHand_(0)
{
    // Pre-allocate vector of cache entries
    cacheEntries_.resize(capacity_);
}

//------------------------------------------------------------------------------
bool BlockCache::readBlock(int fd, off_t blockIndex)
{
    if (capacity_ == 0) {
        throw std::runtime_error("BlockCache capacity is zero; invalid configuration");
    }

    // 1) Check if we already have the block
    std::size_t slot = findSlot(fd, blockIndex);
    if (slot != (std::size_t)-1) {
        // It's already in the cache -> set reference bit to true
        cacheEntries_[slot].block->setReferenceBit(true);
        return true;
    }

    // 2) If not found, we need a free slot or we must evict one
    //    First, find a free slot
    for (std::size_t i = 0; i < capacity_; i++) {
        if (!cacheEntries_[i].valid) {
            // Use this free slot
            slot = i;
            break;
        }
    }
    // If none are free, evict
    if (slot == (std::size_t)-1) {
        slot = evictOne();
        if (slot == (std::size_t)-1) {
            // Eviction failed
            std::cerr << "No block to evict! Capacity reached.\n";
            return false;
        }
    }

    // 3) Prepare the block in that slot
    cacheEntries_[slot].block = std::make_unique<Block>(blockSize_, blockIndex);
    cacheEntries_[slot].fd = fd;
    cacheEntries_[slot].blockIndex = blockIndex;
    cacheEntries_[slot].valid = true;

    // 4) Read data from disk into that block
    if (!loadBlockFromDisk(fd, blockIndex, *cacheEntries_[slot].block)) {
        // If load fails, mark slot invalid
        cacheEntries_[slot].valid = false;
        cacheEntries_[slot].block.reset();
        return false;
    }

    // 5) Mark reference bit
    cacheEntries_[slot].block->setReferenceBit(true);
    return true;
}

//------------------------------------------------------------------------------
void* BlockCache::blockData(int fd, off_t blockIndex)
{
    std::size_t slot = findSlot(fd, blockIndex);
    if (slot == (std::size_t)-1) {
        return nullptr; // Not present
    }
    return cacheEntries_[slot].block->data();
}

//------------------------------------------------------------------------------
void BlockCache::markDirty(int fd, off_t blockIndex)
{
    std::size_t slot = findSlot(fd, blockIndex);
    if (slot != (std::size_t)-1) {
        cacheEntries_[slot].block->setDirty(true);
    }
}

//------------------------------------------------------------------------------
void BlockCache::flushFd(int fd)
{
    for (auto& entry : cacheEntries_) {
        if (entry.valid && entry.fd == fd) {
            if (entry.block->isDirty()) {
                writeBlockToDisk(fd, *entry.block);
                entry.block->setDirty(false);
            }
        }
    }
}

//------------------------------------------------------------------------------
std::size_t BlockCache::evictOne()
{
    if (capacity_ == 0) {
        throw std::runtime_error("BlockCache capacity is zero; invalid configuration");
    }

    // The Clock algorithm:
    // We can attempt up to (2 * capacity_) checks in the worst case
    // (second-chance approach).
    const std::size_t maxChecks = 2 * capacity_;
    std::size_t checks = 0;

    while (checks < maxChecks) {
        auto& entry = cacheEntries_[clockHand_];
        if (!entry.valid) {
            // Strange, an invalid entry is effectively free. Return immediately.
            // But typically you'd never call evictOne() if there's a free slot.
            return clockHand_;
        }

        // If referenceBit == false, we evict it
        if (!entry.block->referenceBit()) {
            // If dirty, write to disk
            if (entry.block->isDirty()) {
                if (!writeBlockToDisk(entry.fd, *entry.block)) {
                    // If we fail to write, we fail the eviction
                    // In a real system, might do something else
                    return static_cast<std::size_t>(-1);
                }
            }
            // Mark slot invalid
            entry.valid = false;
            entry.fd = -1;
            entry.blockIndex = -1;
            entry.block.reset();
            // Return this slot
            std::size_t freedSlot = clockHand_;
            clockHand_ = (clockHand_ + 1) % capacity_;
            return freedSlot;
        }
        // Second chance: set reference bit to false
        entry.block->setReferenceBit(false);

        // Move clock hand
        clockHand_ = (clockHand_ + 1) % capacity_;
        checks++;
    }

    // If we exit the loop, we couldn't evict anything (all blocks got second-chance repeatedly)
    return (std::size_t)-1;
}

//------------------------------------------------------------------------------
std::size_t BlockCache::findSlot(int fd, off_t blockIndex) const
{
    // Linear search
    // If you have a large capacity, consider a better data structure:
    //   e.g., a map<(fd, blockIndex), slot>.
    // But for the sake of demonstration, we keep it simple.
    for (std::size_t i = 0; i < capacity_; i++) {
        const auto& entry = cacheEntries_[i];
        if (entry.valid && entry.fd == fd && entry.blockIndex == blockIndex) {
            return i;
        }
    }
    return (std::size_t)-1;
}

//------------------------------------------------------------------------------
bool BlockCache::loadBlockFromDisk(int fd, off_t blockIndex, Block& block)
{
    off_t offset = blockIndex * static_cast<off_t>(blockSize_);
    // We read exactly blockSize_ bytes from disk
    ssize_t bytesRead = ::pread(fd, block.data(), blockSize_, offset);
    if (bytesRead < 0) {
        std::cerr << "Error reading from disk at offset "
                  << offset << ": " << std::strerror(errno) << "\n";
        return false;
    }
    // If the file is smaller than blockSize_, zero-fill remainder
    if (bytesRead < (ssize_t)blockSize_) {
        std::memset(static_cast<char*>(block.data()) + bytesRead, 0,
                    blockSize_ - bytesRead);
    }
    return true;
}

//------------------------------------------------------------------------------
bool BlockCache::writeBlockToDisk(int fd, Block& block)
{
    off_t offset = block.index() * static_cast<off_t>(blockSize_);
    ssize_t written = ::pwrite(fd, block.data(), blockSize_, offset);
    if (written < 0) {
        std::cerr << "Error writing to disk at offset "
                  << offset << ": " << std::strerror(errno) << "\n";
        return false;
    }
    return true;
}