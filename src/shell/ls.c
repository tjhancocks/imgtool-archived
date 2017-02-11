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

#include <shell/ls.h>
#include <shell/shell.h>

#include <vfs/vfs.h>
#include <vfs/directory.h>
#include <vfs/node.h>

void shell_ls(shell_t shell, int argc, const char *argv[])
{
    // Ignore all arguments. We don't need them.
    vfs_directory_t dir = vfs_get_directory(shell->device_filesystem);
    if (!dir) {
        fprintf(stderr, "Unable to list directory\n");
        return;
    }
    
    vfs_node_t node = dir->first;
    while (node && node->state == vfs_node_used) {
        // Get some meta data to help with the display.
        enum vfs_node_attributes vfsa = node->attributes;
        printf("%c", vfsa & vfs_node_directory_attribute ? 'D' : '-');
        printf("%c", vfsa & vfs_node_hidden_attribute ? 'H' : '-');
        printf("%c", vfsa & vfs_node_read_only_attribute ? 'R' : '-');
        printf("%c", vfsa & vfs_node_system_attribute ? 'S' : '-');
        
        // Get the modification date of the file
        time_t mod = node->modification_time;
        struct tm ts = *localtime(&mod);
        char date_buf[80];
        strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", &ts);
        printf(" %s", date_buf);
        
        printf(" %08dB ", node->size);
        printf("%s\n", node->name);
        node = node->next;
    }
}
