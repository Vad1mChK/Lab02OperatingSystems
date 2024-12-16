#include "ClockCache.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

ClockCache::ClockCache(size_t num_pages, size_t psize)
    : page_size(psize), clock_hand(0), pages() {
    pages.reserve(num_pages);
    for (size_t i = 0; i < num_pages; ++i) {
        pages.emplace_back(page_size);
    }
}

ClockCache::~ClockCache() {
    // unique_ptr автоматически освободит память
}

void ClockCache::flush_all(int fd) {
    for (auto &pg : pages) {
        if (pg.valid && pg.dirty) {
            ssize_t written = pwrite(fd, pg.data.get(), page_size, pg.file_offset);
            if (written < 0) {
                perror("pwrite");
                // Можно обработать ошибку или продолжить
            }
            pg.dirty = false;
        }
    }
}

void ClockCache::evict_page() {
    while (true) {
        auto &page = pages[clock_hand];
        if (!page.reference) {
            // Evict
            if (page.valid && page.dirty) {
                ssize_t written = pwrite(page.fd, page.data.get(), page_size, page.file_offset);
                if (written < 0) {
                    perror("pwrite");
                    // Можно обработать ошибку или продолжить
                }
                page.dirty = false;
            }
            page.valid = false;
            page.file_offset = -1;
            page.fd = -1;
            return;
        } else {
            page.reference = false;
            clock_hand = (clock_hand + 1) % pages.size();
        }
    }
}

void ClockCache::load_page(int fd, off_t offset) {
    evict_page();
    auto &page = pages[clock_hand];
    page.file_offset = offset;
    page.fd = fd;
    page.valid = true;
    page.dirty = false;
    page.reference = true;
    ssize_t rb = pread(fd, page.data.get(), page_size, offset);
    if (rb < 0) {
        perror("pread");
        memset(page.data.get(), 0, page_size);
    }
    // Следующая страница
    clock_hand = (clock_hand + 1) % pages.size();
}

char* ClockCache::get_page(int fd, off_t offset, bool for_write) {
    // Поиск страницы
    for (auto &page : pages) {
        if (page.valid && page.file_offset == offset && page.fd == fd) {
            page.reference = true;
            return page.data.get();
        }
    }
    // Нет в кэше
    load_page(fd, offset);
    size_t idx = (clock_hand + pages.size() - 1) % pages.size();
    return pages[idx].data.get();
}

void ClockCache::mark_page_dirty(off_t offset) {
    for (auto &page : pages) {
        if (page.valid && page.file_offset == offset) {
            page.dirty = true;
            return;
        }
    }
}
