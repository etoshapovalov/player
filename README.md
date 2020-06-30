# Player - Simple video player written in GTK & C
- Created by Andrey Shapovalov

## Usage info
`./player <path to your videofile>`

## Makefile info
Run with `DEBUG=1` env variable for build with debug info (for GDB)

`make` - builds executable  
`make run` - runs builded executable  
`make run-debug` - runs GDB with builded executable connected  

## Tools and libraries used
- GTK
- GStreamer
- GDB (for debugging)