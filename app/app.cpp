#include "lab2_library.hpp"
#include <iostream>
#include <cstring>

int main()
{
    Lab2 lab2(/* cacheCapacity = */ 8, /* blockSize = */ LAB2_BLOCK_SIZE);

    const fd_t fd = lab2.open("/home/vad1mchk/testfile.txt");
    if (fd < 0) {
        std::cerr << "Open failed\n";
        return 1;
    }

    const char* data = "Hello from Lab2!\n";
    ssize_t w = lab2.write(fd, data, std::strlen(data));
    std::cout << "Wrote " << w << " bytes.\n";

    // Move file offset to beginning
    lab2.lseek(fd, 0, SEEK_SET);

    char buf[100] = {0};
    ssize_t r = lab2.read(fd, buf, sizeof(buf) - 1);
    std::cout << "Read  " << r << " bytes: '" << buf << "'\n";

    lab2.fsync(fd);
    lab2.close(fd);

    return 0;
}
