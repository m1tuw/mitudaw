#include <math.h>
#include <spa/param/audio/format-utils.h> // SPA: simple plugin api
#include <pipewire/pipewire.h>
#include "keymaps.h"
#include "notes.h" 
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <pthread.h>
// event2 is keyboard

#define M_PI_M2 ( M_PI + M_PI )

#define DEFAULT_RATE            44100
#define DEFAULT_CHANNELS        2
#define DEFAULT_VOLUME          0.7

struct data {
    struct pw_main_loop *loop;
    struct pw_stream *stream;
    double accumulator; // accumulator keeps track of the phase of the soundwave
};

static int current_key = -1;

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
        printf("current key: %d\n", current_key);
    }
    return NULL;
}

// to: pointer to buffer where we're writing at
static void write_sound(double frequency, int16_t *dst, struct data *data, int n_frames){
    for (int i = 0; i < n_frames; i++) {
        data->accumulator += M_PI_M2 * frequency / DEFAULT_RATE;
        if (data->accumulator >= M_PI_M2)
            data->accumulator -= M_PI_M2;

        /* sin() gives a value between -1.0 and 1.0, we first apply
            * the volume and then scale with 32767.0 to get a 16 bits value
            * between [-32767 32767].
            * Another common method to convert a double to
            * 16 bits is to multiple by 32768.0 and then clamp to
            * [-32768 32767] to get the full 16 bits range. */
        int16_t val = sin(data->accumulator) * DEFAULT_VOLUME * 32767.0;
        for (int c = 0; c < DEFAULT_CHANNELS; c++)
            *dst++ = val;
    }
}

static float on_key_press(int ch) {
    switch(ch){
        // naturals
        case KEY_KB_Q:
            return FREQ_C4;
        case KEY_KB_W:
            return FREQ_D4;
        case KEY_KB_E:
            return FREQ_E4;
        case KEY_KB_R:
            return FREQ_F4;
        case KEY_KB_T:
            return FREQ_G4;
        case KEY_KB_Y:
            return FREQ_A4;
        case KEY_KB_U:
            return FREQ_B4;
        case KEY_KB_I:
            return FREQ_C5;
        case KEY_KB_O:
            return FREQ_D5;
        case KEY_KB_P:
            return FREQ_E5;
        case 26:
            return FREQ_F5;
        case 27:
            return FREQ_G5;
        // sharps
        case KEY_KB_2:
            return FREQ_C4S;
        case KEY_KB_3:
            return FREQ_D4S;
        case KEY_KB_5:
            return FREQ_F4S;
        case KEY_KB_6:
            return FREQ_G4S;
        case KEY_KB_7:
            return FREQ_A4S;
        case KEY_KB_9:
            return FREQ_C5S;
        case KEY_KB_0:
            return FREQ_D5S;
        case KEY_KB_0 + 2:
            return FREQ_F5S;
        case -1:
            return 0;
        default:
            return 0;
    }
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

    pthread_join(kbthread, NULL);
    close(fd);
    pw_stream_destroy(data.stream);
    pw_main_loop_destroy(data.loop);

    return 0;
}