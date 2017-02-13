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

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <vfs/path.h>


void vfs_parse_filename(const char *file, char **name, char **ext)
{
    assert(file);
    assert(name);
    assert(ext);

    // allocate the name and extension
    *name = calloc(255, sizeof(**name));
    *ext = calloc(255, sizeof(**ext));

    // extract the values
    char *ptr = *name;
    while (*file) {
        char c = *file;

        // we've encountered the dot. switch to extension mode!
        if (c == '.') {
            ptr = *ext;
            file++;
            continue;
        }

        *ptr++ = *file++;
    }
}


vfs_path_node_t vfs_construct_path(const char *path)
{
    assert(path);

    // Is the path a relative or absolute one?
    vfs_path_node_t first = NULL;
    vfs_path_node_t last = NULL;
    if (*path == '/') {
        first = calloc(1, sizeof(*first));
        first->is_root = 1;
        last = first;
        path++;
    }

    // Parse the path.
    uint8_t component_count = 0;
    uint32_t path_len = (uint32_t)strlen(path);
    uint32_t start = 0;
    for (uint32_t i = 0; i <= path_len; ++i) {

        // We're at a path component boundary
        if (path[i] == '/' || i == path_len) {

            // Extract the text for the component
            uint32_t length = i - start;
            char *component = calloc(length, sizeof(*component));
            memcpy(component, path + start, length);

            vfs_path_node_t node = NULL;
            if (component_count == 0 && first) {
                node = first;
            }
            else {
                node = calloc(1, sizeof(*node));
                if (last) {
                    last->next = node;
                }
                last = node;

                if (!first) {
                    first = node;
                }
            }

            node->name = component;

            // Reset for the next component.
            start = i + 1;
            component_count++;

        }

    }
    
    return first;
}
