#include "lab2_library.hpp"
#include "ClockCache.hpp"

#include <unordered_map>
#include <memory>
#include <system_error>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>

struct FileState {
    int real_fd;
    off_t position;
    std::unique_ptr<ClockCache> cache;

    FileState(int fd, size_t pages, size_t psize)
        : real_fd(fd), position(0), cache(std::make_unique<ClockCache>(pages, psize)) {}

    // Удаляем копирование
    FileState(const FileState&) = delete;
    FileState& operator=(const FileState&) = delete;

    // Позволяем перемещение
    FileState(FileState&&) = default;
    FileState& operator=(FileState&&) = default;
};

static std::unordered_map<int, std::unique_ptr<FileState>> open_files;
static int next_fd = 1;
static const size_t PAGE_SIZE = 4096;
static const size_t CACHE_PAGES = 16;

int lab2_open(const std::string &path) {
    int fd = ::open(path.c_str(), O_RDWR | O_DIRECT | O_CREAT, 0644);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    // Создаём FileState и добавляем в open_files
    auto fs = std::make_unique<FileState>(fd, CACHE_PAGES, PAGE_SIZE);
    int user_fd = next_fd++;
    open_files.emplace(user_fd, std::move(fs));
    return user_fd;
}

int lab2_close(fd_t fd) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) {
        errno = EBADF;
        return -1;
    }
    // Синхронизируем данные
    lab2_fsync(fd);
    ::close(it->second->real_fd);
    open_files.erase(it);
    return 0;
}

ssize_t lab2_read(fd_t fd, void *buf, size_t count) {
    if (count % PAGE_SIZE != 0) {
        errno = EINVAL;
        return -1;
    }

    auto it = open_files.find(fd);
    if (it == open_files.end()) {
        errno = EBADF;
        return -1;
    }
    FileState &fs = *(it->second);

    char* out_ptr = static_cast<char*>(buf);
    size_t to_read = count;
    ssize_t total_read = 0;

    while (to_read > 0) {
        off_t page_offset = (fs.position / PAGE_SIZE) * PAGE_SIZE;
        size_t offset_in_page = fs.position % PAGE_SIZE;
        size_t can_read = PAGE_SIZE - offset_in_page;
        if (can_read > to_read) can_read = to_read;

        char *page_data = fs.cache->get_page(fs.real_fd, page_offset, false);
        memcpy(out_ptr, page_data + offset_in_page, can_read);

        out_ptr += can_read;
        to_read -= can_read;
        total_read += can_read;
        fs.position += can_read;
    }

    return total_read; // Возвращаем фактически прочитанное количество байт
}

ssize_t lab2_write(fd_t fd, const void *buf, size_t count) {
    if (count % PAGE_SIZE != 0) {
        errno = EINVAL;
        return -1;
    }

    auto it = open_files.find(fd);
    if (it == open_files.end()) {
        errno = EBADF;
        return -1;
    }
    FileState &fs = *(it->second);

    const char* in_ptr = static_cast<const char*>(buf);
    size_t to_write = count;
    ssize_t total_written = 0;

    while (to_write > 0) {
        off_t page_offset = (fs.position / PAGE_SIZE) * PAGE_SIZE;
        size_t offset_in_page = fs.position % PAGE_SIZE;
        size_t can_write = PAGE_SIZE - offset_in_page;
        if (can_write > to_write) can_write = to_write;

        char *page_data = fs.cache->get_page(fs.real_fd, page_offset, true);
        memcpy(page_data + offset_in_page, in_ptr, can_write);
        fs.cache->mark_page_dirty(page_offset);

        in_ptr += can_write;
        to_write -= can_write;
        total_written += can_write;
        fs.position += can_write;
    }

    return total_written; // Возвращаем фактически записанное количество байт
}

off_t lab2_lseek(const fd_t fd, off_t offset, int whence) {
    const auto it = open_files.find(fd);
    if (it == open_files.end()) {
        errno = EBADF;
        return (off_t)-1;
    }
    FileState &fs = *(it->second);

    off_t new_pos;
    if (whence == SEEK_SET) {
        new_pos = offset;
    } else {
        // Поддерживаем только SEEK_SET
        fprintf(stderr, "lab2_lseek: Only SEEK_SET is supported\n");
        errno = EINVAL;
        return (off_t)-1;
    }

    if (new_pos < 0) {
        errno = EINVAL;
        return (off_t)-1;
    }

    fs.position = new_pos;
    return new_pos;
}

int lab2_fsync(fd_t fd) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) {
        errno = EBADF;
        return -1;
    }
    const FileState &fs = *(it->second);
    fs.cache->flush_all(fs.real_fd);
    if (::fsync(fs.real_fd) != 0) {
        perror("fsync");
        return -1;
    }
    return 0;
}

int lab2_advice(int fd, off_t offset, access_hint_t hint) {
    fprintf(stderr, "lab2_advice: Advice ignored (fd=%d, offset=%ld, hint=%ld)\n", fd, (long)offset, (long)hint);
    return 0;
}
