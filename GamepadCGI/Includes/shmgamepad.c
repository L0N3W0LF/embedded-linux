#include <sys/stat.h>      // For MODE constants.
#include <sys/mman.h>      // For memory management.
#include <unistd.h>
#include "shmgamepad.h"

// Get the shared memory handle.
unsigned char getSharedMemoryHandle(int* shm_fildes, char* shm_name, int shm_flags) {
    if((*shm_fildes = shm_open(shm_name, shm_flags, 0666)) == -1) {
        return 0;
    }
    return 1;
}

// Set the shared memory size.
unsigned char setSharedMemorySize(int* shm_fildes, int shm_len) {
    if((ftruncate(*shm_fildes, shm_len) != 0)) {
        return 0;
    }
    return 1;
}

// Map the shared memory.
unsigned char mapSharedMemory(int shm_fildes, int shm_len, struct shm_struct** shm_data) {
    if ((*shm_data = mmap(0, 7, PROT_WRITE, MAP_SHARED, shm_fildes, 0)) == MAP_FAILED) {
        return 0;
    }
    return 1;
}

// Unmap the shared memory.
unsigned char unmapSharedMemory(int shm_fildes, struct shm_struct** shm_data, int shm_len) {
    // Unmap the shared memory.
    if (munmap(shm_data, shm_len) != 0) {    // Returns 0 on succes, -1 on fail.
        return 0;
    }
    // Close the file descriptor.
    if (close(shm_fildes) != 0) {           // Returns 0 on succes, -1 on fail.
        return 0;
    }
    return 1;
}

// Unlink the shared memory.
unsigned char unlinkSharedMemory(char* shm_name) {
    // Unlink the shared memory.
    if (unlink(shm_name) != 0) {            // Returns 0 on succes, -1 on fail.
        return 0;
    }
    return 1;
}