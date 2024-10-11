/* 
*   HEADER FILES
*/
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h> // SPA: simple plugin api
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "callbacks.h"
#include "soundwaves.h"
/*
*   DEFINITIONS
*/
#define SHM_NAME "/current_key"

/*
*   GLOBAL VARIABLES (don't ask why idrc)
*/
static struct termios old_termios, new_termios;

static void run_disable_echo(){
    tcgetattr(STDIN_FILENO, &old_termios);
    new_termios = old_termios;
    new_termios.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

static void run_enable_echo(){
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .process = on_process,
};

// BORRAR ESTO SOLO ES PARA VER SI FUNCIONA XD
int key = 0;
int *last = NULL;

int main(int argc, char *argv[]){
    run_disable_echo();
    //printf("A");
    // forks and pipes
    struct data data = { 0, };
    pid_t pid;

    // fork
    pid = fork();
    if(pid < 0){ // handle bad case
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    if(pid == 0){
        execl("./keyboardInputHandler", "keyboardInputHandler", NULL);
    }
    else{ // good case (parent)
        int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("shm_open failed");
            return 1;
        }
        last = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (last == MAP_FAILED) {
            perror("mmap failed");
            return 1;
        }

        data.last = last;
    }

    // initialize variables
    const struct spa_pod *params[1];
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    // initialize pipewire
    pw_init(&argc, &argv);
    
    // initialize data
    struct effect *slowvib = create_vibrato(7, 10, 0);
    struct effect *modulator = create_vibrato(10, 70, 0);
    struct pipelineNode *pl = create_pipelineNode(slowvib);
    add_pipelineNode(modulator, pl);
    data.pipeline = pl;

    data.waveform = square_wave;
    data.loop = pw_main_loop_new(NULL);
    data.stream = pw_stream_new_simple(
        pw_main_loop_get_loop(data.loop),
        "audio-src",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Playback",
            PW_KEY_MEDIA_ROLE, "Music",
            NULL),
        &stream_events,
        &data);

    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat,
        &SPA_AUDIO_INFO_RAW_INIT(
            .format = SPA_AUDIO_FORMAT_S16,
            .channels = DEFAULT_CHANNELS,
            .rate = DEFAULT_RATE ));

    pw_stream_connect(data.stream,
        PW_DIRECTION_OUTPUT,
        PW_ID_ANY,
        PW_STREAM_FLAG_AUTOCONNECT |
        PW_STREAM_FLAG_MAP_BUFFERS |
        PW_STREAM_FLAG_RT_PROCESS,
        params, 1);

    pw_main_loop_run(data.loop);
    
    munmap(last, sizeof(int)); // Cleanup
    shm_unlink(SHM_NAME); // Remove shared memory object
    run_enable_echo();
    pw_stream_destroy(data.stream);
    pw_main_loop_destroy(data.loop);
    // run main loop

    //run_enable_echo();
    return 0;
}