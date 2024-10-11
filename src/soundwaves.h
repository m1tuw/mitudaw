#include <math.h>

double square_wave(double x){
    if(x >= 0 && x <= M_PI){
        return 1.0;
    }
    else{
        return -1.0;
    }
}