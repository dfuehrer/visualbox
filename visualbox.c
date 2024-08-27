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
//[1;2;95m1234567890[48;5;24m1234567890[38;2;250;25;30m1234567890
//🧑‍🌾
//Tëst 👨‍👩‍👦 🇺🇸 नी நி!
//u̲n̲d̲e̲r̲[0;4ml̲i̲n̲[21me̲[md̲

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

enum BASES {
    BIN = 2,
    OCT = 8,
    DEC = 10,
    HEX = 16,
};

static int reprChar(const unsigned char c, char * outStr, const size_t outStrSize, const enum BASES base);
static void chrError(char expected, char got);
DEC_RESPONSE readDECResponse(int fd, char mode[], size_t modelen);

//#define MAX_CC  ((1 << (8*sizeof (cc_t))) - 1)
#define MAX_CC  ((cc_t) -1)
#endif  // NO_LIBGRAPHEME

typedef unsigned char SGRCode;
//typedef StringView SGRCode;
typedef struct SGRColor {
    SGRCode code;
    SGRCode depth;
    union {
        SGRCode ind;
        struct {
            SGRCode r;
            SGRCode g;
            SGRCode b;
        };
    };
} SGRColor;
union SGR {
    struct SGRSet {
        struct SimpleSGRCodes {
            SGRCode bold;        // 1
            SGRCode faint;       // 2, 22    // 22 reset bold and faint
            SGRCode italic;      // 3, 23    // 23 reset italic
            SGRCode underline;   // 4, 24    // 24 reset underline
            SGRCode blink;       // 5, 6, 25 // 25 reset blink
            SGRCode invert;      // 7, 27    // 27 reset inverse
            SGRCode hide;        // 8, 28    // 28 reset hide
            SGRCode strikeout;   // 9, 29    // 29 reset stikeout
            SGRCode font;        // 10, 11-19
            SGRCode fraktur;     // 20   // i dont know this word but i guess its a font thing
            SGRCode dunderline;  // 21   // either double underline or disable bold... so it kinda double dips
            SGRCode space;       // 26, 50   // 50 reset proportional spacing
            SGRCode frame;       // 51
            SGRCode circle;      // 52, 54  // 54 reset framed and circled
            SGRCode overline;    // 53, 55  // 55 reset overlined
            SGRCode ideogram;    // 60-64, 65   // 65 reset ideogram
            SGRCode superscript; // 73, 74, 75  // 75 reset superscript and subscript
        } simpleCodes;
        SGRColor fg_color;   // 30-37, 38, 39, 90-97    // 39 reset fg color
        SGRColor bg_color;   // 40-47, 48, 49, 100-107  // 49 reset fg color
        SGRColor ul_color;   // 58, 59  // 59 reset underline color
    } codeStruct;
    SGRCode codeArr[sizeof (struct SGRSet) / sizeof (SGRCode)];
};

