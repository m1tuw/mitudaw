#include <ncurses.h>
#include <stdio.h>

int main(){
    initscr();
    int ch = -1;
    while(ch != KEY_UP){
        ch = getch();
        printf("key pressed: %d\n", ch);
        printf("\n");
        // a is 97
    }
    endwin();    
    return 0;
}