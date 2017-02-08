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

#include <shell/set.h>
#include <shell/variable.h>
#include <shell/shell.h>

void shell_set(struct shell *shell, int argc, const char *argv[])
{
    assert(shell);
    
    if (argc != 3) {
        fprintf(stderr, "Usage: set <variable> <value>\n");
        return;
    }
    
    // Is there an existing variable?
    shell_variable_t var = shell_find_variable(shell, argv[1]);
    if (!var) {
        // Create a variable
        var = shell_variable_init(argv[1], NULL);
        shell_add_variable(shell, var);
    }
    
    // Set the value of the variable
    shell_variable_set(var, argv[2]);
}

void shell_setu(struct shell *shell, int argc, const char *argv[])
{
    assert(shell);
    
    if (argc != 3) {
        fprintf(stderr, "Usage: set <variable> <value>\n");
        return;
    }
    
    // Is there an existing variable? If so then abort.
    shell_variable_t var = shell_find_variable(shell, argv[1]);
    if (var) {
        return;
    }
    
    // Create a variable
    var = shell_variable_init(argv[1], argv[2]);
    shell_add_variable(shell, var);
}
