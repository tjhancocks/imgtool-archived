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

void shell_init_dev(shell_t shell, int argc, const char *argv[])
{
    assert(shell);

    uint16_t bps = 0;
    uint32_t count = 0;

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

    // Check to ensure the values are correct
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

    // Check for an attached device
    if (!shell->attached_device) {
        fprintf(stderr, "Please attach a device to initialise.\n");
        fprintf(stderr, "Devices can be attached using the `attach` command\n");
        return;
    }

    // Perform the operation
    device_init(shell->attached_device, bps, count);
}
