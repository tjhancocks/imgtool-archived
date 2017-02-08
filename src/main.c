/*
  Copyright (c) 2017 Tom Hancocks
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <string.h>
#include <stdlib.h>
#include "vdevice.h"
#include "fat12.h"
#include "vfs.h"
#include "shell.h"

// Example input from command line:
//
//  vfs [-bs <value>] [-count <value>] [-fs fat12] [--init] <disk-image>
//
// This opens up a "shell" that is interacting with the disk image.
// "Shell" commands are as follows:
//
//  - cd path
//  - ls
//  - mkdir name
//  - touch name
//  - cp src dest
//  - mv src dest
//  - lcp external-src dest
//  - lmv external-src dest
//  - rm name
//

int main(int argc, const char * argv[])
{

    // We need to ensure we have at least 2 arguments.
    if (argc < 2) {
        fprintf(stderr, "Too few arguments supplied.\n");
        return 1;
    }
    
    // Configuration variables. We will extract the values and replace them
    // as we parse the arguments.
    uint16_t bps = 512;
    uint32_t count = 2880;
    const char *fs = "fat12";
    const char *path = NULL;
    const char *cmd = NULL;
    
    // Iterate through the arguments and parse them, we should skip the first
    // argument as it is the program name.
    uint8_t arg = 0;
    while (++arg < argc) {
        
        // check the argument. flagged values are prefixed with a '-'
        if (argv[arg][0] == '-') {
            // this is a configuration value
            if (strcmp(argv[arg], "-bs") == 0) {
                // set the byte per sector
                bps = atoi(argv[++arg]);
            }
            else if (strcmp(argv[arg], "-fs") == 0) {
                // set the filesystem
                fs = argv[++arg];
            }
            else if (strcmp(argv[arg], "-count") == 0) {
                // set the number of sectors
                count = atoi(argv[++arg]);
            }
            else if (argv[arg][1] == '-') {
                // It is a double hyphen and thus an explicit command.
                cmd = argv[arg] + 2;
            }
        }
        else {
            // this is not a configuration value and should be treated as the
            //disk image
            path = argv[arg];
        }
    }
    
    // Create a new device in preparation for use and setup a virtual file
    // system. Depending on the initial command, this may need to initialise a
    // blank image.
    vdevice_t dev = NULL;
    if (cmd && strcmp(cmd, "init") == 0) {
        dev = device_init_blank(path, bps, count);
    }
    else {
        dev = device_init(path);
    }
    
    // Create the appropriate virtual file system interface so that the correct
    // concrete file system is communicated with.
    vfs_interface_t filesystem = NULL;
    if (strcmp(fs, "fat12") == 0) {
        filesystem = fat12_init();
    }
    
    // Create the virtual file system and launch the shell.
    vfs_t vfs = vfs_init(dev, filesystem);
    shell_t shell = shell_init(vfs);
    shell_do(shell);
    
    // Close the virtual file system. This should clean up everything that is
    // still in memory.
    vfs_destroy(vfs);

    // Report success
    return 0;
}
