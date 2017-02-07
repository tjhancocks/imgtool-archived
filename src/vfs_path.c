#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "vfs_path.h"


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
