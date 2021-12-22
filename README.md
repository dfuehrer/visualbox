# visualbox
## Overview
- a simple utility to output formatted input with an exact (visual) width and height
    - the output is the same as the input except that the its lines and columns shown when outputted to a terminal are as given
- terminal color sequences are preserved and don't contribute to the width
- emojis should have the correct width
- tabs are set to a width of 4 spaces
    - align to 4 space columns as expected
- considering utf-8 allows characters up to 6 bytes (> sizeof(wchar_t)) and wider than 1 column this quickly becomes complicated without library functions
    - since utf-8 doesn't handle terminal color sequences or tab widths, those were handled manually
    - used somewhat standard C standard library functions `mbrlen`, `mbrtowc`, and `wcwidth`

## Usage
- Usage:
```bash
visualbox [file] width height
```
- if file is omitted, then `visualbox` will read from `stdin`
### Use case
- I created this to use in my [bat-minimap](https://github.com/dfuehrer/system-scripts/blob/main/bat-minimap) script
    - I use this script for previewing text files in `lf`
![demo](screenshot.png)

## Building
- build with `make(1)`
- install with `make install`
    - installs locally to ~/.local/bin by defualt
    - tweak the directories in config.mk to choose where it gets installed

## TODO
- [ ] add options:
    - [ ] delimiter at the end of lines
    - [ ] tab width
