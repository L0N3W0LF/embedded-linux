#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include "shmgamepad.h"         // For the shared memory.
#include <fcntl.h>              // For O_* constants.

#define XBOXCONTROLLER    0x045e
#define MICROSOFT         0x028e
#define INPUTENDPOINT     0x02
#define OUTPUTENDPOINT    0x81
#define DEVICEOPEN        1
#define DEVICECLOSED      0

// A 20-byte struct for the input report.
struct __attribute__((__packed__)) data {
    unsigned char type;           // Type of message.
    unsigned char lenght;         // Lenght of message.
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
    unsigned unusedb:1;
    unsigned a:1;                 // A button.
    unsigned b:1;                 // B button.
    unsigned x:1;                 // X button.
    unsigned y:1;                 // Y button.
    unsigned char ltrig;          // Left Trigger.
    unsigned char rtrig;          // Right Trigger.
    unsigned char l_xaxis1;       // Left Joy-stick X-axis.
    unsigned char l_xaxis2;       // Left Joy-stick X-axis.
    unsigned char l_yaxis1;       // Left Joy-stick Y-axis.
    unsigned char l_yaxis2;       // Left Joy-stick Y-axis.
    unsigned char r_xaxis1;       // Right Joy-stick X-axis.
    unsigned char r_xaxis2;       // Right Joy-stick X-axis.
    unsigned char r_yaxis1;       // Right Joy-stick Y-axis.
    unsigned char r_yaxis2;       // Right Joy-stick Y-axis.
    unsigned char unused[6];
};

// This function will log an error to the logfile.
void writeToLogfile(char* msg, int var) {
    FILE *logFile = fopen("/var/logs/gamepadd.log", "a+");          // Open the logfile.
    if (logFile != NULL) {
        time_t e_time = time(NULL);                                 // Get the epoch time.
        fprintf(logFile, "%s:%i @ %s", msg, var, ctime(&e_time));   // Print the msg, var, and convert epoch time to a string.
        fclose(logFile);                                            // Close the logfile.
    }
}

// This takes a device handle, data[], lenght of data[], and performs a usb interrupt transfer to the output endpoint 0x02.
char readFromController(libusb_device_handle *h, unsigned char *data, int lenght) {
    int error, transferred;
    if ((error = libusb_interrupt_transfer(h, OUTPUTENDPOINT, data, lenght, &transferred, 0))) {
        if (error == LIBUSB_ERROR_NO_DEVICE) {
            return DEVICECLOSED;   // If the error is of no device, the device is closed.
        } else {
            // If the error is not of no device, log the error and exit.
            writeToLogfile("Transfer failed (REPORT)", error);
            exit(1);
        }
    }
    return DEVICEOPEN;      // If there are no errors, the device is open.
}

// Write the bytes to memory.
void writeStateToSharedMemory(struct shm_struct* shmgamepad, const struct data deviceState) {
    (*shmgamepad).up = deviceState.up;              // D-pad Up.
    (*shmgamepad).down = deviceState.down;          // D-pad Down.
    (*shmgamepad).left = deviceState.left;          // D-pad Left.
    (*shmgamepad).right = deviceState.right;        // D-pad Right.
    (*shmgamepad).start = deviceState.start;        // Start button.
    (*shmgamepad).back = deviceState.back;          // Back button.
    (*shmgamepad).lpress = deviceState.lpress;      // Left Joy-stick press.
    (*shmgamepad).rpress = deviceState.rpress;      // Right Joy-stick press.
    (*shmgamepad).lb = deviceState.lb;              // LB button.
    (*shmgamepad).rb = deviceState.rb;              // RB button.
    (*shmgamepad).logo = deviceState.logo;          // Xbox Logo.
    (*shmgamepad).a = deviceState.a;                // A button.
    (*shmgamepad).b = deviceState.b;                // B button.
    (*shmgamepad).x = deviceState.x;                // X button.
    (*shmgamepad).y = deviceState.y;                // Y button.
    (*shmgamepad).ltrig = deviceState.ltrig;        // Left Trigger.
    (*shmgamepad).rtrig = deviceState.rtrig;        // Right Trigger.
}

// This function is for setting the pattern of the light ring on the controller.
void writeToLightRing(libusb_device_handle *h, unsigned char pattern) {
    int error, transferred;
    unsigned char data[] = {1,3, pattern};              // The data to send to the device to change LED patterns.
    if ((error = libusb_interrupt_transfer(h, INPUTENDPOINT, data, sizeof(data), &transferred, 0))) {
        // If there is an error, log the error and exit.
        writeToLogfile("Transfer failed (LED)", error);
        exit(1);
    }
}

