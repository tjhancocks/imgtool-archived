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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <shell/variable.h>
#include <shell/scripting.h>
#include <shell/shell.h>
#include <common/host.h>


#pragma mark - Environment Variables

void parse_environment_variable(const char *env, char **sym, char **val)
{
    assert(env);
    assert(sym);
    assert(val);

    // The symbol and value are deliniated by an `=` character. The exact format
    // should be something along the lines of: SYMBOL=value
    unsigned long start = 0;
    unsigned long total_length = strlen(env);
    char **ptr = sym;

    for (unsigned long i = 0; i <= total_length; ++i) {
        char c = env[i];

        // Is this a deliniater (`=` or 0x00)? If so we need to record the value
        // that we've just parsed into the appropriate location.
        if ((c == '=' || c == 0x00) && ptr) {
            // Extract the value and write it to ptr
            unsigned long len = i - start;
            *ptr = calloc(len + 1, sizeof(**ptr));
            memcpy(*ptr, env + start, len);
            start = i + 1;

            // Switch ptr to the value pointer, or NULLify it.
            ptr = (ptr == sym) ? val : NULL;
        }
    }
}

shell_variable_t construct_variables_from_environment(const char *env[])
{
    assert(env);

    // Iterate over the list of environment variables, getting their symbols
    // and values and construct a list of variables that can be given to the
    // shell when it launches.
    shell_variable_t environment_vars = NULL;

    for (const char **ptr = env; *ptr != NULL; ++ptr) {
        char *symbol = NULL;
        char *value = NULL;
        parse_environment_variable(*ptr, &symbol, &value);

        shell_variable_t var = shell_variable_init(symbol, value);
        var->next = environment_vars;
        if (environment_vars) {
            environment_vars->prev = var;
        }
        environment_vars = var;

        // Clean up the symbol and value
        free(symbol);
        free(value);
    }

    return environment_vars;
}


#pragma mark - Core

int main(int argc, const char * argv[], const char *env[])
{
    // Extract all environment variables and build up a list of shell variables
    // in preparation.
    shell_variable_t env_vars = construct_variables_from_environment(env);

    // Parse out the arguments passed to us explicitly by the user. These are
    // optional and may contain a script to run and an initial disk image to
    // work with.
    const char *script_path = NULL;
    const char *image_path = NULL;
    int c = 0;
    while ((c = getopt(argc, (char **)argv, "s:o:")) != -1) {
        switch (c) {
            case 's': // User specified script
                script_path = host_expand_path(optarg);
                break;

            case 'o': // User specified image
                image_path = host_expand_path(optarg);
                break;

            default:
                break;
        }
    }

    // If there is a shell script path specified, construct a new shell script
    // object.
    shell_script_t script = shell_script_open(script_path);

    // Construct a shell object and launch it.
    shell_t shell = shell_init(env_vars, script, image_path);
    shell_do(shell);

    // Clean up memory
    shell_script_destroy(script);
    free((void *)script_path);
    free((void *)image_path);

    return 0;
}
