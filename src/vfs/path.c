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

        // we've encountered the dot. switc to extension mode!
        if (c == '.') {
            ptr = *ext;
            file++;
            continue;
        }

        *ptr++ = *file++;
    }
}


path_node_t vfs_construct_path(const char *path)
{
    assert(path);

    // If the path is representing the root of the file system we need to return
    // NULL.
    if (strcmp(path, "/") == 0) {
        return NULL;
    }
    
    return NULL
}
