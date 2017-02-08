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
#include <stdint.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>

#include "shell_parser.h"

void shell_parse(const char *input, int *argc, char ***argv)
{
    assert(argv);
    assert(argc);

    if (strlen(input) == 0) {
        return;
    }

    *argv = malloc(sizeof(void *) * 64);
    memset(*argv, 0, sizeof(void *) * 64);

    uint32_t start = 0;
    uint32_t length = 0;
    uint8_t escaped = 0;
    uint8_t in_string = 0;

    for (uint32_t i = 0; i < strlen(input) + 1; ++i) {
        char c = input[i];

        if ((!escaped && !in_string && isspace(c)) || c == EOF || c == '\0') {
            // get the token
            length = i - start;
            char *token = calloc(length + 1, sizeof(*token));
            strncpy(token, input + start, length);

            // Strip quotes
            if (length >= 2 && token[0] == '"') {
                token++;
            }
            if (length >= 2 && token[length - 2] == '"') {
                token[length - 2] = '\0';
            }

            // Store the argument appropriately
            *(*argv + *argc) = token;
            *argc = *argc + 1;

            // reset for next
            start = i + 1;
        }
        else if (!escaped && c == '"' && in_string == 0) {
            in_string = 1;
            start = i;
        }
        else if (!escaped && c == '"' && in_string == 1) {
            in_string = 0;
        }
        else if (c == '\\') {
            escaped = 1;
            continue;
        }

        escaped = 0;
    }
}
