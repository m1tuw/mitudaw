#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>


// Function to set or clear a specific bit in the bitmask
void set_key_state(int keycode, int pressed){
    if(pressed){
        key_state[keycode / BYTE_SIZE] |= (1 << (keycode % BYTE_SIZE));  // Set bit
    }
    else{
        key_state[keycode / BYTE_SIZE] &= ~(1 << (keycode % BYTE_SIZE));  // Clear bit
    }
}

// Function to get the state of a specific key
int get_key_state(int keycode){
    return (key_state[keycode / BYTE_SIZE] >> (keycode % BYTE_SIZE)) & 1;
}

/*
    this function is intended to run in parallel
    with the main loop.
*/
void *keypress_handler(void *args, unsigned char *key_state){
    printf("keypress handler is running.");
    while(1){
        
    }
    return NULL;
}

int main(){
    
}