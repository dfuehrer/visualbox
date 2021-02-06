#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// im ok with setting this because why would i need more that a 5000 wide box
// its 5000 cuase the color characters do take up a ton
#define MAX_LEN 5000


// cut out a box width by height from stdin
// dont count ansi color codes, so the box is visually width by height
// add spaces so that you can paste something on the end in a nice column
// TODO figure out big characters like emojis that take up multiple bytes but dont take up that much space
//  i think theres other bugs too
// TODO like it says below, check bounds so off cases dont cause an out of bounds issue
int main(int argc, char ** argv){
    // at least for now im going to require width and height
    // in the future id like to not require the height and maybe add a file option (though stdin isnt really any worse, can just require < file)
    // also right now it wont read from a file
    // 1: wrong num args error
    if(argc < 3){
        puts("must give width and height");
        return 1;
    }
    // TODO hardcoded no file, comes from stdin, prolly make an option to give a file in the future
    // TODO make sure conversion to numbers here works
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    // 2: out of width bounds error
    if(width >= MAX_LEN){
        // then this is way too big since ansi characters take up a ton of bytes
        puts("whats the big idea, thats what too wide");
        return 2;
    }else if((width < 1) || (height < 1)){
        puts("Seriously, if you want to output nothing there are easier ways");
        return 2;
    }
    //printf("converting to a width of %d and a height of %d\n", width, height);
    char line[MAX_LEN] = "";
    int visstrlen = 0;
    int increment = 1;
    char * c;
    // TODO multi-byte chars really screw me up
    //  figuring out wchar seems like a big pain
    // TODO tabs really screw me over too
    for(int lineno = 0; lineno < height && fgets(line, MAX_LEN, stdin); lineno++){
        for(c = line, increment = 1, visstrlen = 0; (visstrlen < width) && *c && (*c != '\n') && (*c != '\r'); c++){
            if(*c == '\033'){
                increment = 0;
            }else if(*c == '\t'){
                // use 4 spaces for tabs, count all 4 spaces now and replace the tab
                visstrlen += 4;
                int sl = strlen(c) - 1;
                c += 4;
                memmove(c, c - 3, (c - line + sl > MAX_LEN - 1) ? line - c + MAX_LEN - 1 : sl);
                // add in the spaces
                // yes this is worse than using or memcpy but it is prolly faster
                *((u_int32_t *) (c - 4)) = 0x20202020;
                //memcpy(c - 4, "    ", 4);
                c--;
            }else if(increment){
                visstrlen++;
                //printf("%c: %d\n", *c, visstrlen);
            }else if(*c == 'm'){
                increment = 1;
            }
        }
        // TODO adding in a few chars here, make sure to stop the loop prior to getting past the length ot the string
        // TODO give options for if the bar at the end is there
        // add color reset at end
        if(c - line + 4 > MAX_LEN)
            c -= 4;
        strcpy(c, "\033[0m");
        // i can just end it here but in odd circumstances that arent typical of the use of this program its faster to use memset so my hands were tied
        // its really not about wanting to do that from the start and just about speed in real use definitely dont dowbt me
        //printf("%s%-*s│\n", line, width - visstrlen, "");

        // add spaces at end if missing room
        c += 4;
        // after i memset it doesnt seem to want me to set vals outside of the memset so im just going to memset more and then it should work
        if(width > visstrlen)
            memset(c, ' ', width - visstrlen + 1);
        // add Null char to end string
        c[width-visstrlen] = '\0';
        printf("%s│\n", line);

        // this was putting it all in at once so i only need puts
        //strcpy(c + width - visstrlen, "│");
        //puts(line);
    }
    return 0;
}