enum SGRCodes {
    SGR_RESET = 0,
    SGR_BOLD  = 1,
    SGR_FAINT = 2,
    SGR_UNSET_BOLD = 22,
    SGR_ITALIC = 3,
    SGR_UNSET_ITALIC = 23,
    SGR_UNDERLINE = 4,
    SGR_UNSET_UNDERLINE = 24,
    SGR_SLOW_BLINK = 5,
    SGR_RAPID_BLINK = 6,
    SGR_UNSET_BLINK = 25,
    SGR_INVERT = 7,
    SGR_UNSET_INVERT = 27,
    SGR_HIDE = 8,
    SGR_UNSET_HIDE = 28,
    SGR_STRIKE = 9,
    SGR_UNSET_STRIKE = 29,
    SGR_PRIMARY_FONT = 10,
    SGR_ALT_FONT1 = 11,
    SGR_ALT_FONT2 = 12,
    SGR_ALT_FONT3 = 13,
    SGR_ALT_FONT4 = 14,
    SGR_ALT_FONT5 = 15,
    SGR_ALT_FONT6 = 16,
    SGR_ALT_FONT7 = 17,
    SGR_ALT_FONT8 = 18,
    SGR_ALT_FONT9 = 19,
    SGR_FRANKTUR = 20,
    SGR_DUNDERLINE = 21,
    SGR_SPACING = 26,
    SGR_UNSET_SPACING = 50,
    SGR_FG_BLACK = 30,
    SGR_FG_RED = 31,
    SGR_FG_GREEN = 32,
    SGR_FG_YELLOW = 33,
    SGR_FG_BLUE = 34,
    SGR_FG_MAGENTA = 35,
    SGR_FG_CYAN = 36,
    SGR_FG_WHITE = 37,
    SGR_FG_BRIGHT_BLACK = 90,
    SGR_FG_BRIGHT_RED = 91,
    SGR_FG_BRIGHT_GREEN = 92,
    SGR_FG_BRIGHT_YELLOW = 93,
    SGR_FG_BRIGHT_BLUE = 94,
    SGR_FG_BRIGHT_MAGENTA = 95,
    SGR_FG_BRIGHT_CYAN = 96,
    SGR_FG_BRIGHT_WHITE = 97,
    SGR_FG_COLOR = 38,
    SGR_UNSET_FG_COLOR = 39,
    SGR_BG_BLACK = 40,
    SGR_BG_RED = 41,
    SGR_BG_GREEN = 42,
    SGR_BG_YELLOW = 43,
    SGR_BG_BLUE = 44,
    SGR_BG_MAGENTA = 45,
    SGR_BG_CYAN = 46,
    SGR_BG_WHITE = 47,
    SGR_BG_BRIGHT_BLACK = 100,
    SGR_BG_BRIGHT_RED = 101,
    SGR_BG_BRIGHT_GREEN = 102,
    SGR_BG_BRIGHT_YELLOW = 103,
    SGR_BG_BRIGHT_BLUE = 104,
    SGR_BG_BRIGHT_MAGENTA = 105,
    SGR_BG_BRIGHT_CYAN = 106,
    SGR_BG_BRIGHT_WHITE = 107,
    SGR_BG_COLOR = 48,
    SGR_UNSET_BG_COLOR = 49,
    SGR_FRAMED = 51,
    SGR_CIRCLED = 52,
    SGR_UNSET_FRAME = 54,
    SGR_OVERLINE = 53,
    SGR_UNSET_OVERLINE = 55,
    SGR_UNSET_UL_COLOR = 59,
    SGR_UL_COLOR = 58,
    SGR_IDEOGRAM_UL = 60,
    SGR_IDEOGRAM_DL = 61,
    SGR_IDEOGRAM_OL = 62,
    SGR_IDEOGRAM_DOL = 63,
    SGR_IDEOGRAM_STRESS = 64,
    SGR_UNSET_IDEOGRAM = 65,
    SGR_SUPERSCRIPT = 73,
    SGR_SUBSCRIPT = 74,
    SGR_UNSET_SCRIPT = 75,
};

