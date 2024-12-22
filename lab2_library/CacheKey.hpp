#ifndef CACHE_KEY_HPP
#define CACHE_KEY_HPP

#include <functional> // for std::hash
#include <cstddef>    // for std::size_t

#define GOLDEN_RATIO_HASH_CONSTANT 0x9E3779B9

/**
 * @struct CacheKey
 * Represents a unique key for a block in the cache, combining a file descriptor and a block index.
 */
struct CacheKey {
    int fd;            // File descriptor
    off_t blockIndex;  // Block index within the file

    // Equality operator for hash-based containers (e.g., std::unordered_map)
    bool operator==(const CacheKey& other) const {
        return fd == other.fd && blockIndex == other.blockIndex;
    }

    // Comparison operator for tree-based containers (e.g., std::map)
    bool operator<(const CacheKey& other) const {
        return (fd < other.fd) || (fd == other.fd && blockIndex < other.blockIndex);
    }
};

// Hash specialization for std::unordered_map
namespace std {
    template <>
    struct hash<CacheKey> {
        size_t operator()(const CacheKey& key) const noexcept {
            // Combine hashes of `fd` and `blockIndex` using a simple hash combining technique
            size_t h1 = std::hash<int>()(key.fd);
            size_t h2 = std::hash<off_t>()(key.blockIndex);
            return h1 ^ (h2 + GOLDEN_RATIO_HASH_CONSTANT + (h1 << 6) + (h1 >> 2)); // Inspired by boost::hash_combine
        }
    };
}

#endif // CACHE_KEY_HPP