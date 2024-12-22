#ifndef BLOCK_CACHE_HPP
#define BLOCK_CACHE_HPP

#include <unordered_map>
#include <deque>
#include <cstddef>
#include <cstdint>
#include <memory>
#include "Block.hpp"
#include "CacheKey.hpp"

/**
 * \class BlockCache
 * \brief A fixed-size block cache implementing the Clock eviction policy.
 */
class BlockCache {
public:
 BlockCache(std::size_t capacity, std::size_t blockSize);
 ~BlockCache() = default;

 std::size_t blockSize() const { return blockSize_; }

 bool readBlock(int fd, off_t blockIndex);
 void* blockData(int fd, off_t blockIndex);
 void markDirty(int fd, off_t blockIndex);
 void flushFd(int fd);

private:
 struct CacheEntry {
  std::unique_ptr<Block> block;
  bool referenceBit = false;
 };

 std::size_t capacity_;
 std::size_t blockSize_;

 /// Map of cache entries: CacheKey -> CacheEntry
 std::unordered_map<CacheKey, CacheEntry> cacheEntries_;
 // /// Order for the Clock algorithm
 // std::deque<CacheKey> evictionOrder_;
 /// Clock hand index for eviction
 std::size_t clockHand_ = 0;
 /// Vector of CacheKeys to maintain order
 std::vector<CacheKey> evictionOrder_;

 bool evictOne();
 bool loadBlockFromDisk(int fd, off_t blockIndex, Block& block);
 bool writeBlockToDisk(int fd, Block& block);
};

#endif // BLOCK_CACHE_HPP