enum ParseSGRState {
    PARSE_ERROR,    // found invalid format
    PARSE_CONT,     // ;
    PARSE_FINISH,   // m
    PARSE_CLEAR,    // clear out (0), implies m (if it just ended in ; then its continuing and isnt really a clear)
};
enum ParseSGRState setSGRCode(struct SGRSet * codes, const char * str, char ** endptr);
static enum ParseSGRState setSGRColor(struct SGRColor * color, char ** endptr);
static void printSGRColor(const SGRColor * color, bool * havePrinted);

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
    bool processAllSGR = true;
    bool printSGRNewLine = true;
    bool ansifilter = false;
    map_t params;
    map_t flags;
    initMap(&params);
    initMap(&flags);
    addMapMembers(&flags , NULL,    BOOL, false, "Ssd",  STRVIEW("help"  ), "h", 1);
    MapData * widthNode  = addMapMembers(&params, NULL,    INT,  false, "SsdS", STRVIEW("width" ), "w", 1, STRVIEW("rows"));
    MapData * heightNode = addMapMembers(&params, NULL,    INT,  false, "SsdS", STRVIEW("height"), "h", 1, STRVIEW("columns"));
    addMapMembers(&params, &tabsize,INT,  true,  "Ssd",  STRVIEW("tabs"  ), "t", 1);
    addMapMembers(&params, delim,   STR,  true,  "Ssd",  STRVIEW("delim" ), "d", 1);
    MapData * processAllSGRNode     = addMapMembers(&flags , &flagTrue ,   BOOL, false,  "Ssd",  STRVIEW("process-whole-line"), STR_LEN("p"));
    MapData * noProcessAllSGRNode   = addMapMembers(&flags , &flagFalse,   BOOL, false,  "Ssd",  STRVIEW("no-process-whole-line"), STR_LEN("P"));
    MapData * printSGRNewLineNode   = addMapMembers(&flags , &flagTrue ,   BOOL, false,  "Ssd",  STRVIEW("multiline-color"), STR_LEN("m"));
    MapData * noPrintSGRNewLineNode = addMapMembers(&flags , &flagFalse,   BOOL, false,  "Ssd",  STRVIEW("no-multiline-color"), STR_LEN("M"));
    MapData * ansiFilterNode        = addMapMembers(&flags , &flagFalse,   BOOL, false,  "Ssd",  STRVIEW("ansifilter"), STR_LEN("A"));
    setNodeNegation(    processAllSGRNode,   noProcessAllSGRNode);
    setNodeNegation(  noProcessAllSGRNode,     processAllSGRNode);
    setNodeNegation(  printSGRNewLineNode, noPrintSGRNewLineNode);
    setNodeNegation(noPrintSGRNewLineNode,   printSGRNewLineNode);
    setNodeNegation(    processAllSGRNode, noPrintSGRNewLineNode);
    setNodeNegation(  printSGRNewLineNode,        ansiFilterNode);
    setNodeNegation(    processAllSGRNode,        ansiFilterNode);
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
    processAllSGR   = getNode_bool(  processAllSGRNode);
    printSGRNewLine = getNode_bool(printSGRNewLineNode);
    ansifilter = getNode_bool(ansiFilterNode);
    DEBUGF("p: %d, m: %d, A: %d\n", processAllSGR, printSGRNewLine, ansifilter);
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
        WARNF("Seriously, if you want to output nothing there are easier ways\n");
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

    // start by allocating the bare minimum size
    char * c = NULL;
    char * prevColStart = c;
    size_t len = (width + 1) * (sizeof *c);
    char * line = (char *)malloc(len);
    memset(line, 0, len);       // set the line to all NULL chars
    ssize_t nread = 0;
    mbstate_t state = {0};
    int i = 1;
    union SGR codes = {0};
    enum ParseSGRState SGRstate = PARSE_CONT;
    for(int lineno = 0; lineno < height && ((nread = getline(&line, &len, file)) != -1); ++lineno){
        DEBUGF("line: %d (read %zd)\n", i, nread);
        debug_v(line);
        DEBUGF("\n");
        size_t cInc = 1;
        visstrlen = 0;
        for(c = prevColStart = line; (visstrlen < width) && (*c != '\n') && (*c != '\0') && (*c != '\r'); c += cInc){
            if(*c == '\033' && c[1] == '['){
                DEBUGF("got color at char %zu\n", c-line);
                char * tmp = c + 2;
                SGRstate = PARSE_CONT;
                while(SGRstate == PARSE_CONT){
                    SGRstate = setSGRCode(&codes.codeStruct, tmp, &tmp);
                    if(SGRstate == PARSE_ERROR){
                        DEBUGF("error, bad SGR code, moving on like it was fine\n");
                        // TODO do something maybe depending on args
                        // - maybe clear codes
                    }
                }
                DEBUGF("end color at char %zu\n", tmp-line);
                cInc = 1;
                if(!ansifilter){
                    c = tmp - cInc;
                }else{
                    memmove(c, tmp, nread - (tmp-line));
                    --c;
                }
                continue;
            }
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
#               ifndef NO_LIBGRAPHEME
                    cInc = grapheme_next_character_break_utf8(c, nread + line - c);
                DEBUGF("1 char: '%.*s' (%zu bytes)\n", (int) cInc, c, cInc);
#               else
                    cInc = 1;
#               endif   // NO_LIBGRAPHEME
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
#                   ifndef NO_LIBGRAPHEME
                        ERRORF("error: libgrapheme grapheme cluster length and mbrlen disagree: %zu vs %zu, using mbrlen\n", cInc, j);
#                   endif // NO_LIBGRAPHEME
                    cInc = j;
                }
//#               ifdef NO_LIBGRAPHEME
//                    cInc = j;
//#               endif
                visstrlen += incWidth;
            }
            // clear the prev color start pointer since we are currently dealing with actual output
            prevColStart = c + cInc;
            //DEBUGF("%c: %d\n", *c, visstrlen);
        }
        if(ansifilter){
            // probably dont want the possibility of printing color on next line after clearing color from this line
            SGRstate = PARSE_CLEAR;
        }
        enum ParseSGRState endSGRstate = SGRstate;

        if(processAllSGR){
            for( ; (*c != '\n') && (*c != '\0') && (*c != '\r'); ++c){
                if(*c == '\033' && c[1] == '['){
                    DEBUGF("got color at char %zu\n", c-line);
                    char * tmp = c + 2;
                    SGRstate = PARSE_CONT;
                    while(SGRstate == PARSE_CONT){
                        SGRstate = setSGRCode(&codes.codeStruct, tmp, &tmp);
                        if(SGRstate == PARSE_ERROR){
                            DEBUGF("error, bad SGR code, moving on like it was fine\n");
                            // TODO do something maybe depending on args
                            // - maybe clear codes
                        }
                    }
                    c = tmp - 1;
                    DEBUGF("end color at char %zu\n", c-line);
                    continue;
                }
            }
            if(ansifilter){
                // probably dont want the possibility of printing color on next line after clearing color from this line
                SGRstate = PARSE_CLEAR;
            }
        }

        // TODO instead of resetting color at end look for color
        c = prevColStart;
        if(endSGRstate == PARSE_FINISH){
            DEBUGF("resetting color at char %zu (ended at %zu)\n", prevColStart - line, c - line);
            // add color reset at end
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

        // finally print the line at the fixed (visual) width
        printf("%s%-*s%s\n", line, width - visstrlen, "", delim);

        // TODO add args to turn on setting codes for new lines
        if(printSGRNewLine && SGRstate == PARSE_FINISH){
            bool first = true;
            for(size_t codeNum = 0; codeNum < sizeof codes.codeStruct.simpleCodes; ++codeNum){
                if(codes.codeArr[codeNum] != 0){
                    if(first){
                        first = false;
                        printf("\033[");
                    }else{
                        putchar(';');
                    }
                    printf("%hhu", codes.codeArr[codeNum]);
                }
            }
            printSGRColor(&codes.codeStruct.fg_color, &first);
            printSGRColor(&codes.codeStruct.bg_color, &first);
            printSGRColor(&codes.codeStruct.ul_color, &first);
            if(!first){
                putchar('m');
            }
        }

        ++i;
    }
    // TODO if height > num lines in file, maybe pad with whitespace

    // TODO maybe dont do final free just before quitting since it just takes time for no reason
    free(line);
    return 0;
}

