#include <gtest/gtest.h>
#include "lab2_library.hpp"
#include <cstring>
#include <cstdlib>
#include <memory>

// Функция для создания выровненного буфера с использованием unique_ptr
std::unique_ptr<char, decltype(&free)> create_aligned_buffer(size_t size) {
    void* ptr;
    if (posix_memalign(&ptr, 4096, size) != 0) {
        return std::unique_ptr<char, decltype(&free)>(nullptr, free);
    }
    return std::unique_ptr<char, decltype(&free)>(static_cast<char*>(ptr), free);
}

TEST(Lab2LibrarySmokeTest, WriteFile) {
    const char* data = "Hello, World!";
    size_t data_size = strlen(data);

    // Создаем выровненный буфер размером 4096 байт
    auto buffer = create_aligned_buffer(4096);
    ASSERT_NE(buffer.get(), nullptr) << "posix_memalign failed";

    memset(buffer.get(), 0, 4096); // Заполняем нулями
    memcpy(buffer.get(), data, data_size); // Копируем данные

    int fd = lab2_open("testfile");
    ASSERT_NE(fd, -1);

    ssize_t bytes_written = lab2_write(fd, buffer.get(), 4096);
    ASSERT_EQ(bytes_written, 4096);

    lab2_close(fd);
}

TEST(Lab2LibrarySmokeTest, ReadFile) {
    const char* expected_data = "Hello, World!";
    size_t data_size = strlen(expected_data);

    // Создаем выровненный буфер размером 4096 байт для записи
    auto write_buffer = create_aligned_buffer(4096);
    ASSERT_NE(write_buffer.get(), nullptr) << "posix_memalign failed";

    memset(write_buffer.get(), 0, 4096);
    memcpy(write_buffer.get(), expected_data, data_size);

    int fd = lab2_open("testfile");
    ASSERT_NE(fd, -1);

    // Записываем данные
    ssize_t bytes_written = lab2_write(fd, write_buffer.get(), 4096);
    ASSERT_EQ(bytes_written, 4096);

    // Перемещаем указатель на начало файла
    off_t new_pos = lab2_lseek(fd, 0, SEEK_SET);
    ASSERT_EQ(new_pos, 0);

    // Создаем выровненный буфер размером 4096 байт для чтения
    auto read_buffer = create_aligned_buffer(4096);
    ASSERT_NE(read_buffer.get(), nullptr) << "posix_memalign failed";
    memset(read_buffer.get(), 0, 4096);

    // Читаем данные
    ssize_t bytes_read = lab2_read(fd, read_buffer.get(), 4096);
    ASSERT_EQ(bytes_read, 4096);

    // Проверяем только первые 13 байт
    ASSERT_EQ(memcmp(read_buffer.get(), expected_data, data_size), 0);

    lab2_close(fd);
}

TEST(Lab2LibrarySmokeTest, SeekAndWriteFile) {
    const char* initial_data = "Initial content...";
    const char* updated_data = "Initial UPDATED...";
    size_t initial_size = strlen(initial_data);
    size_t updated_size = strlen(updated_data);

    // Создаем выровненные буферы размером 4096 байт
    auto aligned_buf_initial = create_aligned_buffer(4096);
    auto aligned_buf_updated = create_aligned_buffer(4096);
    ASSERT_NE(aligned_buf_initial.get(), nullptr) << "posix_memalign failed";
    ASSERT_NE(aligned_buf_updated.get(), nullptr) << "posix_memalign failed";

    memset(aligned_buf_initial.get(), 0, 4096);
    memset(aligned_buf_updated.get(), 0, 4096);
    memcpy(aligned_buf_initial.get(), initial_data, initial_size);
    memcpy(aligned_buf_updated.get(), updated_data, updated_size);

    int fd = lab2_open("testfile");
    ASSERT_NE(fd, -1);

    // Записываем начальные данные
    ssize_t bytes_written = lab2_write(fd, aligned_buf_initial.get(), 4096);
    ASSERT_EQ(bytes_written, 4096);

    // Перемещаем указатель на начало файла
    off_t new_pos = lab2_lseek(fd, 0, SEEK_SET);
    ASSERT_EQ(new_pos, 0);

    // Записываем обновленные данные
    bytes_written = lab2_write(fd, aligned_buf_updated.get(), 4096);
    ASSERT_EQ(bytes_written, 4096);

    // Перемещаем указатель на начало файла
    new_pos = lab2_lseek(fd, 0, SEEK_SET);
    ASSERT_EQ(new_pos, 0);

    // Создаем выровненный буфер для чтения
    auto read_buffer = create_aligned_buffer(4096);
    ASSERT_NE(read_buffer.get(), nullptr) << "posix_memalign failed";
    memset(read_buffer.get(), 0, 4096);

    // Читаем данные
    ssize_t bytes_read = lab2_read(fd, read_buffer.get(), 4096);
    ASSERT_EQ(bytes_read, 4096);

    // Сравниваем первые 17 байт
    ASSERT_EQ(memcmp(read_buffer.get(), updated_data, updated_size), 0);

    lab2_close(fd);
}

TEST(Lab2LibrarySmokeTest, FsyncTest) {
    const char* data = "FSYNC UPDATED";
    size_t data_size = strlen(data);

    // Создаем выровненный буфер размером 4096 байт
    auto buffer = create_aligned_buffer(4096);
    ASSERT_NE(buffer.get(), nullptr) << "posix_memalign failed";

    memset(buffer.get(), 0, 4096);
    memcpy(buffer.get(), data, data_size);

    int fd = lab2_open("testfile");
    ASSERT_NE(fd, -1);

    // Записываем данные
    ssize_t bytes_written = lab2_write(fd, buffer.get(), 4096);
    ASSERT_EQ(bytes_written, 4096);

    // Выполняем синхронизацию
    int sync_result = lab2_fsync(fd);
    ASSERT_EQ(sync_result, 0);

    // Перемещаем указатель на начало файла
    off_t new_pos = lab2_lseek(fd, 0, SEEK_SET);
    ASSERT_EQ(new_pos, 0);

    // Создаем выровненный буфер для чтения
    auto read_buffer = create_aligned_buffer(4096);
    ASSERT_NE(read_buffer.get(), nullptr) << "posix_memalign failed";
    memset(read_buffer.get(), 0, 4096);

    // Читаем данные
    ssize_t bytes_read = lab2_read(fd, read_buffer.get(), 4096);
    ASSERT_EQ(bytes_read, 4096);

    // Сравниваем первые 13 байт
    ASSERT_EQ(memcmp(read_buffer.get(), data, data_size), 0);

    lab2_close(fd);
}
