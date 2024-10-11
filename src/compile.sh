gcc -Wall main.c -o main $(pkg-config --cflags --libs libpipewire-0.3) -lncurses -lm
gcc -Wall keyboardInputHandler.c -o keyboardInputHandler $(pkg-config --cflags --libs libpipewire-0.3) -lncurses -lm
