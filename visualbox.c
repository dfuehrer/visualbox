#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#define __USE_XOPEN
#include <wchar.h>
#include <locale.h>

#define END_COL_LEN 4
#define END_COL_STR ("\033[0m")


// cut out a box width by height from stdin
// dont count ansi color codes, so the box is visually width by height
// add spaces so that you can paste something on the end in a nice column
// TODO like it says below, check bounds so off cases dont cause an out of bounds issue
int main(const int argc, const char * const argv[]){
    // at least for now im going to require width and height
    // in the future id like to not require the height and maybe add a file option (though stdin isnt really any worse, can just require < file)
    // also right now it wont read from a file
    // 1: wrong num args error
    FILE * file;
    int width, height;
    int wInd = 1;
    // use 4 spaces for tabs
    // TODO consider making tab size changeable
    int tabsize = 4;
    if(argc < 3){
        puts("must give width and height");
        return 1;
    }else if(argc == 3){
        // no file given, must be stdin
        file = stdin;
    }else{
        file = fopen(argv[1], "r");
        ++wInd;
    }
    // TODO make sure conversion to numbers here works (look into using strtol)
    width  = atoi(argv[wInd]);
    height = atoi(argv[wInd+1]);
    // 2: out of width bounds error
    if((width < 1) || (height < 1)){
        fputs("Seriously, if you want to output nothing there are easier ways\n", stderr);
        if(height > 0){
            // if asked for no width then print out the number of empty lines asked for
            // TODO should also read through the buffer if piped in
            char * line = calloc(height, sizeof (char));
            memset(line, '\n', height-1);
            line[height-1] = '\0';
            puts(line);
            free(line);
        }
        return 2;
    }
    char *locale;
    locale = setlocale(LC_ALL, "");
    (void) locale;

    /* printf("stdout orientation:\t%d\n", fwide(stdout, 0)); */
    /* printf("stdin orientation:\t%d\n", fwide(stdin, 1)); */

    //fprintf(stderr, "converting to a width of %d and a height of %d\n", width, height);
    // TODO use getline to dynamically allocate so we dont have this weird limitation, even if it is reasonable
    // TODO use wcswidth to find the width at the end
    //  this will require having a separate array so i can remove all control chars / non-printing chars, may be a pain
    //wchar_t line[MAX_LEN] = L"";
    //wchar_t * c;
    int visstrlen = 0;
    int increment = 1;

    // start by allocating the bare minimum size
    char * c = NULL;
    char * prevColStart = c;
    size_t len = (width + 1) * (sizeof *c);
    char * line = (char *)malloc(len);
    memset(line, 0, len);       // set the line to all NULL chars
    //char * const spaces = (char *)calloc(tabsize, sizeof (char));
    //memset(spaces, ' ', tabsize);
    ssize_t nread = 0;
    mbstate_t state = {0};
    int i = 1;
    for(int lineno = 0; lineno < height && ((nread = getline(&line, &len, file)) != -1); ++lineno){
        //fprintf(stderr, "line: %d (read %zd)\n", i, nread);
        size_t cInc = 1;
        bool usedColor = false;
        increment = 1;
        visstrlen = 0;
        for(c = prevColStart = line; (visstrlen < width) && (*c != '\n') && (*c != '\0') && (*c != '\r'); c += cInc){
            if(*c == '\033'){
                // if c == \033 then starting color info, doesnt add to width
                increment = 0;
                usedColor = true;
                cInc = 1;
                // save the place of the last unresolved color change to not add it to the output
                //  (we clear this if we get something to print or a full color reset so we dont actually have to set it here)
                //prevColStart = c;
            }else if(increment){
                if(*c == '\t'){
                    // if the char is a tab then add spaces manually
                    // if not enough room to just move the string then allocate more room
                    //int tabAmount;
                    //if(visstrlen + tabsize > width){
                    //    tabAmount = width - visstrlen;
                    //}else{
                    //    tabAmount = tabsize - (visstrlen % tabsize);
                    //}
                    int tabAmount = (visstrlen + tabsize > width) ? width - visstrlen : tabsize - (visstrlen % tabsize);
                    if((size_t)(nread + tabsize) > len){
                        // not doing a realloc because i dont want to move the whole string and then have to move the end of the string again
                        // TODO try to check if we can actaully just make the buffer bigger and use the efficient form of realloc
                        // TODO maybe check how many tabs are left in the line so we can just allocate once for this line
                        len += tabAmount;
                        char * tmpLine = malloc(len);
                        memcpy(tmpLine, line, c - line);
                        char * tmpc = tmpLine + (c - line);
                        //memcpy(tmpc, spaces, tabAmount);
                        memset(tmpc, ' ', tabAmount);
                        strcpy(tmpc + tabAmount, c + 1);
                        free(line);
                        line = tmpLine;
                        c = tmpc;
                    }else{
                        // move the string forward and add in the spaces where the tab was
                        memmove(c + tabAmount, c + 1, nread + line - c);
                        //memcpy(c, spaces, tabAmount);
                        memset(c, ' ', tabAmount);
                    }
                    // count all tabAmount spaces now
                    nread += tabAmount-1;
                    visstrlen += tabAmount;
                    cInc = tabAmount;
                }else{
                    // get the number of chars in the current multi-byte char
                    cInc = mbrlen(c, nread + line - c, &state);
                    // if it was an error then break
                    if((cInc == (size_t)-1) || (cInc == (size_t)-2)){
                        // TODO figure out error
                        fprintf(stderr, "error: mbrlen = %zd\n", cInc);
                        fprintf(stderr, "c: '%s', nread: %zd, line-c: %zd\n", c, nread, line-c);
                        break;
                    }
                    // get the current multi-byte char as a wchar
                    wchar_t wcs[2] = L"";
                    mbrtowc(wcs, c, cInc, &state);
                    // get the width of the wide char to increment the visstrlen
                    //int inc = wcswidth(wcs, cInc / (sizeof wcs[0]) + 1);
                    int inc = wcwidth(wcs[0]);
                    //fprintf(stderr, "chars: %zd, width: %d, str: '%*s'\n", cInc, inc, (int)cInc, c);
                    if(i > 0){
                        // if the visstrlen would be greater than the width then were done, dont increment the visstrlen
                        if(visstrlen + inc > width){
                            break;
                        }
                        visstrlen += inc;
                    }
                }
                // clear the prev color start pointer since we are currently dealing with actual output
                prevColStart = c + cInc;
                //fprintf(stderr, "%c: %d\n", *c, visstrlen);
            }else if(*c == 'm'){
                // if c == m then ending color block, will start incrementing by width again
                increment = 1;
                if(!strncmp(c - (END_COL_LEN-1), END_COL_STR, END_COL_LEN)){
                    // if this is just ending a color reset str then mark as no color used
                    usedColor = false;
                }
            }
        }
        if(usedColor){
            // add color reset at end
            c = prevColStart;
            if((size_t)(c - line + END_COL_LEN+1) > len){  // END_COL_LEN is the length of the color reset str
                len += END_COL_LEN+1;
                size_t tmpdist = c - line;
                line = (char *)realloc(line, len);
                c = line + tmpdist;
            }
            //memcpy(c, END_COL_STR, END_COL_LEN);
            strcpy(c, END_COL_STR);
        }else{
            *c = '\0';
        }
        //fprintf(stderr, "width: %d\n", visstrlen);

        // i can just end it here but in odd circumstances that arent typical of the use of this program its faster to use memset so my hands were tied
        // its really not about wanting to do that from the start and just about speed in real use definitely dont doubt me
        //printf("%s%-*s|\n", line, width - visstrlen, "");
        printf("%s%-*s%lc\n", line, width - visstrlen, "", L'│');
        // TODO get the delim from clargs
        //printf("%s%-*s%s\n", line, width - visstrlen, "", delim);

        // add spaces at end if missing room
        // TODO need to allocate more space to be able to do this (should allocate it at the same time as the usedColor realloc
        //c += END_COL_LEN;
        //if(width > visstrlen){
        //    // after i memset it doesnt seem to want me to set vals outside of the memset so im just going to memset more and then it should work
        //    //wmemset(c, L' ', width - visstrlen + 2);
        //    memset(c, ' ', width - visstrlen);
        //}
        //// TODO give options for if the bar at the end is there / what to use as the delimeter
        //// NOTE this might stick out past the end of the string
        ////strcpy(c + width - visstrlen, "│");
        ////printf("%s\n", line);
        //printf("%s%lc\n", line, L'│');
        ++i;
    }
    free(line);
    //free(spaces);
    return 0;
}
