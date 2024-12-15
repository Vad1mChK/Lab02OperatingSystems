#include "lab2_library.hpp"
#include <iostream>
#include <cstring>

int main() {
    try {
        fd_t fd = lab2_open("testfile.txt");

        char write_data[4096];
        for (size_t i = 0; i < 4096; ++i) {
            write_data[i] = (i % 32 == 31) ? '\n' : (i & 1) ? 'H' : 'h';
        }

        lab2_write(fd, write_data, sizeof(write_data));

        lab2_lseek(fd, 0, SEEK_SET);

        char read_data[64];
        lab2_read(fd, read_data, sizeof(write_data));

        std::cout << "Read data: " << read_data << std::endl;

        lab2_fsync(fd);
        lab2_close(fd);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}