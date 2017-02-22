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
#include <string.h>
#include <unistd.h>

#include <common/host.h>
#include <shell/grub.h>
#include <shell/shell.h>
#include <grub/install.h>

static inline const char *copy_string(const char *string)
{
    size_t len = strlen(string);
    char *new_string = calloc(len + 1, sizeof(*new_string));
    memcpy(new_string, string, len);
    return new_string;
}

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
                case 'd': // The destination location of the GRUB installation
                    cfg.install_path = copy_string(optarg);
                    break;
                    
                case 'c': // The path of the GRUB configuration file
                    cfg.configuration_path = copy_string(optarg);
                    break;
                    
                case 'n': // The name of the OS to list in the GRUB menu
                    cfg.os_name = copy_string(optarg);
                    break;
                    
                case 'r': // The root of the OS Boot volume
                    cfg.root_name = copy_string(optarg);
                    break;
                    
                case 'k': // The location of the kernel
                    cfg.kernel_path = copy_string(optarg);
                    break;
                    
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