// This function is used to set the speed of the light weight and heavy weight rumblers.
void writeToRumbler(libusb_device_handle *h, unsigned char heavy, unsigned char light) {
    int error, transferred;
    unsigned char data[] = {0,8,0, heavy, light,0,0,0}; // The data to send to the device to change rumble.
    if ((error = libusb_interrupt_transfer(h, INPUTENDPOINT, data, sizeof(data), &transferred, 0))) {
        // If there is an error, log the error and exit.
        writeToLogfile("Transfer failed (RUMBLE)", error);
        exit(1);
    }
}

// Run the routines to initialize the shared memory.
void openSharedMemory(int* shm_fildes, int shm_size, struct shm_struct** shm, char* shm_name) {
        // Create a shared memory.
    if (getSharedMemoryHandle(shm_fildes, shm_name, O_CREAT | O_RDWR) == 0) {
        writeToLogfile("Could not get the shared memory handle", errno);
        exit(1);
    }
    // Set the shared memory size.
    if (setSharedMemorySize(shm_fildes, shm_size) == 0) {
        writeToLogfile("Could not set the shared memory size", errno);
        exit(1);
    }
    // Map the shared memory.
    if (mapSharedMemory(*shm_fildes, shm_size, shm) == 0) {
        writeToLogfile("Could not map the shared memory", errno);
        exit(1);
    }
}

// Run the routines to setup a deamon.
void deamonize() {
    int pid = fork();               // Fork the parrent.
    if(pid < 0) {
        // If the fork failed, log the error and exit the parrent.
        writeToLogfile("Could not fork the process", errno);
        exit(1);
    }
    if (pid > 0) {
        // If the fork is created, exit the parrent.
        writeToLogfile("PID", pid);
        exit(0);
    }
    umask(0);                   // Specify new file permissions.
    if (setsid() < 0) {         // Set the session id.
        writeToLogfile("Could not set new ID for child", errno);
        exit(1);                // Exit the child, could not set new ID.
    }
    chdir("/");                 // Change the working directory to root.
    // Close STDIN, STDOUT, STDERR.
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int main() {
    libusb_device_handle *h = NULL;                             // Pointer to the device.
    unsigned char deviceConnection = DEVICECLOSED;              // The state of the device.
    unsigned char f_led = 0, f_lrumble = 0, f_hrumble = 0;      // Flags for the LED, light rumble, and heavy rumble.
    int result, fildesgamepad;                                  // To hold return values.
    struct data deviceState;                                    // To hold the input report.
    struct shm_struct *shmgamepad = NULL;                       // Structure of the shared memory.
    libusb_init(NULL);                                          // Initialize the library (for a session), NULL == default session.

    // Deamonize the application.
    deamonize();

    // Initialize the shared memory.
    openSharedMemory(&fildesgamepad, sizeof(struct shm_struct), &shmgamepad, MEMSHARENAME);

    while(1)
    {
        // Check if the gamepad controller is open.
        if (deviceConnection == DEVICECLOSED) {
            // If the device is not open, try to open the it.
            if ((h = libusb_open_device_with_vid_pid(NULL, XBOXCONTROLLER, MICROSOFT)) != NULL) {
                deviceConnection = DEVICEOPEN;          // Set the device as open.
                (*shmgamepad).connected = DEVICEOPEN;   // Write to shared memory that device is closed.
            }
        } else {
            // When the device is open, get the state from the input report.
            result = readFromController(h, (unsigned char*)&deviceState, sizeof(deviceState));
            if (result == DEVICEOPEN) {
                    // If the device is open, write the state to the shared memory.
                    writeStateToSharedMemory(shmgamepad, deviceState);

                    // Read the led pattern and rumbler settings from the shared memory.
                    // (Note: rotation is bugged because of timeout).
                    if ((*shmgamepad).ledpat != f_led) {
                        writeToLightRing(h, (*shmgamepad).ledpat);  // If the pattern has changed, update device.
                        f_led = (*shmgamepad).ledpat;               // Update the led pattern flag.
                    }
                    if ((*shmgamepad).heavyrumble != f_hrumble || (*shmgamepad).lightrumble != f_lrumble) {
                        writeToRumbler(h, (*shmgamepad).heavyrumble, (*shmgamepad).lightrumble);    // If rumble speed has changed, update device.
                        f_hrumble = (*shmgamepad).heavyrumble;      // Update the heavy rumble flag.
                        f_lrumble = (*shmgamepad).lightrumble;      // Update the light rumble flag.
                    }
            } else {
                // If result is not 1, the device has been disconnected.
                deviceConnection = DEVICECLOSED;                    // Set the device as closed.
                (*shmgamepad).connected = DEVICECLOSED;             // Write to shared memory that device is closed.
                h = NULL;                                           // Set the handle to null.

                // When the device is disconected, reset the flags, LED pattern, and rumble.
                (*shmgamepad).ledpat = f_led = 0x01;                // The default pattern is blinking.
                (*shmgamepad).lightrumble = f_lrumble = 0x00;       // Light rumble speed 0.
                (*shmgamepad).heavyrumble = f_hrumble = 0x00;       // Heavy rumble speed 0.
            }
        }
    }
}