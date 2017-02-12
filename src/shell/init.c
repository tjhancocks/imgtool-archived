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

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <shell/init.h>
#include <shell/shell.h>
#include <device/virtual.h>

#define FLOPPY_DEFAULT_BPS      512
#define FLOPPY_DEFAULT_SECTORS  2880

void shell_init_dev(shell_t shell, int argc, const char *argv[])
{
    assert(shell);
    
    // Check for an attached device first. We need the media type to correctly
    // handle this.
    if (!shell->attached_device) {
        fprintf(stderr, "Please attach a device to initialise.\n");
        fprintf(stderr, "Devices can be attached using the `attach` command\n");
        return;
    }

    
    // Get the arguments and values that were passed to the command.
    uint16_t bps = 0;
    uint32_t count = 0;
    
    // Before we do that check to see if the device media is a Floppy Disk. If
    // it is we can infer these values. Set them now so that they can be
    // overwritten if the user desires.
    if (shell->attached_device->media == vmedia_floppy) {
        bps = FLOPPY_DEFAULT_BPS;
        count = FLOPPY_DEFAULT_SECTORS;
    }
    
    // Parse the arguments...
    int c = 0;
    optind = 1;
    while ((c = getopt(argc, (char **)argv, "b:c:")) != -1) {
        switch (c) {
            case 'b': // Bytes Per Sector
                bps = atoi(optarg);
                break;

            case 'c': // Sector Count
                count = atoi(optarg);
                break;

            default:
                break;
        }
    }

    // Check to ensure the values are correct. Ensure neither value is 0.
    if (bps == 0) {
        fprintf(stderr, "You must specify the bytes per sector.\n");
        fprintf(stderr, "Usage: init -b <bps> -c <count>\n");
        return;
    }
    else if (count == 0) {
        fprintf(stderr, "You must specify the sector count.\n");
        fprintf(stderr, "Usage: init -b <bps> -c <count>\n");
        return;
    }
    
    // TODO: Possible check here to ensure floppy disk values make sense?
    
    // Perform the operation
    device_init(shell->attached_device, bps, count);
}
