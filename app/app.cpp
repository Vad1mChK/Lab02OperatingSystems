#include "lab2_library.hpp"
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdlib>

#define DATA_SIZE (256 * 1024 * 1024) // 256 MB

int main() {
    try {
        const fd_t fd = lab2_open("/home/vad1mchk/testfile.txt");

        // Allocate aligned memory for write buffer
        char* write_data;
        if (posix_memalign(reinterpret_cast<void**>(&write_data), 4096, DATA_SIZE) != 0) {
            throw std::runtime_error("Failed to allocate aligned memory for write_data");
        }

        // Fill write_data buffer
        for (size_t i = 0; i < DATA_SIZE; ++i) {
            write_data[i] = (i % 32 == 31) ? '\n' : (i & 1) ? 'H' : ('h' + ((i / 32) & 15));
        }

        // Write to file
        ssize_t written = lab2_write(fd, write_data, DATA_SIZE);
        if (written != DATA_SIZE) {
            throw std::runtime_error("Failed to write the entire buffer");
        }

        // Seek back to the beginning of the file
        lab2_lseek(fd, 0, SEEK_SET);

        // Allocate aligned memory for read buffer
        char* read_data;
        if (posix_memalign(reinterpret_cast<void**>(&read_data), 4096, DATA_SIZE) != 0) {
            free(write_data); // Free write buffer before exiting
            throw std::runtime_error("Failed to allocate aligned memory for read_data");
        }
        memset(read_data, 0, DATA_SIZE);

        // Read from file
        ssize_t read = lab2_read(fd, read_data, DATA_SIZE);
        if (read != DATA_SIZE) {
            free(write_data);
            free(read_data);
            throw std::runtime_error("Failed to read the entire buffer");
        }

        // Print the read data
        // std::cout << "Read data: " << read_data << std::endl;

        // Free allocated buffers
        free(write_data);
        free(read_data);

        // Synchronize and close the file
        lab2_fsync(fd);
        lab2_close(fd);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
