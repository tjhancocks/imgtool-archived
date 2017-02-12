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

#include <common/host.h>
#include <shell/grub.h>
#include <shell/shell.h>
#include <grub/install.h>

int shell_grub(struct shell *shell, int argc, const char *argv[])
{
    assert(shell);
    
    struct grub_configuration cfg = {0};
    
    // Parse out all command line options provided
    int c = 0;
    optind = 1;
    while (optind < argc) {
        if ((c = getopt(argc, (char **)argv, "d:c:n:r:k:")) != -1) {
            switch (c) {
                default:
                    break;
            }
        }
        else {
            free((void *)cfg.source_path);
            cfg.source_path = (const char *)host_expand_path(argv[optind]);
            optind++;
        }
    }
    
    // Trigger the GRUB installer.
    grub_install(shell->device_filesystem, cfg);
    
    return SHELL_OK;
}
