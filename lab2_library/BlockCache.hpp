#ifndef BLOCK_CACHE_HPP
#define BLOCK_CACHE_HPP

#include <vector>
#include <cstddef>
#include <cstdint>
#include <utility>     // for std::pair
#include <memory>
#include "Block.hpp"

/**
 * \class BlockCache
 * \brief A fixed-size block cache implementing the Clock eviction policy.
 *
 * Each entry in the cache can hold one block from a file described by:
 *  - fd: file descriptor (int)
 *  - blockIndex: which block number in that file
 *
 * The capacity of this cache is fixed at construction time.
 */
class BlockCache
{
public:
    /**
     * \param capacity   Maximum number of blocks the cache can hold.
     * \param blockSize  Size (in bytes) of each block.
     */
    BlockCache(std::size_t capacity, std::size_t blockSize);
    ~BlockCache() = default;

    /// Return the block size (in bytes).
    std::size_t blockSize() const { return blockSize_; }

    /**
     * \brief Load or fetch a block from the cache, possibly from disk.
     * \param fd         File descriptor.
     * \param blockIndex Block index within the file.
     * \return True on success, false on error (e.g., can't evict).
     */
    bool readBlock(int fd, off_t blockIndex);

    /**
     * \brief Return a pointer to the block’s data, or nullptr if not found.
     */
    void* blockData(int fd, off_t blockIndex);

    /**
     * \brief Mark a block dirty so that it must be written to disk on eviction.
     */
    void markDirty(int fd, off_t blockIndex);

    /**
     * \brief Flush all dirty blocks associated with the given fd.
     */
    void flushFd(int fd);

private:
    /**
     * \struct CacheEntry
     * \brief Holds one block plus metadata about which file/block it represents.
     */
    struct CacheEntry {
        /// Unique pointer to the block itself (aligned buffer, dirty bit, ref bit).
        std::unique_ptr<Block> block;
        /// The file descriptor associated with this block.
        int fd = -1;
        /// The block number within that file.
        off_t blockIndex = -1;
        /// True if this slot is valid (an actual block loaded).
        bool valid = false;
    };

    // -- Private data members -- //

    /// Maximum number of blocks in the cache.
    std::size_t capacity_;
    /// Bytes per block.
    std::size_t blockSize_;
    /**
     * The "clock hand" index.
     * We rotate over [0..capacity_-1] searching for blocks whose referenceBit == false.
     * If referenceBit == true, we set it to false and move on (second chance).
     */
    std::size_t clockHand_ = 0;

    /**
     * The actual storage for our blocks (fixed size).
     * Each entry is a CacheEntry with a unique_ptr<Block> inside it.
     */
    std::vector<CacheEntry> cacheEntries_;

    // -- Private methods -- //

    /**
     * \brief Evict one block from the cache using the Clock algorithm.
     * \return Index of the evicted slot, or (size_t)-1 on failure.
     */
    std::size_t evictOne();

    /**
     * \brief Find the cache slot holding (fd, blockIndex), or return (size_t)-1 if not found.
     */
    std::size_t findSlot(int fd, off_t blockIndex) const;

    /**
     * \brief Load the specified file block from disk into the given CacheEntry.
     */
    bool loadBlockFromDisk(int fd, off_t blockIndex, Block& block);

    /**
     * \brief Write the given block to disk if dirty.
     */
    bool writeBlockToDisk(int fd, Block& block);
};

#endif // BLOCKCACHE_HPP