static void printSGRColor(const SGRColor * color, bool * havePrinted){
    if(color->code != 0){
        if(*havePrinted){
            *havePrinted = false;
            printf("\033[");
        }else{
            putchar(';');
        }
        printf("%hhu", color->code);
        if(color->code == 38){
            putchar(';');
            if(color->depth == 5){
                printf("5;%hhu", color->ind);
            }else if(color->depth == 2){
                printf("2;%hhu;%hhu;%hhu", color->r, color->g, color->b);
            }
        }
    }
}

static enum ParseSGRState readSGRCode(SGRCode * code, const char * str, char ** endptr){
    long num = strtol(str, endptr, 10);
    // TODO prolly check errno
    // if str at ; or m then input skipped because it was 0
    if((*endptr)[0] != ';' && (*endptr)[0] != 'm'){
        // error because found something that wasnt ;, m, or int
        //DEBUGF("could not interpret ANSI SGR code '%.*s', does not end with ';' or 'm'\n", (int)(*endptr - str), str);
        DEBUGF("could not interpret ANSI SGR code '");
        for(const char * c = str; c < *endptr && *c != '\000'; ++c){
            char tmp[11] = {0};
            reprChar(*c, STR_LEN(tmp), HEX);
            //reprChar(*c, STR_LEN(tmp), OCT);
            DEBUGF("%s", tmp);
        }
        DEBUGF("' (len %zu), does not end with ';' or 'm'\n", (size_t)(*endptr - str));
        return PARSE_ERROR;
    }
    // increment endptr to get past ; or m
    ++(*endptr);
    if(num > (unsigned char) -1){
        DEBUGF("could not interpret ANSI SGR code '%ld'\n", num);
        return PARSE_ERROR;
    }
    *code = num;

