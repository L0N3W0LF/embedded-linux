#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include "shmgamepad.h"         // For the shared memory.
#include <fcntl.h>              // For O_* constants.

#define DEVICEOPEN        1
#define DEVICECLOSED      0

// Find the value of a query variable.
int getQueryVariable(const char* query_string, const unsigned char len, const char* varname) {
    // Make a copy of the query string.
    char* buffer = (char*)malloc(len+1);
    strcpy(buffer, query_string);

    // Get the value=variable.
    char* var = strtok(buffer, "&=");
    char* val = strtok(NULL, "&=");
    while(var != NULL && val != NULL) {
        // Check if the is variable is what we are looking for.
        if (strcmp(var, varname) == 0) {
            // If variable is whats we are looking for return the value.
            free(buffer);       // Free the buffer before exit.
            return atoi(val);
        }
        // Get the next value=variable.
        var = strtok(NULL, "&=");
        val = strtok(NULL, "&=");
    }
    free(buffer);   // Free the buffer before exit.
    return -1;
}

// Write the bytes to memory.
void writeStateToSharedMemory(struct shm_struct* shmgamepad) {
    // Response format (in decimals).
    // Name:  logo:a:b:x:y:up:down:left:right:start:back:lpress:rpress:lb:rb:ltrig:rtrig.
    // Bytes:    1:1:1:1:1: 1:   1:   1:    1:    1:   1:     1:     1: 1: 1:    3:    3.
    printf("%1d", (*shmgamepad).logo);              // Xbox Logo.
    printf("%1d", (*shmgamepad).a);                 // A button.
    printf("%1d", (*shmgamepad).b);                 // B button.
    printf("%1d", (*shmgamepad).x);                 // X button.
    printf("%1d", (*shmgamepad).y);                 // Y button.
    printf("%1d", (*shmgamepad).up);                // D-pad Up.
    printf("%1d", (*shmgamepad).down);              // D-pad Down.
    printf("%1d", (*shmgamepad).left);              // D-pad Left.
    printf("%1d", (*shmgamepad).right);             // D-pad Right.
    printf("%1d", (*shmgamepad).start);             // Start button.
    printf("%1d", (*shmgamepad).back);              // Back button.
    printf("%1d", (*shmgamepad).lpress);            // Left Joy-stick press.
    printf("%1d", (*shmgamepad).rpress);            // Right Joy-stick press.
    printf("%1d", (*shmgamepad).lb);                // LB button.
    printf("%1d", (*shmgamepad).rb);                // RB button.
    printf("%3d", (*shmgamepad).ltrig);             // Left Trigger.
    printf("%3d", (*shmgamepad).rtrig);             // Right Trigger.
}

// Run the routines to initialize the shared memory.
void openSharedMemory(int* shm_fildes, int shm_size, struct shm_struct** shm, char* shm_name) {
    // Create a shared memory.
    if (getSharedMemoryHandle(shm_fildes, shm_name, O_RDWR) == 0) {
        printf("FailedGetSharedMemory");
        exit(1);
    }
    // Map the shared memory.
    if (mapSharedMemory(*shm_fildes, shm_size, shm) == 0) {
        printf("FailedMapSharedMemory");
        exit(1);
    }
}

int main()
{
    int fildesgamepad;                             // File descriptor.
    struct shm_struct* shmgamepad = NULL;          // Structure of the shared memory.
    char* con_len;                                 // To hold the lenght of the query string.
    char* query_string;                            // To hold the query string.
    unsigned char query_lenght;
    int getstate, ledpattern, lrumble, hrumble;    // Hold variables for the actions.

    printf("Content-type: text/html\n\n");         // Set the encoding.
    con_len = getenv("CONTENT_LENGTH");            // Get the lenght.

    // Initialize the shared memory.
    openSharedMemory(&fildesgamepad, sizeof(struct shm_struct), &shmgamepad, MEMSHARENAME);

    // Check if the controller is not disconnected.
    if ((*shmgamepad).connected == 0) {
        printf("DeviceDisconnected");
        exit(1);
    }

    // Get the query string from stdin.
    if (con_len != NULL) {
        query_lenght = atoi(con_len);
        query_string = (char*)malloc(query_lenght+1);       // Allocate some space.
        scanf("%s", query_string);                          // Read query string.

        // Get the possible actions.
        getstate = getQueryVariable(query_string, query_lenght, "getstate");
        ledpattern = getQueryVariable(query_string, query_lenght, "ledpattern");
        lrumble = getQueryVariable(query_string, query_lenght, "lrumble");
        hrumble = getQueryVariable(query_string, query_lenght, "hrumble");

        // Do the requested actions.
        if (getstate == 1) {
            writeStateToSharedMemory(shmgamepad);
        }

        // Set the led pattern, possible patterns 0-13.
        if (ledpattern >= 0 && ledpattern <= 13) {
            (*shmgamepad).ledpat = ledpattern;
        }

        // Set the light rumble speed, possible speed 0-255.
        if (lrumble >= 0 && lrumble <= 255) {
            (*shmgamepad).lightrumble = lrumble;
        }

        // Set the heavy rumble speed, possible speed 0-255.
        if (hrumble >= 0 && hrumble <= 255) {
            (*shmgamepad).heavyrumble = hrumble;
        }
        // Free the allocated space.
        free(query_string);
    }
    return 0;
}