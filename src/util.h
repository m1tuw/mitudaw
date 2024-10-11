#include <math.h>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h> // SPA: simple plugin api
#include "config.h"
#define M_PI_M2 ( M_PI + M_PI )
/*
*   UTILITY: STRUCTS AND MORE
*   pa ordenar el codigo :3
*
*/

/**
    param_controller: works like a board of pointers from which
    we can control the parameters of a soundwave
 */
struct param_controller {
    double *freq;
    double *amplitude;
    double *phase;
};

/**
    effect: tuple of arguments and a function that receives them.

    an effect modifies the frequency, amplitude and phase of a soundwave.
    it can also write new sound to the sample, but we will implement that later.

    apply is used to apply the given effect. if we need additional arguments
    like constants, we pass them through args. param_controller is where
    the changes will be actually reflected.
 */
struct effect{
    void *args;
    void (*apply)(struct param_controller, void *args);
};

void apply_vibrato(struct param_controller pc, void *args){
    double *params = args;
    if(*(pc.freq) == 0) return;
    double vibrato_amplitude = params[0];
    double vibrato_frequency = params[1];
    double vibrato_accumulator = params[3];

    vibrato_accumulator += M_PI_M2 * vibrato_frequency / DEFAULT_RATE;
    if(vibrato_accumulator >= M_PI_M2) vibrato_accumulator -= M_PI_M2;

    double val = sin(vibrato_accumulator) * vibrato_amplitude;
    *(pc.freq) += val;

    params[3] = vibrato_accumulator;
}

struct effect *create_vibrato(double amplitude, double frequency, double phase){
    struct effect *vibrato = malloc(sizeof(struct effect));
    vibrato->args = malloc(4 * sizeof(double));
    double *ptr = vibrato->args;
    ptr[0] = amplitude;
    ptr[1] = frequency;
    ptr[2] = phase;
    ptr[3] = 0;
    vibrato->apply = apply_vibrato;
    return vibrato;
}

/**
    a pipeline is a sequence of effects.
    it can be optimized by using a double linked list but i'm too lazy for that.
 */
struct pipelineNode{
    struct effect *effect;
    struct pipelineNode *next;
};

struct pipelineNode *create_pipelineNode(struct effect *ef){
    struct pipelineNode *pl = malloc(sizeof(struct pipelineNode));
    pl->effect = ef;
    pl->next = NULL;
    return pl;
}

void add_pipelineNode(struct effect *ef, struct pipelineNode *nd){
    struct pipelineNode *pl = create_pipelineNode(ef);
    struct pipelineNode *ptr = nd;
    while(ptr->next != NULL){
        ptr = ptr->next;
    }
    ptr->next = pl;
}

/**
    main data structure provided by pipewire.
    i don't know how any of this works, i copied it from the official documentation
    and tweaked it until it magically worked.
 */
struct data {
    struct pw_main_loop *loop;
    struct pw_stream *stream;
    struct pipelineNode *pipeline;
    double (*waveform)(double);
    double accumulator;
    int *last;
};
