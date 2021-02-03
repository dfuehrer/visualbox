#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// im ok with setting this because why would i need more that a 5000 wide box
// its 5000 cuase the color characters do take up a ton
#define MAX_LEN 5000


int main(int argc, char ** argv){
    // at least for now im going to require width and height
    // in the future id like to not require the height and maybe add a file option (though stdin isnt really any worse, can just require < file)
    // also right now it wont read from a file
    if(argc < 3){
        puts("must give width and height");
        return 1;
    }
    // TODO hardcoded no file, comes from stdin, prolly make an option to give a file in the future
    // TODO error if width > MAX_LEN or < 1
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    //printf("converting to a width of %d and a height of %d\n", width, height);
    char line[MAX_LEN];
    int strlen = 0;
    int increment = 1;
    char * c;
    for(int lineno = 0; lineno < height && fgets(line, MAX_LEN, stdin); lineno++){
        for(c = line, increment = 1, strlen = 0; strlen < width && *c && *c != '\n'; c++){
            if(*c == '\033'){
                increment = 0;
            }else if(increment){
                strlen++;
            }else if(*c == 'm'){
                increment = 1;
            }
        }
        // TODO adding in a few chars here, make sure to stop the loop prior to getting past the length ot the string
        // add color reset at end
        strcpy(c, "\033[0m");
        // add spaces at end if missing room
        c += 4;
        if(width > strlen)
            memset(c, ' ', width - strlen);
        // add Null char to end string
        *(c+width-strlen) = '\0';
        // TODO give options for if the bar at the end is there
        printf("%-*s│\n", width+4, line);
    }
    return 0;
}
