#define MEMSHARENAME      "memshare"

// A 7-byte struct for the shared memory.
struct __attribute__((__packed__)) shm_struct {
    unsigned up:1;                // D-pad Up.
    unsigned down:1;              // D-pad Down.
    unsigned left:1;              // D-pad Left.
    unsigned right:1;             // D-pad Right.
    unsigned start:1;             // Start button.
    unsigned back:1;              // Back button.
    unsigned lpress:1;            // Left Joy-stick press.
    unsigned rpress:1;            // Right Joy-stick press.
    unsigned lb:1;                // LB button.
    unsigned rb:1;                // RB button.
    unsigned logo:1;              // Xbox Logo.
    unsigned a:1;                 // A button.
    unsigned b:1;                 // B button.
    unsigned x:1;                 // X button.
    unsigned y:1;                 // Y button.
    unsigned connected:1;         // Device Connection State.
    unsigned char ltrig;          // Left Trigger.
    unsigned char rtrig;          // Right Trigger.
    unsigned char ledpat;         // LED Pattern.
    unsigned char lightrumble;    // Light Rumbler.
    unsigned char heavyrumble;    // Heavy Rumbler.
};

// Get the shared memory handle.
unsigned char getSharedMemoryHandle(int* shm_fildes, char* shm_name, int shm_flags);

// Set the shared memory size.
unsigned char setSharedMemorySize(int* shm_fildes, int shm_len);

// Map the shared memory.
unsigned char mapSharedMemory(int shm_fildes, int shm_len, struct shm_struct** shm_data);

// Unmap the shared memory.
unsigned char unmapSharedMemory(int shm_fildes, struct shm_struct** shm_data, int shm_len);

// Unlink the shared memory.
unsigned char unlinkSharedMemory(char* shm_name);