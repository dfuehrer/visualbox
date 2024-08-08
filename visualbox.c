#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#ifndef NO_LIBGRAPHEME
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <poll.h>
#endif  // NO_LIBGRAPHEME
#include <sys/types.h>
#include <wchar.h>
#include <locale.h>

#include "clparser/parseargs.h"
#include "libgrapheme/grapheme.h"

// maybe test on this stuff:
// 12345678901234567890123456789123456789
// 🧑‍🌾
// Tëst 👨‍👩‍👦 🇺🇸 नी நி!
// u̲n̲d̲e̲r̲l̲i̲n̲e̲d̲

#define END_COL_STR  ("\033[0m")
#define END_COL_SIZE (sizeof END_COL_STR)
#define END_COL_LEN  (END_COL_SIZE - 1)

//#define DEBUG_MESSAGES
#ifdef DEBUG_MESSAGES
#   define DEBUGF(...) fprintf(stderr, __VA_ARGS__)
#else
#   define DEBUGF(...) 
#endif  // DEBUG_MESSAGES

#define WARN_MESSAGES
#ifdef WARN_MESSAGES
#   define WARNF(...) fprintf(stderr, __VA_ARGS__)
#else
#   define WARNF(...) 
#endif  // WARN_MESSAGES

#define ERROR_MESSAGES
#ifdef ERROR_MESSAGES
#   define ERRORF(...) fprintf(stderr, __VA_ARGS__)
#else
#   define ERRORF(...) 
#endif  // ERROR_MESSAGES

#define TODO(MSG)   assert(false && MSG);

#ifndef NO_LIBGRAPHEME
#ifndef __USE_MISC
static void cfmakeraw(struct termios *termios_p);
#endif  // __USE_MISC

typedef enum {
    DEC_UNKNOWN   = '0',
    DEC_SET       = '1',
    DEC_RESET     = '2',
    DEC_PERMSET   = '3',
    DEC_PERMRESET = '4',
} DEC_RESPONSE;

static void chrError(char expected, char got);
DEC_RESPONSE readDECResponse(int fd, char mode[], size_t modelen);

//#define MAX_CC  ((1 << (8*sizeof (cc_t))) - 1)
#define MAX_CC  ((cc_t) -1)
#endif  // NO_LIBGRAPHEME

static void debug_v(char str[]);

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
    bool clust = false;
    bool no_2027 = false;
    int wait_tenths = 1;
    map_t params;
    map_t flags;
    initMap(&params);
    initMap(&flags);
    addMapMembers(&flags , NULL,    BOOL, false, "Ssd",  STRVIEW("help"  ), "h", 1);
    MapData * widthNode  = addMapMembers(&params, NULL,    INT,  false, "SsdS", STRVIEW("width" ), "w", 1, STRVIEW("rows"));
    MapData * heightNode = addMapMembers(&params, NULL,    INT,  false, "SsdS", STRVIEW("height"), "h", 1, STRVIEW("columns"));
    addMapMembers(&params, &tabsize,INT,  true,  "Ssd",  STRVIEW("tabs"  ), "t", 1);
    addMapMembers(&params, delim,   STR,  true,  "Ssd",  STRVIEW("delim" ), "d", 1);
    // TODO add color stripping options
#ifndef NO_LIBGRAPHEME
    // TODO add option to tell it not to try mode 2027
    addMapMembers(&flags , NULL,    BOOL, false, "SS",   STRVIEW("force-cluster" ), STRVIEW("f"));
    addMapMembers(&flags , NULL,    BOOL, false, "SS",   STRVIEW("no-mode-2027"  ), STRVIEW("N"));
    // TODO consider making this ms instead of tenths
    addMapMembers(&params, &wait_tenths, INT, true, "SS", STRVIEW("wait-tenths"  ), STRVIEW("W"));
