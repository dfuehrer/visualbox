#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
// TODO maybe use libgrapheme from suckless if its better for finding how many chars there are
#include <wchar.h>
#include <locale.h>

#include "clparser/parseargs.h"
#include "libgrapheme/grapheme.h"

#define END_COL_STR  ("\033[0m")
#define END_COL_SIZE (sizeof END_COL_STR)
#define END_COL_LEN  (END_COL_SIZE - 1)


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
    // use 4 spaces for tabs
    int tabsize = 4;
    const char * delim = "│";
    map_t params;
    map_t flags;
    initMap(&params);
    initMap(&flags);
    addMapMembers(&flags , NULL,    BOOL, false, "Ssd",  STRVIEW("help"  ), "h", 1);
    MapData * widthNode  = addMapMembers(&params, NULL,    INT,  false, "SsdS", STRVIEW("width" ), "w", 1, STRVIEW("rows"));
    MapData * heightNode = addMapMembers(&params, NULL,    INT,  false, "SsdS", STRVIEW("height"), "h", 1, STRVIEW("columns"));
    addMapMembers(&params, &tabsize,INT,  true,  "Ssd",  STRVIEW("tabs"  ), "t", 1);
    addMapMembers(&params, delim,   STR,  true,  "Ssd",  STRVIEW("delim" ), "d", 1);
    MapData * fileNode = addMapMembers(&params, "-",     STR,  true,  "Ssd",  STRVIEW("file"  ), "f", 1);
    MapData * positionalNodes[] = {
        fileNode,
        widthNode,
        heightNode,
        NULL,
    };
    const char ** positionalArgs;
    Errors e = parseArgs(argc-1, argv+1, &flags, &params, positionalNodes, &positionalArgs, false);
    if(e != Success){
        fprintf(stderr, "error parsing clargs\n");
        return e;
    }
    bool help = getMapMember_bool(&flags, "help", 4);
    if(help){
        printUsage(&flags, &params, (const MapData **)positionalNodes, argv[0]);
        fprintf(stderr, "file taken as positional argument\n");
        printHelp(&flags, &params, (const MapData **)positionalNodes,
                "help = Print this help message\n\
                file = file to output (default stdin)\n\
                width = width of output (required)\n\
                height = height of output (required)\n\
                tabs = number of spaces per tab\n\
                delim = delimeter to print at end of line"
                );
        return 1;
    }
    const char * filename = getMapMemberData(&params, "file", 4);
    if(strcmp(filename, "-") == 0){
        file = stdin;
    }else{
        file = fopen(filename, "r");
    }
    free(positionalArgs);
    // TODO check if width and height given
    width   = getMapMember_int(&params, "width" , 5);
    height  = getMapMember_int(&params, "height", 6);
    tabsize = getMapMember_int(&params, "tabs",   4);
    delim   = getMapMemberData(&params, "delim",  5);
    freeMap(&flags);
    freeMap(&params);
    // 2: out of width bounds error
    if((width < 1) || (height < 1)){
        fputs("Seriously, if you want to output nothing there are easier ways\n", stderr);
        // TODO this is dumb because it doesnt check how long the output is and doesnt output the delimeter
        if(height > 0){
            // if asked for no width then print out the number of empty lines asked for
            // TODO should also read through the buffer if piped in
            char * line = calloc(height, sizeof (char));
            if(line == NULL){
                perror("allocate line");
                exit(EXIT_FAILURE);
            }
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

    //printf("stdout orientation:\t%d\n", fwide(stdout, 0));
    //printf("stdin orientation:\t%d\n", fwide(stdin, 1));

    //fprintf(stderr, "converting to a width of %d and a height of %d\n", width, height);
    // TODO use wcswidth to find the width at the end
    //  this will require having a separate array so i can remove all control chars / non-printing chars, may be a pain
    int visstrlen = 0;
    bool increment = true;

    // start by allocating the bare minimum size
    char * c = NULL;
    char * prevColStart = c;
    size_t len = (width + 1) * (sizeof *c);
    char * line = (char *)malloc(len);
    memset(line, 0, len);       // set the line to all NULL chars
    ssize_t nread = 0;
    mbstate_t state = {0};
    int i = 1;
    for(int lineno = 0; lineno < height && ((nread = getline(&line, &len, file)) != -1); ++lineno){
        //fprintf(stderr, "line: %d (read %zd)\n", i, nread);
        size_t cInc = 1;
        bool usedColor = false;
        increment = true;
        visstrlen = 0;
        for(c = prevColStart = line; (visstrlen < width) && (*c != '\n') && (*c != '\0') && (*c != '\r'); c += cInc){
            if(*c == '\033' && c[1] == '['){
                // TODO check that this is actually a color and not some other type of thing
                // if c == \033 then starting color info, doesnt add to width
                increment = false;
                usedColor = true;
                cInc = 2;
                // save the place of the last unresolved color change to not add it to the output
                //  (we clear this if we get something to print or a full color reset so we dont actually have to set it here)
                //prevColStart = c;
            }else if(increment){
                if(*c == '\t'){
                    // if the char is a tab then add spaces manually
                    // if not enough room to just move the string then allocate more room
                    int tabAmount = (visstrlen + tabsize > width) ? width - visstrlen : tabsize - (visstrlen % tabsize);
                    if((size_t)(nread + tabsize) > len){
                        // not doing a realloc because i dont want to move the whole string and then have to move the end of the string again
                        // TODO try to check if we can actaully just make the buffer bigger and use the efficient form of realloc
                        // TODO maybe check how many tabs are left in the line so we can just allocate once for this line
                        len += tabAmount;
                        char * tmpLine = malloc(len);
                        memcpy(tmpLine, line, c - line);
                        char * tmpc = tmpLine + (c - line);
                        memset(tmpc, ' ', tabAmount);
                        strcpy(tmpc + tabAmount, c + 1);
                        free(line);
                        line = tmpLine;
                        c = tmpc;
                    }else{
                        // move the string forward and add in the spaces where the tab was
                        memmove(c + tabAmount, c + 1, nread + line - c);
                        memset(c, ' ', tabAmount);
                    }
                    // count all tabAmount spaces now
                    nread += tabAmount-1;
                    visstrlen += tabAmount;
                    cInc = tabAmount;
                }else{
                    cInc = grapheme_next_character_break_utf8(c, nread + line - c);
                    fprintf(stderr, "1 char: '%.*s' (%zu bytes)\n", (int) cInc, c, cInc);
                    size_t j;
                    size_t jInc = 0;
                    int incWidth = 0;
                    for(j = 0; j < cInc; j += jInc){
                        // get the number of chars in the current multi-byte char (code point)
                        jInc = mbrlen(c + j, nread + line - c - j, &state);
                        // if it was an error then break
                        if((jInc == (size_t)-1) || (jInc == (size_t)-2)){
                            // TODO figure out error
                            fprintf(stderr, "error: mbrlen = %zd\n", jInc);
                            fprintf(stderr, "c: '%s', nread: %zd, line-c: %zd\n", c+j, nread, line-c-j);
                            break;
                        }
                        // get the current multi-byte char as a wchar
                        wchar_t wcs[2] = L"";
                        mbrtowc(wcs, c+j, jInc, &state);
                        // get the width of the wide char to increment the visstrlen
                        //int cWidth = wcswidth(wcs, cInc / (sizeof wcs[0]) + 1);
                        int cWidth = wcwidth(wcs[0]);
                        fprintf(stderr, "char '%.*s' width: %d\n", (int) jInc, c+j, cWidth);
                        incWidth += cWidth;
                        //fprintf(stderr, "chars: %zd, width: %d, str: '%*s'\n", cInc, cWidth, (int)cInc, c);
                        // if the visstrlen would be greater than the width then were done, dont increment the visstrlen
                        if(visstrlen + incWidth > width){
                            break;
                        }
                    }
                    // if didnt finish loop above then break out of this loop too
                    if(j < cInc){
                        break;
                    }
                    visstrlen += incWidth;
                }
                // clear the prev color start pointer since we are currently dealing with actual output
                prevColStart = c + cInc;
                //fprintf(stderr, "%c: %d\n", *c, visstrlen);
            }else if(*c == 'm'){
                // if c == m then ending color block, will start incrementing by width again
                increment = true;
                if(!strncmp(c - (END_COL_LEN-1), END_COL_STR, END_COL_LEN)){
                    // if this is just ending a color reset str then mark as no color used
                    usedColor = false;
                }
            }
        }
        if(usedColor){
            // add color reset at end
            c = prevColStart;
            if((size_t)(c - line + END_COL_SIZE) > len){  // END_COL_SIZE is the length of the color reset str
                len += END_COL_SIZE;
                size_t tmpdist = c - line;
                line = (char *)realloc(line, len);
                c = line + tmpdist;
            }
            memcpy(c, END_COL_STR, END_COL_SIZE);
            //strcpy(c, END_COL_STR);
        }else{
            *c = '\0';
        }
        //fprintf(stderr, "width: %d\n", visstrlen);

        // TODO if height > num lines in file, maybe pad with whitespace

        // i can just end it here but in odd circumstances that arent typical of the use of this program its faster to use memset so my hands were tied
        // its really not about wanting to do that from the start and just about speed in real use definitely dont doubt me
        //printf("%s%-*s|\n", line, width - visstrlen, "");
        //printf("%s%-*s%lc\n", line, width - visstrlen, "", L'│');
        printf("%s%-*s%s\n", line, width - visstrlen, "", delim);

        // add spaces at end if missing room
        // TODO need to allocate more space to be able to do this (should allocate it at the same time as the usedColor realloc
        //c += END_COL_LEN;
        //if(width > visstrlen){
        //    // after i memset it doesnt seem to want me to set vals outside of the memset so im just going to memset more and then it should work
        //    //wmemset(c, L' ', width - visstrlen + 2);
        //    memset(c, ' ', width - visstrlen);
        //}
        //// NOTE this might stick out past the end of the string
        ////strcpy(c + width - visstrlen, "│");
        ////printf("%s\n", line);
        //printf("%s%lc\n", line, L'│');
        ++i;
    }
    free(line);
    return 0;
}
