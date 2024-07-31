#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* extract_filename_prefix(const char* path) {
    // Find the last occurrence of '/'
    const char* last_slash = strrchr(path, '\\');
    const char* last_forwad = strrchr(path, '/');
    if (!last_slash) {
        if(!last_forwad) {
            last_slash = path;
        } else {
            last_slash = last_forwad+1; 
        }// No '/' found, use the whole string
    } else {
        last_slash++; // Move past the '/'
    }

    // Find the first occurrence of '.'
    const char* first_dot = strchr(last_slash, '.');
    if (!first_dot) {
        return strdup(last_slash); // No '.' found, return the whole string after the last '/'
    }

    // Calculate the length of the prefix
    size_t prefix_length = first_dot - last_slash;

    // Allocate memory for the new string
    char* prefix = (char*)malloc(prefix_length + 1);
    if (!prefix) {
        return NULL; // Handle memory allocation failure
    }

    // Copy the prefix to the new string
    strncpy(prefix, last_slash, prefix_length);
    prefix[prefix_length] = '\0'; // Null-terminate the new string

    return prefix;
}
void generateHeader(const char* shaderName) {
    printf("Generating for %s...\n", shaderName);
    FILE* shader = fopen(shaderName, "r");
    if(!shader) {
        fprintf(stderr, "Failed to open file %s, aborting.\n", shaderName);
        exit(EXIT_FAILURE);
    }
    int fileSize;
    fseek(shader, 0, SEEK_END);
    fileSize = ftell(shader);
    fseek(shader, 0, SEEK_SET);

    uint8_t* buffer = malloc(sizeof(uint8_t) * fileSize);
    if(!buffer) {
        fprintf(stderr, "Failed to allocate buffer, aborting.\n");
        exit(EXIT_FAILURE);
    }
    fread(buffer, 1, fileSize, shader);
    fclose(shader);
    if(fileSize%4 != 0) {
        fprintf(stderr, "File is %d bytes and thus is not 6 byte aligned, aborting.", fileSize);
        exit(EXIT_FAILURE);
    }
     

       char* lastPrefix = extract_filename_prefix(shaderName);

        if(shaderName[1] == '.') {
            char* lastBSlash = strrchr(shaderName, '\\');
            char* lastFSlash = strrchr(shaderName, '/');
        
            char* lastSlash = !lastBSlash ? lastFSlash : lastBSlash;
            int indexOfSlash = lastSlash - shaderName;
            int lengthOfPostfix = strlen(lastPrefix);
            printf("indexofSlash:%d, lengthOfPostfix%d\n", indexOfSlash, lengthOfPostfix);
            char* newName = malloc(indexOfSlash+1+lengthOfPostfix+3);
            strncpy(newName, shaderName, indexOfSlash+1);
            printf("firstpart: %s\n", newName);
            strcpy(newName+indexOfSlash+1, lastPrefix);
            newName[indexOfSlash+lengthOfPostfix+1] = '.';
            newName[indexOfSlash+lengthOfPostfix+2] = 'h';
            newName[indexOfSlash+lengthOfPostfix+3] = '\0';
            free(lastPrefix);
            lastPrefix = newName;
        }
    char* headerName = lastPrefix; 
    FILE* header = fopen(headerName, "w");
     if(!header) {
        fprintf(stderr, "Failed to create file %s, aborting.\n", headerName);
        exit(EXIT_FAILURE);
    }
     char* lastBSlash = strrchr(headerName, '\\');
            char* lastFSlash = strrchr(headerName, '/');
        
            char* lastSlash = !lastBSlash ? lastFSlash : lastBSlash;
            int indexOfSlash = lastSlash - headerName;
    char* lastPeriod = strrchr(headerName, '.');
    int length = lastPeriod - lastSlash -1;
    char* listName = malloc(length+1);
    strncpy(listName, &headerName[indexOfSlash+1],length);
    listName[length] = '\0';
    fprintf(header, "#pragma once\n");
    fprintf(header, "#include <stdint.h>\n");
    fprintf(header, "const uint32_t %sShaderByteCode[] = {\n", listName);
    for(int i = 0; i < fileSize; i++) {
        uint32_t word = buffer[i] | (buffer[i + 1] << 8) | (buffer[i + 2] << 16) | (buffer[i + 3] << 24);
        fprintf(header, "    0x%08x,\n", word);
    }
    fprintf(header, "};");
    fclose(header);
    printf("Generated %s\n", headerName);
    free(headerName);
    free(listName);

}
int main(int argc, char* argv[]) {
    if(argc == 0) {
        printf("Please input a spirv filename.\n");
    }
    for(int i = 1; i < argc; i++) {
        generateHeader(argv[i]);
    }
    return 0;
}