#endif // NO_LIBGRAPHEME
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
        ERRORF("error parsing clargs\n");
        return e;
    }
    bool help = getMapMember_bool(&flags, "help", 4);
    if(help){
        printUsage(&flags, &params, (const MapData **)positionalNodes, argv[0]);
        fprintf(stderr, "file taken as positional argument\n");
        // TODO probably don't use this help printing function cause it's honestly not that helpful and it has an issue with the width of the default delim which just is dumb for an application that should be able to handle stuff
        //printHelp(&flags, &params, (const MapData **)positionalNodes,
        //        "help = Print this help message\n"
        //        "file = file to output (default stdin)\n"
        //        "width = width of output (required)\n"
        //        "height = height of output (required)\n"
        //        "tabs = number of spaces per tab\n"
        //        "delim = delimeter to print at end of line\n"
        //        #ifndef NO_LIBGRAPHEME
        //        "force-cluster = force length to clustered grapheme\n"
        //        "no-mode-2027 = don't check if terminal can use clustering\n"
        //        #endif // NO_LIBGRAPHEME
        //        );
        fprintf(stderr,
                "\t[ --help|-h ]                                    Print this help message\n"
                "\t[ --file|-f file = - ]                           file to output (default stdin)\n"
                "\t--width|-w|--rows rows                           width of output (required)\n"
                "\t--height|-h|--columns columns                    height of output (required)\n"
                "\t[ --tabs|-t tabs = %d ]                           number of spaces per tab\n"
                "\t[ --delim|-d delim = %s ]                         delimeter to print at end of line\n"
#ifndef NO_LIBGRAPHEME
                "\t[ --force-cluster|-f ]                           force length to clustered grapheme\n"
                "\t[ --no-mode-2027|-N ]                            don't check if terminal can use clustering\n"
                "\t[ --wait-tenths|-W ]                             time to wait for mode 2027 result from terminal (in tenths of seconds)\n"
#endif // NO_LIBGRAPHEME
                , tabsize, delim);
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
    width   = getNode_int( widthNode);
    height  = getNode_int(heightNode);
    tabsize = getMapMember_int(&params, STR_LEN("tabs"  ));
    delim   = getMapMemberData(&params, STR_LEN("delim" ));
#ifndef NO_LIBGRAPHEME
    clust   = getMapMember_bool(&flags, STR_LEN("force-cluster"));
    no_2027 = getMapMember_bool(&flags, STR_LEN("no-mode-2027"));
    wait_tenths = getMapMember_int(&params, STR_LEN("wait-tenths"));
    if(wait_tenths > MAX_CC){
        WARNF("Warning: gave wait time of %gs, limiting to max of %hhu tenths\n", wait_tenths/10.0f, MAX_CC);
        wait_tenths = MAX_CC;
    }
#else
    no_2027 = true;
    (void) no_2027;
#endif // NO_LIBGRAPHEME
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

#ifndef NO_LIBGRAPHEME
    // TODO speed up the mode 2027 logic, it seems to be pretty slow on some terminals
    // try using mode 2027 if supported
    // - mode 2027 is a proposed spec implemented by a few niche terminals to set whether fancy clustered characters should be used
    //  - fancy clustering is nice but many apps assume they can just use mbrlen and wcswidth so they get out of sync with the terminal
    //  - this mode lets clustering be available to apps that can handle it while defaulting to a safer option for most apps
    if(!no_2027 && !clust){
        struct termios cur_tios = {0};
        struct termios raw_tios;
        int stdin_fd = STDIN_FILENO;
        if(!isatty(stdin_fd)){
            stdin_fd = STDOUT_FILENO;
            if(!isatty(stdin_fd)){
                stdin_fd = STDERR_FILENO;
                if(!isatty(stdin_fd)){
                    DEBUGF("std in, out, and err not connected to terminal, not setting terminal props\n");
                    goto skip_checkterm;
                }
            }
        }
        stdin_fd = open(ttyname(stdin_fd), O_RDWR);
        //stdin_fd = open(ctermid(NULL), O_RDWR);
        if(stdin_fd < 0){
            DEBUGF("error opening term file '%s' for fd %d: %s\n", ttyname(stdin_fd), stdin_fd, strerror(errno));
            goto skip_checkterm;
        }
        int ret = tcgetattr(stdin_fd, &cur_tios);
        if(ret != 0){
            DEBUGF("error getting terminal props: %s\n", strerror(errno));
            goto skip_checkterm;
        }
        raw_tios = cur_tios;
        // TODO make fd non-blocking so terminal doesnt hang on lack of response
        //  - can be done with tcsetattr with MIN and TIME of c_cc
        //  - can be done normal way with epoll/select or whatever (optionally making it nonblocking)
        cfmakeraw(&raw_tios);
        //DEBUGF("setting wait time for 1 byte to %gs\n", wait_tenths/10.0f);
        //raw_tios.c_cc[VTIME] = wait_tenths;
        //raw_tios.c_cc[VMIN] = 1;
        ret = tcsetattr(stdin_fd, TCSANOW, &raw_tios);
        // TODO check that things set properly
        if(ret != 0){
            DEBUGF("error setting terminal props: %s\n", strerror(errno));
            goto skip_checkterm;
        }
#define REQUEST_2027    "\033[?2027$p"
#define SET_2027        "\033[?2027h"
        // request if mode 2027 is set
        //DEBUGF("sending request\n");
        write(stdin_fd, STR_LEN(REQUEST_2027));
        //DEBUGF("reading response\n");
        struct pollfd pollfds[1] = {0};
        pollfds[0].fd = stdin_fd;
        pollfds[0].events = POLLIN;
        // TODO should i be using something other than poll?
        //  select seems more portable but more painful to use
        //  epoll is linux specific and faster in theory but seems like it wouldnt actually be faster for this use case of 1 fd checked once
        ret = poll(pollfds, 1, wait_tenths*100);
        if(ret == -1){
            DEBUGF("poll returned %d: (%d) %s\n", ret, errno, strerror(errno));
            goto skip_checkterm;
        }
        if(ret == 0 || !(pollfds[0].revents & POLLIN)){
            DEBUGF("read not ready, skipping (assuming no clustering)\n");
            goto skip_checkterm;
        }
        char response = readDECResponse(stdin_fd, STR_LEN("2027"));
        //printf("got response: '%c'\n", response);
        if(response == DEC_SET || response == DEC_PERMSET){
            clust = true;
        }else if(response == DEC_RESET){
            // try to set clustering
            write(stdin_fd, STR_LEN(SET_2027));
            write(stdin_fd, STR_LEN(REQUEST_2027));
            pollfds[0].revents = 0;
            ret = poll(pollfds, 1, wait_tenths*100);
            if(ret == -1){
                // TODO maybe handle different things in this case
                DEBUGF("poll returned %d: (%d) %s\n", ret, errno, strerror(errno));
                goto skip_checkterm;
            }
            if(ret == 0 || !(pollfds[0].revents & POLLIN)){
                DEBUGF("read not ready, skipping (assuming no clustering).  weird since got return from first request..\n");
                goto skip_checkterm;
            }
            response = readDECResponse(stdin_fd, STR_LEN("2027"));
            if(response == DEC_SET || response == DEC_PERMSET){
                clust = true;
            }else{
                WARNF("tried to set mode 2027 for clustering but it failed (got '%c' response)\n", response);
            }
        }
        // TODO check that things set properly

skip_checkterm:
        // TODO maybe dont cleanup and close this if timed out reading response so it can be read later?
        //  - right now if skipped reading then the response will be printed out to user which sucks
        //  - but do need to set terminal back to original mode to do everything else
        if(memcmp(&cur_tios, &raw_tios, sizeof cur_tios) != 0){
            // reset terminal props
            ret = tcsetattr(stdin_fd, TCSANOW, &cur_tios);
            if(ret != 0){
                // TODO do something
                ERRORF("error setting terminal properties back: %s\n", strerror(errno));
            }
        }
        //if(stdin_fd != STDIN_FILENO){
        //    close(stdin_fd);
        //}
        close(stdin_fd);
    }
#endif // NO_LIBGRAPHEME

    char *locale;
    locale = setlocale(LC_ALL, "");
    (void) locale;

    DEBUGF("converting to a width of %d and a height of %d\n", width, height);
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
        DEBUGF("line: %d (read %zd)\n", i, nread);
        debug_v(line);
        DEBUGF("\n");
        size_t cInc = 1;
        bool usedColor = false;
        increment = true;
        visstrlen = 0;
        for(c = prevColStart = line; (visstrlen < width) && (*c != '\n') && (*c != '\0') && (*c != '\r'); c += cInc){
            if(*c == '\033' && c[1] == '['){
                // TODO check that this is actually a color and not some other type of thing
                // if c == \033[, then starting color info, doesnt add to width
                //  - technically just starting ansi control sequence, which I'm hoping will only be color
                increment = false;
                usedColor = true;
                cInc = 1;
                DEBUGF("got color at char %zu\n", c-line);
                // save the place of the last unresolved color change to not add it to the output
                //  (we clear this if we get something to print or a full color reset so we dont actually have to set it here)
                prevColStart = c;
                // increment c because we know 2 chars here but cInc is only 1 (to check every 1 chars of the color code)
                ++c;
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
#                   ifndef NO_LIBGRAPHEME
                        cInc = grapheme_next_character_break_utf8(c, nread + line - c);
                        DEBUGF("1 char: '%.*s' (%zu bytes)\n", (int) cInc, c, cInc);
#                   else
                        cInc = 1;
#                   endif   // NO_LIBGRAPHEME
                    size_t j;
                    size_t jInc = 0;
                    int incWidth = 0;
                    for(j = 0; j < cInc; j += jInc){
                        // get the number of chars in the current multi-byte char (code point)
                        jInc = mbrlen(c + j, nread + line - c - j, &state);
                        // if it was an error then break
                        if((jInc == (size_t)-1) || (jInc == (size_t)-2)){
                            // TODO figure out error
                            ERRORF("error: mbrlen = %zd\n", jInc);
                            ERRORF("c: '%.*s', nread: %zd, line-c: %zd, j: %zu\n", (int) (nread + line - c - j), c+j, nread, line-c-j, j);
                            break;
                        }
                        // get the current multi-byte char as a wchar
                        wchar_t wcs[2] = L"";
                        mbrtowc(wcs, c+j, jInc, &state);
                        // get the width of the wide char to increment the visstrlen
                        //int cWidth = wcswidth(wcs, cInc / (sizeof wcs[0]) + 1);
                        int cWidth = wcwidth(wcs[0]);
                        DEBUGF("char '%.*s' width: %d (%zu bytes), visstrlen: %d, incWidth: %d\n", (int) jInc, c+j, cWidth, jInc, visstrlen, incWidth);
                        if(cWidth >= 0){
                            if(clust){
                                // TODO what is the size of a cluster? right now assuming its the max width of the individual chars which isnt always correct
                                if(cWidth > incWidth){
                                    incWidth = cWidth;
                                }
                            }else{
                                incWidth += cWidth;
                            }
                        }else{
                            WARNF("char '%.*s' (%zu len) unprintable\n", (int) jInc, c+j, jInc);
                        }
                        //DEBUGF("chars: %zd, width: %d, str: '%*s'\n", cInc, cWidth, (int)cInc, c);
                        // if the visstrlen would be greater than the width then were done, dont increment the visstrlen
                        if(visstrlen + incWidth > width){
                            break;
                        }
                    }
                    // if didnt finish loop above then break out of this loop too
                    if(j < cInc){
                        break;
                    }else if(j > cInc){
#                       ifndef NO_LIBGRAPHEME
                            ERRORF("error: libgrapheme grapheme cluster length and mbrlen disagree: %zu vs %zu, using mbrlen\n", cInc, j);
#                       endif // NO_LIBGRAPHEME
                        cInc = j;
                    }
//#                   ifdef NO_LIBGRAPHEME
//                        cInc = j;
//#                   endif
                    visstrlen += incWidth;
                }
                // clear the prev color start pointer since we are currently dealing with actual output
                prevColStart = c + cInc;
                //DEBUGF("%c: %d\n", *c, visstrlen);
            }else if(*c == 'm'){
                // if c == m then ending color block, will start incrementing by width again
                increment = true;
                DEBUGF("finished color at char %zu\n", c-line);
                if(!strncmp(c - (END_COL_LEN-1), END_COL_STR, END_COL_LEN)){
                    // if this is just ending a color reset str then mark as no color used
                    usedColor = false;
                }
            }
        }
        // TODO instead of resetting color at end look for color
        if(usedColor){
            DEBUGF("resetting color at char %zu (ended at %zu)\n", prevColStart - line, c - line);
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
        //DEBUGF("width: %d\n", visstrlen);

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

//static int repr_o(char c, char * outStr, size_t outStrSize){
//    int len;
//    if(isprint(c)){
//        len = 1;
//        if(outStrSize > 1){
//            outStr[0] = c;
//            outStr[1] = '\000';
//        }else if(outStrSize > 0){
//            outStr[0] = '\000';
//        }
//    }else{
//        len = snprintf(outStr, outStrSize, "\\%03o", c);
//    }
//    return len;
//}
//static int repr_x(char c, char * outStr, size_t outStrSize){
//    int len;
//    if(isprint(c)){
//        len = 1;
//        if(outStrSize > 1){
//            outStr[0] = c;
//            outStr[1] = '\000';
//        }else if(outStrSize > 0){
//            outStr[0] = '\000';
//        }
//    }else{
//        //len = snprintf(outStr, outStrSize, "0x%02x", c);
//        len = snprintf(outStr, outStrSize, "\\x%02x", c);
//    }
//    return len;
//}
enum BASES {
    BIN = 2,
    OCT = 8,
    DEC = 10,
    HEX = 16,
};
static int reprChar(const unsigned char c, char * outStr, const size_t outStrSize, const enum BASES base){
    int len;
    if(isprint(c)){
        len = 1;
        // TODO at this rate just call snprintf
        //  - or just assert that we have enough chars at the top and then we can assume we do
        if(outStrSize > 1){
            outStr[0] = c;
            outStr[1] = '\000';
        }else if(outStrSize > 0){
            outStr[0] = '\000';
        }
    }else{
        switch(base){
            case BIN:
                {
                    // \b + digit per bit
                    len = ARRAY_LENGTH("\b")-1 + 8 * sizeof c;
                    size_t i = 0;
                    outStr[i] = '\\';
                    if(i >= outStrSize) goto finish_bin; else ++i;
                    outStr[i] = 'b';
                    if(i >= outStrSize) goto finish_bin; else ++i;
                    for(size_t j = 1; j <= 8 * sizeof c; ++j){
                        //outStr[i] = () ? '1' : '0';
                        outStr[i] = ((c >> (8 * sizeof c - j)) & 0x01) + '0';
                        if(i >= outStrSize) goto finish_bin; else ++i;
                    }
finish_bin:
                    outStr[i] = '\000';
                    break;
                }
            case OCT:
                len = snprintf(outStr, outStrSize, "\\%03o", (int)c);
                break;
            case DEC:
                // NOTE dont use this
                len = snprintf(outStr, outStrSize, "\\%d", (int)c);
                break;
            case HEX:
                //len = snprintf(outStr, outStrSize, "0x%02o", (int)c);
                len = snprintf(outStr, outStrSize, "\\x%02x", (int)c);
                break;
            default:
                ERRORF("base %d unknown\n", base);
                assert(false);
        }

    }
    return len;
}
static void debug_v(char str[]){
    //char tmp[6] = {0};
    char tmp[11] = {0};
    for(char * c = str; *c != '\000'; ++c){
        reprChar(*c, STR_LEN(tmp), HEX);
        //reprChar(*c, STR_LEN(tmp), BIN);
        DEBUGF("%s", tmp);
    }
}
#ifndef NO_LIBGRAPHEME
static inline void chrError(char expected, char got){
    char expected_str[6] = {0};
    char got_str[6] = {0};
    reprChar(expected, STR_LEN(expected_str), OCT);
    reprChar(got, STR_LEN(got_str), OCT);
    WARNF("error, ANSI DEC response expected '%s' but got '%s'\n", expected_str, got_str);
}

DEC_RESPONSE readDECResponse(int fd, char mode[], size_t modelen){
    assert(modelen < 10);
    // "\033[?" *mode* ";" *response* "$y"
#   define RESPONSE_BASE_LEN  (3 + 1 + 2)
    char buf[10+10];
    ssize_t len = read(fd, buf, RESPONSE_BASE_LEN + 1 + modelen);
    if(len < 1){
        WARNF("read timed out getting response for mode %.*s from terminal: %s\n", (int) modelen, mode, strerror(errno));
    }
    size_t i = 0;
    if(buf[i++] != '\033') {
        chrError('\033', buf[i-1]);
        return DEC_UNKNOWN;
    }
    if(buf[i++] != '[')    {
        chrError('[', buf[i-1]);
        return DEC_UNKNOWN;
    }
    if(buf[i++] != '?')    {
        chrError('?', buf[i-1]);
        return DEC_UNKNOWN;
    }
    for(size_t j = 0; j < modelen; ++j){
        if(buf[i++] != mode[j]){
            chrError(mode[j], buf[i-1]);
            return DEC_UNKNOWN;
        }
    }
    if(buf[i++] != ';')    {
        chrError(';', buf[i-1]);
        return DEC_UNKNOWN;
    }
    char response = buf[i++];
    if(response < DEC_UNKNOWN || response > DEC_PERMRESET) {
        char resp_str[6] = {0};
        reprChar(response, STR_LEN(resp_str), OCT);
        WARNF("error, invalid response '%s' (expected something between %c and %c)\n", resp_str, DEC_UNKNOWN, DEC_PERMRESET);
        return DEC_UNKNOWN;
    }
    if(buf[i++] != '$')    {
        chrError('$', buf[i-1]);
        return DEC_UNKNOWN;
    }
    if(buf[i++] != 'y')    {
        chrError('y', buf[i-1]);
        return DEC_UNKNOWN;
    }
    return response;
}

#ifndef __USE_MISC
static inline void cfmakeraw(struct termios *termios_p){
    termios_p->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
            | INLCR | IGNCR | ICRNL | IXON);
    termios_p->c_oflag &= ~OPOST;
    termios_p->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    termios_p->c_cflag &= ~(CSIZE | PARENB);
    termios_p->c_cflag |= CS8;
}
#endif  // __USE_MISC
#endif  // NO_LIBGRAPHEME
