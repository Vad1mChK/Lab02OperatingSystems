#include "BlockCache.hpp"
#include <unistd.h>     // pread, pwrite
#include <fcntl.h>      // O_DIRECT, etc.
#include <cerrno>       // errno
#include <cstring>      // memset, memcpy
#include <iostream>     // debug printing, if needed
#include <stdexcept>    // runtime_error
#include <algorithm>    // std::remove_if

BlockCache::BlockCache(std::size_t capacity, std::size_t blockSize)
    : capacity_(capacity)
    , blockSize_(blockSize)
{
    evictionOrder_.reserve(capacity_);
}

bool BlockCache::readBlock(int fd, off_t blockIndex) {
    if (capacity_ == 0) {
        throw std::runtime_error("BlockCache capacity is zero; invalid configuration");
    }

    CacheKey key{fd, blockIndex};

    // Check if the block is already in the cache
    auto it = cacheEntries_.find(key);
    if (it != cacheEntries_.end()) {
        // Block is in cache; update reference bit
        std::cerr << "Block found in cache (fd=" << fd << ", blockIndex=" << blockIndex << "), setting reference bit to true.\n";
        it->second.referenceBit = true;
        return true;
    }

    // If the block is not in the cache, load it
    if (cacheEntries_.size() >= capacity_) {
        // Evict one block if the cache is full
        std::cerr << "Cache is full (capacity=" << capacity_ << "), attempting to evict a block.\n";
        if (!evictOne()) {
            std::cerr << "Failed to evict a block from cache (fd=" << fd << ", blockIndex=" << blockIndex << ").\n";
            errno = ENOMEM; // Out of memory
            return false;
        }
    }

    // Load the block into the cache
    CacheEntry newEntry;
    newEntry.block = std::make_unique<Block>(blockSize_, blockIndex);
    if (!loadBlockFromDisk(fd, blockIndex, *newEntry.block)) {
        std::cerr << "Failed to load block from disk (fd=" << fd << ", blockIndex=" << blockIndex << ").\n";
        return false;
    }

    std::cerr << "Loaded new block into cache (fd=" << fd << ", blockIndex=" << blockIndex << "), setting reference bit to true.\n";
    newEntry.referenceBit = true;

    // Insert the new block into the cache
    cacheEntries_[key] = std::move(newEntry);
    evictionOrder_.push_back(key);

    return true;
}

void* BlockCache::blockData(int fd, off_t blockIndex) {
    CacheKey key{fd, blockIndex};
    auto it = cacheEntries_.find(key);
    if (it == cacheEntries_.end()) {
        return nullptr; // Not present
    }
    return it->second.block->data();
}

void BlockCache::markDirty(int fd, off_t blockIndex) {
    CacheKey key{fd, blockIndex};
    auto it = cacheEntries_.find(key);
    if (it != cacheEntries_.end()) {
        it->second.block->setDirty(true);
    }
}

void BlockCache::flushFd(int fd) {
    for (auto it = cacheEntries_.begin(); it != cacheEntries_.end();) {
        if (it->first.fd == fd) {
            if (it->second.block->isDirty()) {
                if (!writeBlockToDisk(it->first.fd, *it->second.block)) {
                    std::cerr << "Failed to write dirty block to disk (fd=" << fd << ", blockIndex=" << it->first.blockIndex << ").\n";
                }
                it->second.block->setDirty(false);
            }
            // Remove from evictionOrder_
            auto evictIt = std::find(evictionOrder_.begin(), evictionOrder_.end(), it->first);
            if (evictIt != evictionOrder_.end()) {
                size_t index = std::distance(evictionOrder_.begin(), evictIt);
                evictionOrder_.erase(evictIt);
                // Adjust clockHand_ if necessary
                if (index < clockHand_) {
                    if (clockHand_ > 0) clockHand_--;
                } else if (clockHand_ >= evictionOrder_.size()) {
                    clockHand_ = 0;
                }
            }
            it = cacheEntries_.erase(it); // Remove the block
        } else {
            ++it;
        }
    }
}

