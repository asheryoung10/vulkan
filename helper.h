#include <stdint.h>
#include <stdio.h>
uint32_t clamp(uint32_t a, uint32_t min, uint32_t max) {
    if(a > max) {
        return max;
    } else if( a < min) {
        return min;
    } else {
        return a;
    }
};


#include <stdlib.h>

char* readFile(const char* filename, uint32_t* size) {
    FILE* file;
    errno_t err = fopen_s(&file, filename, "rb");
    if (err != 0 || !file) {
        perror("failed to open file");
        exit(EXIT_FAILURE);
    }

    // Seek to the end of the file
    fseek(file, 0, SEEK_END);

    // Get the size of the file
    long fileSize = ftell(file);
    *size = fileSize;
    // Seek back to the beginning of the file
    fseek(file, 0, SEEK_SET);

    // Allocate a buffer to hold the contents of the file
    char* buffer = (char*)malloc(fileSize);
    if (!buffer) {
        perror("failed to allocate buffer");
        exit(EXIT_FAILURE);
    }

    // Read the contents of the file into the buffer
    fread(buffer, 1, fileSize, file);

    // Close the file
    fclose(file);

    return buffer;
}

