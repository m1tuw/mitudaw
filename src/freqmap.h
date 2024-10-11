//#include <stdio.h>

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