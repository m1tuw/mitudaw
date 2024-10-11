#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>        // For shm_open
#include <stdlib.h>

#define SHM_NAME "/current_key" // Name for the shared memory object

unsigned char key_states[32];
int isHeld = 0;

#define NUM_KEYS 256
#define BYTE_SIZE 8

void readkb(int fd, int *last) {
    struct input_event ev;

    ssize_t n = read(fd, &ev, sizeof(ev));
    if (n == (ssize_t)-1) {
        perror("Failed to read event");
        return;
    }

    // Handle only key press events
    if (ev.type == EV_KEY) {
        isHeld = ev.value;
        if(isHeld != 0){
            *last = ev.code;
        }
        else{
            *last = 0;
        }
    }
}

int main() {
    const char *device = "/dev/input/event2";
    int fd = open(device, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open input device");
        return 1;
    }

    // Create shared memory for the variable 'last'
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return 1;
    }
    ftruncate(shm_fd, sizeof(int)); // Set size
    int *last = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (last == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    while (1) {
        readkb(fd, last);
        // You can also add logic to notify the main process if needed
    }

    munmap(last, sizeof(int)); // Cleanup
    close(fd);
    return 0;
}
