#include "util.h"
#include "config.h"
#include "freqmap.h"

double freqs[256];
int ok = 0;

void on_process(void *userdata){
    if(!ok){
        init_freqs(freqs);
        ok = 1;
    }
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
    if ((dst = buf->datas[0].data) == NULL){
        return;
    }

    stride = sizeof(int16_t) * DEFAULT_CHANNELS;
    n_frames = buf->datas[0].maxsize / stride;
    if (b->requested){
        n_frames = SPA_MIN(b->requested, n_frames);
    }

    int key = -1;
    if(data->last){
        key = *(data->last);
    }
        
    double freq_default = freqs[key]*2;
    //printf("%f\n", freq_default);
    //printf("%d\n", key);
    double amplitude_default = 0.5;
    double phase_default = 0.0;

    double freq = freq_default;
    double amplitude = amplitude_default;
    double phase = phase_default;
    // do something with the pipeline i guess
    struct param_controller params;
    params.freq = &freq;
    params.amplitude = &amplitude;
    params.phase = &phase;

    struct pipelineNode *pl = data->pipeline;
    for (int i = 0; i < n_frames; i++) {
        int16_t total = 0;
        
        // problema: hacer esto para todas las keys presionadas es horrible
        // hay que crear tantos sonidos como keys haya
        // todo: limitar las teclas posibles hasta 5 y crear 5 ondas distintas
        freq = freq_default;
        amplitude = amplitude_default;
        phase = phase_default;

        struct pipelineNode *curr = pl;
        while(curr != NULL){
            struct effect *ef = curr->effect;
            (ef->apply)(params, ef->args);
            curr = curr->next;
        }
        data->accumulator += M_PI_M2 * freq / DEFAULT_RATE;
        if (data->accumulator >= M_PI_M2)
            data->accumulator -= M_PI_M2;

        int16_t val = (data->waveform)(data->accumulator) * amplitude * 32767.0;

        total += val;
        for (int c = 0; c < DEFAULT_CHANNELS; c++)
            *dst++ = val;
    }

    buf->datas[0].chunk->offset = 0;
    buf->datas[0].chunk->stride = stride;
    buf->datas[0].chunk->size = n_frames * stride;

    pw_stream_queue_buffer(data->stream, b);
}