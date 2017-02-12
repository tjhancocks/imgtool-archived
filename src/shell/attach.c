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
#include <string.h>

#include <shell/attach.h>
#include <shell/shell.h>
#include <device/virtual.h>
#include <common/host.h>


static enum vmedia_type _parse_media_type(const char *type)
{
    if (strcmp(type, "fdd") == 0 || strcmp(type, "fd0") == 0) {
        return vmedia_floppy;
    }
    else if (strcmp(type, "hdd") == 0 || strcmp(type, "hd0") == 0) {
        return vmedia_hard_disk;
    }
    else {
        return vmedia_floppy;
    }
}

void shell_attach(shell_t shell, int argc, const char *argv[])
{
    assert(shell);

    enum vmedia_type media = vmedia_floppy;
    int no_exist = 0;
    const char *path = NULL;
    
    int c = 0;
    optind = 1;
    while (optind < argc) {
        if ((c = getopt(argc, (char **)argv, "m:c")) != -1) {
            switch (c) {
                case 'm': // The Media type of the image being attached.
                    media = _parse_media_type(optarg);
                    break;
                    
                case 'c': // The user has indicated that no replace should
                          // happen.
                    no_exist = 1;
                    break;
                    
                default:
                    break;
            }
        }
        else {
            free((void *)path);
            path = (const char *)host_expand_path(argv[optind]);
            optind++;
        }
    }
    
    // Check for a previous attached device. If one is attached then ensure
    // it isn't mounted.
    if (shell->attached_device && shell->device_filesystem) {
        fprintf(stderr, "Currently attached device is mounted. Aborting.\n");
        return;
    }
    
    // If the no_exist flag is set, then try and determine if the file currently
    // exists. If it does then fail here.
    if (no_exist) {
        FILE *fp = fopen(path, "r");
        if (fp) {
            fclose(fp);
            fprintf(stderr, "Specified file already exists. Aborting.\n");
            return;
        }
    }

    // Create the device.
    shell->attached_device = device_create(path, media);
    free((void *)path);
}

void shell_detach(shell_t shell, int argc, const char *argv[])
{
    assert(shell);

    // Check for an attached device.
    if (!shell->attached_device) {
        return;
    }

    // If the device is mounted then abort the process.
    if (shell->device_filesystem) {
        fprintf(stderr, "Unable to detach mounted device.\n");
        return;
    }

    device_destroy(shell->attached_device);
    shell->attached_device = NULL;
}