    DEBUGF("end with char '%c': %d\n", (*endptr)[-1], ((*endptr)[-1] == ';') ? PARSE_CONT : PARSE_FINISH);
    return ((*endptr)[-1] == ';') ? PARSE_CONT : PARSE_FINISH;
}
static enum ParseSGRState setSGRColor(struct SGRColor * color, char ** endptr){
    enum ParseSGRState state;
    state = readSGRCode(&color->depth, *endptr, endptr);
    if(state != PARSE_CONT){
        //return state;
        return PARSE_ERROR;
    }
    if(color->depth == 5){
        // 256 color pallette
        state = readSGRCode(&color->ind, *endptr, endptr);
    }else if(color->depth == 2){
        // full color (rgb)
        state = readSGRCode(&color->r, *endptr, endptr);
        if(state != PARSE_CONT){
            //state = PARSE_ERROR;
            goto ret;
        }
        state = readSGRCode(&color->g, *endptr, endptr);
        if(state != PARSE_CONT){
            //state = PARSE_ERROR;
            goto ret;
        }
        state = readSGRCode(&color->b, *endptr, endptr);
    }else{
        DEBUGF("could not interpret ANSI SGR code %d, expected 2 or 5\n", color->depth);
        return PARSE_ERROR;
    }
ret:
    return state;
}
enum ParseSGRState setSGRCode(struct SGRSet * codes, const char * str, char ** endptr){

    SGRCode code = 0;
    enum ParseSGRState ret = readSGRCode(&code, str, endptr);
    DEBUGF("got code %hhd, state: %d\n", code, ret);

