#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

// im ok with setting this because why would i need more that a 5000 wide box
// its 5000 cuase the color characters do take up a ton
#define MAX_LEN 5000


// cut out a box width by height from stdin
// dont count ansi color codes, so the box is visually width by height
// add spaces so that you can paste something on the end in a nice column
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
    // TODO this hardcoded no file, comes from stdin, prolly make an option to give a file in the future
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
    char *locale;
    locale = setlocale(LC_ALL, "");

    /* printf("stdout orientation:\t%d\n", fwide(stdout, 0)); */
    /* printf("stdin orientation:\t%d\n", fwide(stdin, 1)); */

    //printf("converting to a width of %d and a height of %d\n", width, height);
    wchar_t line[MAX_LEN] = L"";
    int visstrlen = 0;
    int increment = 1;
    wchar_t * c;
    for(int lineno = 0; lineno < height && fgetws(line, MAX_LEN, stdin); lineno++){
        for(c = line, increment = 1, visstrlen = 0; (visstrlen < width) && (*c != L'\0') && (*c != L'\n') && (*c != L'\r'); c++){
            if(*c == L'\033'){
                increment = 0;
            }else if(*c == L'\t'){
                // use 4 spaces for tabs, count all 4 spaces now and replace the tab
                // TODO consider making tab size changeable
                visstrlen += 4;
                int sl = wcslen(c) - 1;
                c += 4;
                wmemmove(c, c - 3, (c - line + sl > MAX_LEN - 1) ? line - c + MAX_LEN - 1 : sl);
                // add in the spaces
                // yes this is worse than using or memcpy but it is prolly faster
                // but for wchar im sure it wont work so i need to just use wmemcpy
                //*((u_int32_t *) (c - 4)) = 0x20202020;
                wmemcpy(c - 4, L"    ", 4);
                c--;
            }else if(increment){
                visstrlen++;
                //printf("%c: %d\n", *c, visstrlen);
            }else if(*c == L'm'){
                increment = 1;
            }
        }
        // add color reset at end
        if(c - line + 4 > MAX_LEN)
            c -= 4;
        wcscpy(c, L"\033[0m");
        // i can just end it here but in odd circumstances that arent typical of the use of this program its faster to use memset so my hands were tied
        // its really not about wanting to do that from the start and just about speed in real use definitely dont dowbt me
        //printf("%ls%-*ls│\n", line, width - visstrlen, L"");

        // add spaces at end if missing room
        c += 4;
        if(width > visstrlen)
            // after i memset it doesnt seem to want me to set vals outside of the memset so im just going to memset more and then it should work
            //wmemset(c, L' ', width - visstrlen + 2);
            wmemset(c, L' ', width - visstrlen);
        // TODO give options for if the bar at the end is there / what to use as the delimeter
        // i actually dont want to add this bar here, id rather do it in the printf but technically this is a wide character so
        //c[width-visstrlen] = L'│';
        // add Null char to end string
        //c[width-visstrlen+1] = L'\0';
        wcscpy(c + width - visstrlen, L"│");
        printf("%ls\n", line);
    }
    return 0;
}