bool BlockCache::evictOne() {
    if (capacity_ == 0 || evictionOrder_.empty()) {
        std::cerr << "Could not evict the block. Either the capacity is 0 or the eviction order is empty.\n";
        return false;
    }

    size_t initialClockHand = clockHand_;
    size_t checks = 0;

    while (checks < capacity_) {
        if (evictionOrder_.empty()) {
            std::cerr << "Eviction order is empty. Cannot evict.\n";
            return false;
        }

        // Wrap around using modulo
        clockHand_ %= evictionOrder_.size();

        CacheKey currentKey = evictionOrder_[clockHand_];
        auto it = cacheEntries_.find(currentKey);
        if (it != cacheEntries_.end()) {
            if (!it->second.referenceBit) {
                // Evict this block
                std::cerr << "Evicting block (fd=" << currentKey.fd << ", blockIndex=" << currentKey.blockIndex << ").\n";
                if (it->second.block->isDirty()) {
                    if (!writeBlockToDisk(it->first.fd, *it->second.block)) {
                        std::cerr << "Could not write dirty block to disk (fd=" << currentKey.fd << ", blockIndex=" << currentKey.blockIndex << ").\n";
                        return false;
                    }
                }
                // Remove from cacheEntries_ and evictionOrder_
                cacheEntries_.erase(it);
                evictionOrder_.erase(evictionOrder_.begin() + clockHand_);
                std::cerr << "Successfully evicted block (fd=" << currentKey.fd << ", blockIndex=" << currentKey.blockIndex << ").\n";
                // Do not advance clockHand_ since the current position now points to the next block
                return true; // Eviction successful
            } else {
                // Give a second chance
                it->second.referenceBit = false;
                std::cerr << "Second chance given to block (fd=" << currentKey.fd << ", blockIndex=" << currentKey.blockIndex << ").\n";
            }
        } else {
            std::cerr << "Block not found in cacheEntries_ during eviction (fd=" << currentKey.fd << ", blockIndex=" << currentKey.blockIndex << "). Removing from evictionOrder_.\n";
            evictionOrder_.erase(evictionOrder_.begin() + clockHand_);
            // Do not advance clockHand_ as the current position now points to the next block
            continue;
        }

        // Move clockHand_ forward
        clockHand_++;
        checks++;
    }

    std::cerr << "No block found for eviction after iterating through all blocks.\n";
    return false; // Eviction failed
}

bool BlockCache::loadBlockFromDisk(int fd, off_t blockIndex, Block& block) {
    off_t offset = blockIndex * static_cast<off_t>(blockSize_);
    // We read exactly blockSize_ bytes from disk
    ssize_t bytesRead = ::pread(fd, block.data(), blockSize_, offset);
    if (bytesRead < 0) {
        std::cerr << "Error reading from disk at offset "
                  << offset << ": " << std::strerror(errno) << "\n";
        return false;
    }
    // If the file is smaller than blockSize_, zero-fill remainder
    if (bytesRead < static_cast<ssize_t>(blockSize_)) {
        std::memset(static_cast<char*>(block.data()) + bytesRead, 0,
                    blockSize_ - bytesRead);
    }
    return true;
}

bool BlockCache::writeBlockToDisk(int fd, Block& block) {
    off_t offset = block.index() * static_cast<off_t>(blockSize_);
    ssize_t written = ::pwrite(fd, block.data(), blockSize_, offset);
    if (written < 0) {
        std::cerr << "Error writing to disk at offset "
                  << offset << ": " << std::strerror(errno) << "\n";
        return false;
    }
    if (static_cast<size_t>(written) != blockSize_) {
        std::cerr << "Partial write to disk at offset "
                  << offset << ": wrote " << written << " bytes, expected " << blockSize_ << " bytes.\n";
        return false;
    }
    return true;
}