    switch(code){
        case SGR_RESET            : memset(codes, 0, sizeof *codes);         break;

        case SGR_BOLD             : codes->simpleCodes.bold        = code;   break;
        case SGR_FAINT            : codes->simpleCodes.faint       = code;   // fallthrough
        case SGR_UNSET_BOLD       : codes->simpleCodes.bold        = code;   break;
        case SGR_ITALIC           :
        case SGR_UNSET_ITALIC     : codes->simpleCodes.italic      = code;   break;
        case SGR_UNDERLINE        :
        case SGR_UNSET_UNDERLINE  : codes->simpleCodes.underline   = code;   break;
        case SGR_SLOW_BLINK       :
        case SGR_RAPID_BLINK      :
        case SGR_UNSET_BLINK      : codes->simpleCodes.blink       = code;   break;
        case SGR_INVERT           :
        case SGR_UNSET_INVERT     : codes->simpleCodes.invert      = code;   break;
        case SGR_HIDE             :
        case SGR_UNSET_HIDE       : codes->simpleCodes.hide        = code;   break;
        case SGR_STRIKE           :
        case SGR_UNSET_STRIKE     : codes->simpleCodes.strikeout   = code;   break;
        case SGR_PRIMARY_FONT     :
        case SGR_ALT_FONT1        :
        case SGR_ALT_FONT2        :
        case SGR_ALT_FONT3        :
        case SGR_ALT_FONT4        :
        case SGR_ALT_FONT5        :
        case SGR_ALT_FONT6        :
        case SGR_ALT_FONT7        :
        case SGR_ALT_FONT8        :
        case SGR_ALT_FONT9        : codes->simpleCodes.font        = code;   break;
        case SGR_FRANKTUR         : codes->simpleCodes.fraktur     = code;   break;      // i dont know this word but i guess its a font thing
        case SGR_DUNDERLINE       : codes->simpleCodes.dunderline  = code;   break;      // either double underline or disable bold... so it kinda double dips
        case SGR_SPACING          :
        case SGR_UNSET_SPACING    : codes->simpleCodes.space       = code;   break;
        case SGR_FG_BLACK         :
        case SGR_FG_RED           :
        case SGR_FG_GREEN         :
        case SGR_FG_YELLOW        :
        case SGR_FG_BLUE          :
        case SGR_FG_MAGENTA       :
        case SGR_FG_CYAN          :
        case SGR_FG_WHITE         :
        case SGR_FG_BRIGHT_BLACK  :
        case SGR_FG_BRIGHT_RED    :
        case SGR_FG_BRIGHT_GREEN  :
        case SGR_FG_BRIGHT_YELLOW :
        case SGR_FG_BRIGHT_BLUE   :
        case SGR_FG_BRIGHT_MAGENTA:
        case SGR_FG_BRIGHT_CYAN   :
        case SGR_FG_BRIGHT_WHITE  :
        case SGR_UNSET_FG_COLOR   : codes->fg_color.code = code;   break;
        case SGR_FG_COLOR         : codes->fg_color.code = code;
                                    ret = setSGRColor(&codes->fg_color, endptr);
                                    break;
        case SGR_BG_BLACK         :
        case SGR_BG_RED           :
        case SGR_BG_GREEN         :
        case SGR_BG_YELLOW        :
        case SGR_BG_BLUE          :
        case SGR_BG_MAGENTA       :
        case SGR_BG_CYAN          :
        case SGR_BG_WHITE         :
        case SGR_BG_BRIGHT_BLACK  :
        case SGR_BG_BRIGHT_RED    :
        case SGR_BG_BRIGHT_GREEN  :
        case SGR_BG_BRIGHT_YELLOW :
        case SGR_BG_BRIGHT_BLUE   :
        case SGR_BG_BRIGHT_MAGENTA:
        case SGR_BG_BRIGHT_CYAN   :
        case SGR_BG_BRIGHT_WHITE  :
        case SGR_UNSET_BG_COLOR   : codes->bg_color.code = code;   break;
        case SGR_BG_COLOR         : codes->bg_color.code = code;
                                    ret = setSGRColor(&codes->bg_color, endptr);
                                    break;
        case SGR_FRAMED           : codes->simpleCodes.frame       = code;   break;
        case SGR_CIRCLED          : codes->simpleCodes.circle      = code;   // fallthrough
        case SGR_UNSET_FRAME      : codes->simpleCodes.frame       = code;   break;
        case SGR_OVERLINE         :
        case SGR_UNSET_OVERLINE   : codes->simpleCodes.overline    = code;   break;
        case SGR_UNSET_UL_COLOR   : codes->ul_color.code = code;   break;
        case SGR_UL_COLOR         : codes->ul_color.code = code;
                                    ret = setSGRColor(&codes->ul_color, endptr);
                                    break;
        case SGR_IDEOGRAM_UL      :
        case SGR_IDEOGRAM_DL      :
        case SGR_IDEOGRAM_OL      :
        case SGR_IDEOGRAM_DOL     :
        case SGR_IDEOGRAM_STRESS  :
        case SGR_UNSET_IDEOGRAM   : codes->simpleCodes.ideogram    = code;   break;
        case SGR_SUPERSCRIPT      :
        case SGR_SUBSCRIPT        :
        case SGR_UNSET_SCRIPT     : codes->simpleCodes.superscript = code;   break;
        default                   : ret = PARSE_ERROR;
    }

    return ret;
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
