#include "lab2_library.hpp"
#include "ClockCache.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <system_error>
#include <unordered_map>
#include <sys/stat.h>

#include "util.hpp"

// Structure to store file-specific metadata
struct FileState {
    fd_t real_fd;        // Real file descriptor
    off_t position;      // Current file position
    ClockCache cache;    // Cache for this file

    // Default constructor (required for std::unordered_map)
    FileState()
        : real_fd(-1),                          // Invalid default file descriptor
          position(0),                          // Default position
          cache(LAB2_NUM_PAGES, LAB2_DEFAULT_BLOCK_SIZE) {} // Default ClockCache (16 pages, 4KB each)

    // Parameterized constructor
    FileState(fd_t fd, size_t num_pages, size_t page_size)
        : real_fd(fd),
          position(0),
          cache(num_pages, page_size) {}
};

// Global map for open files
std::unordered_map<fd_t, FileState> open_files;
fd_t next_fd = 1; // Custom file descriptor counter

fd_t lab2_open(const std::string& path) {
    int real_fd = open(path.c_str(), O_RDWR | O_DIRECT | O_CREAT, 0644);
    if (real_fd < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to open file");
    }

    // Initialize file state with a cache
    FileState file_state = FileState(
        real_fd,
        LAB2_NUM_PAGES,
        LAB2_DEFAULT_BLOCK_SIZE
    );

    fd_t custom_fd = next_fd++;
    open_files[custom_fd] = file_state;

    return custom_fd;
}

int lab2_close(fd_t fd) {
    if (open_files.find(fd) == open_files.end()) {
        errno = EBADF;
        return -1;
    }

    FileState& file_state = open_files[fd];
    lab2_fsync(fd); // Sync data before closing
    close(file_state.real_fd);
    open_files.erase(fd);

    return 0;
}

ssize_t lab2_read(fd_t fd, void* buf, ssize_t count) {
    if (open_files.find(fd) == open_files.end()) {
        errno = EBADF;
        return -1;
    }

    FileState& file_state = open_files[fd];
    ssize_t bytes_read = 0;
    char* buffer = static_cast<char*>(buf);

    while (count > 0) {
        off_t page_offset = file_state.position / LAB2_DEFAULT_BLOCK_SIZE * LAB2_DEFAULT_BLOCK_SIZE;
        size_t page_offset_within = file_state.position % LAB2_DEFAULT_BLOCK_SIZE;
        size_t to_read = std::min(static_cast<size_t>(count), LAB2_DEFAULT_BLOCK_SIZE - page_offset_within);

        char* page_data = file_state.cache.get_page(file_state.real_fd, page_offset);
        std::memcpy(buffer, page_data + page_offset_within, to_read);

        buffer += to_read;
        bytes_read += to_read;
        count -= to_read;
        file_state.position += to_read;
    }

    return bytes_read;
}

ssize_t lab2_write(fd_t fd, const void* buf, ssize_t count) {
    if (open_files.find(fd) == open_files.end()) {
        errno = EBADF;
        return -1;
    }

    FileState& file_state = open_files[fd];
    ssize_t bytes_written = 0;
    const char* buffer = static_cast<const char*>(buf);

    while (count > 0) {
        off_t page_offset = file_state.position / LAB2_DEFAULT_BLOCK_SIZE * LAB2_DEFAULT_BLOCK_SIZE;
        size_t page_offset_within = file_state.position % LAB2_DEFAULT_BLOCK_SIZE;
        size_t to_write = std::min(static_cast<size_t>(count), LAB2_DEFAULT_BLOCK_SIZE - page_offset_within);

        char* page_data = file_state.cache.get_page(file_state.real_fd, page_offset);
        std::memcpy(page_data + page_offset_within, buffer, to_write);

        file_state.cache.mark_page_dirty(page_offset);

        buffer += to_write;
        bytes_written += to_write;
        count -= to_write;
        file_state.position += to_write;
    }

    return bytes_written;
}

off_t lab2_lseek(fd_t fd, off_t offset, int whence) {
    if (open_files.find(fd) == open_files.end()) {
        errno = EBADF;
        return -1;
    }

    FileState& file_state = open_files[fd];
    off_t new_position = 0;

    switch (whence) {
        case SEEK_SET:
            new_position = offset;
            break;
        case SEEK_CUR:
            new_position = file_state.position + offset;
            break;
        case SEEK_END: {
            struct stat st;
            if (fstat(file_state.real_fd, &st) < 0) {
                return -1;
            }
            new_position = st.st_size + offset;
            break;
        }
        default:
            errno = EINVAL;
            return -1;
    }

    if (new_position < 0) {
        errno = EINVAL;
        return -1;
    }

    file_state.position = new_position;
    return new_position;
}

int lab2_fsync(fd_t fd) {
    if (open_files.find(fd) == open_files.end()) {
        errno = EBADF;
        return -1;
    }

    FileState& file_state = open_files[fd];

    // Flush all dirty pages using the callback
    file_state.cache.invoke_for_each_dirty_page([&file_state](CachePage& page) {
        ssize_t written = pwrite(file_state.real_fd, page.data, LAB2_DEFAULT_BLOCK_SIZE, page.file_offset);
        if (written < 0) {
            perror("Error writing dirty page to disk");
            throw std::runtime_error("Failed to write dirty page to disk"); // Handle the error cleanly
        }
        page.dirty = false; // Mark the page as clean
    });

    // Call fsync on the real file descriptor to flush to disk
    return fsync(file_state.real_fd);
}


int lab2_advice(int fd, off_t offset, access_hint_t hint) {
    // Not implemented
    return TODO(int);
}
