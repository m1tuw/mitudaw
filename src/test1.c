#include <math.h>
#include <spa/param/audio/format-utils.h> // SPA: simple plugin api
#include <pipewire/pipewire.h>
#include "keymaps.h"
#include "notes.h" 
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
// event2 is keyboard

#define M_PI_M2 ( M_PI + M_PI )

#define DEFAULT_RATE            44100
#define DEFAULT_CHANNELS        2
#define DEFAULT_VOLUME          0.7
#define DEFAULT_VIBRATO         50

double keymap[256];

struct data {
    struct pw_main_loop *loop;
    struct pw_stream *stream;
    double accumulator; // accumulator keeps track of the phase of the soundwave
    double vibrato;
};

static int current_key = -1;

void init_freqs(double *note_frequencies){
    note_frequencies[16] = 261.63;  // Q -> C4
    note_frequencies[17] = 293.66;  // W -> D4
    note_frequencies[18] = 329.63;  // E -> E4
    note_frequencies[19] = 349.23;  // R -> F4
    note_frequencies[20] = 392.00;  // T -> G4
    note_frequencies[21] = 440.00;  // Y -> A4
    note_frequencies[22] = 493.88;  // U -> B4
    note_frequencies[23] = 523.25;  // I -> C5
    note_frequencies[24] = 587.33;  // O -> D5
    note_frequencies[25] = 659.25;  // P -> E5
    note_frequencies[26] = 698.46;  // [ -> F5
    note_frequencies[27] = 783.99;  // ] -> G5
    note_frequencies[28] = 880.00;  // \ -> A5
    note_frequencies[3] = 277.18;  // 2 -> C#4
    note_frequencies[4] = 311.13;  // 3 -> D#4
    note_frequencies[6] = 369.99;  // 5 -> F#4
    note_frequencies[7] = 415.30;  // 6 -> G#4
    note_frequencies[8] = 466.16;  // 7 -> A#4
    note_frequencies[10] = 554.37;  // 9 -> C#5
    note_frequencies[11] = 622.25;  // 0 -> D#5
    note_frequencies[13] = 739.99;  // ¿ -> F#5

    // bottom row
    note_frequencies[44] = 130.81;  // Z -> C3
    note_frequencies[31] = 138.59;  // S -> C#3
    note_frequencies[45] = 146.83;  // X -> D3
    note_frequencies[32] = 155.56;  // D -> D#3
    note_frequencies[46] = 164.81;  // C -> E3
    note_frequencies[47] = 174.61;  // V -> F3
    note_frequencies[34] = 185.00;  // B -> F#3
    note_frequencies[48] = 196.00;  // H -> G3
    note_frequencies[35] = 207.65;  // J -> G#3
    note_frequencies[49] = 220.00;  // K -> A3
    note_frequencies[36] = 233.08;  // L -> A#3
    note_frequencies[50] = 246.94;  // Ñ -> B3
    note_frequencies[51] = 261.63;  // M -> C4
}

void readkb(void *args, int* curr, int* is_held){
    int fd = *((int*)args);
    struct input_event ev;
    
    ssize_t n = read(fd, &ev, sizeof(ev));
    if (n == (ssize_t)-1) {
        perror("Failed to read event");
        return;
    }

    // Check if the event is a key press/release event
    if (ev.type == EV_KEY) {
        *is_held = ev.value;
        *curr = ev.code;
    }
}

void *keypress_handler(void *args){
    printf("keypress handler is running.");
    int curr = -1;
    int is_held = 0;
    while(1){
        readkb(args, &curr, &is_held);
        if(is_held){
            current_key = curr;
        }
        else{
            current_key = -1;
        }
        //printf("current key: %d\n", current_key);
    }
    return NULL;
}

double square_wave(double x){
    if(x >= 0 && x <= M_PI){
        return 1.0;
    }
    else{
        return -1.0;
    }
}

// to: pointer to buffer where we're writing at
static void write_sound(double frequency, int16_t *dst, struct data *data, int n_frames){
    for (int i = 0; i < n_frames; i++) {
        data->vibrato += M_PI_M2 * DEFAULT_VIBRATO / DEFAULT_RATE;
        if (data->vibrato >= M_PI_M2)
            data->vibrato -= M_PI_M2;
        data->accumulator += M_PI_M2 * (2*frequency + (frequency>0.0?(sin(data->vibrato))*10 : 0.0)) / DEFAULT_RATE;
        if (data->accumulator >= M_PI_M2)
            data->accumulator -= M_PI_M2;

        /* sin() gives a value between -1.0 and 1.0, we first apply
            * the volume and then scale with 32767.0 to get a 16 bits value
            * between [-32767 32767].
            * Another common method to convert a double to
            * 16 bits is to multiple by 32768.0 and then clamp to
            * [-32768 32767] to get the full 16 bits range. */
        int16_t val = square_wave(data->accumulator) * DEFAULT_VOLUME * 32767.0;
        for (int c = 0; c < DEFAULT_CHANNELS; c++)
            *dst++ = val;
    }
}

static float on_key_press(int ch) {
    return keymap[ch];
}

static void on_process(void *userdata){
    struct data *data = userdata;
    struct pw_buffer *b;
    struct spa_buffer *buf;
    int n_frames, stride;
    int16_t *dst;

    if ((b = pw_stream_dequeue_buffer(data->stream)) == NULL) {
        pw_log_warn("out of buffers: %m");
        return;
    }

    buf = b->buffer;
    if ((dst = buf->datas[0].data) == NULL)
        return;

    stride = sizeof(int16_t) * DEFAULT_CHANNELS;
    n_frames = buf->datas[0].maxsize / stride;
    if (b->requested)
        n_frames = SPA_MIN(b->requested, n_frames);

    double freq = on_key_press(current_key);
    write_sound(freq, dst, data, n_frames);

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = stride;
    buf->datas[0].chunk->size = n_frames * stride;

    pw_stream_queue_buffer(data->stream, b);
}
 
static const struct pw_stream_events stream_events = {
        PW_VERSION_STREAM_EVENTS,
        .process = on_process,
};
 
int main(int argc, char *argv[]){
    // key mapping
    init_freqs(keymap);

    // disable echo
    struct termios old, nw;
    tcgetattr(STDIN_FILENO, &old);
    nw = old;

    nw.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &nw);

    // pipewire initialization
    struct data data = { 0, };
    const struct spa_pod *params[1];
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    pw_init(&argc, &argv);

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

    // keypress handling code
    int fd = open("/dev/input/event2", O_RDONLY);
    if(fd == -1){
        perror("failed to open device");
        return 1;
    }

    pthread_t kbthread;
    if (pthread_create(&kbthread, NULL, keypress_handler, &fd) != 0) {
        perror("Failed to create thread");
        close(fd);
        return 1;
    }

    pw_main_loop_run(data.loop);

    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    pthread_join(kbthread, NULL);
    close(fd);
    pw_stream_destroy(data.stream);
    pw_main_loop_destroy(data.loop);

    return 0;
}