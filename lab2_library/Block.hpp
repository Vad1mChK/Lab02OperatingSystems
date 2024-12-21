#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <cstddef>
#include <memory>

class Block {
public:
    Block(std::size_t blockSize, off_t blockIndex);
    ~Block();

    // Accessors
    void* data() { return data_.get(); }
    const void* data() const { return data_.get(); }

    bool isDirty() const { return dirty_; }
    bool referenceBit() const { return referenceBit_; }
    off_t index() const { return blockIndex_; }

    void setDirty(bool d) { dirty_ = d; }
    void setReferenceBit(bool b) { referenceBit_ = b; }

private:
    off_t blockIndex_;
    bool dirty_;
    bool referenceBit_;
    // Aligned memory
    std::unique_ptr<unsigned char[], void(*)(void*)> data_;
    // Could store blockSize_ if needed

    // Helper for allocation
    static void deleter(void* p) { ::free(p); }
};

#endif //BLOCK_HPP
