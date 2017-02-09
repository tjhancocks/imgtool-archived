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

#include <shell/variable.h>
#include <shell/shell.h>

shell_variable_t shell_variable_init(const char *symbol, const char *value)
{
    assert(symbol);
    
    shell_variable_t var = calloc(1, sizeof(*var));
    
    uint32_t symbol_size = (uint32_t)strlen(symbol) + 1;
    var->symbol = calloc(symbol_size, sizeof(*var->symbol));
    strlcpy((void *)var->symbol, symbol, symbol_size);
    
    shell_variable_set(var, value);
    
    return var;
}

void shell_variable_unlink(shell_variable_t var)
{
    assert(var);
    
    shell_variable_t prev = var->prev;
    shell_variable_t next = var->next;
    
    if (prev) {
        prev->next = next;
    }
    
    if (next) {
        next->prev = prev;
    }
    
    var->next = NULL;
    var->prev = NULL;
}

void shell_variable_destory(shell_variable_t var)
{
    if (var) {
        shell_variable_unlink(var);
        free((void *)var->symbol);
        free((void *)var->value);
    }
    free(var);
}


void shell_variable_set(shell_variable_t var, const char *value)
{
    assert(var);
    
    free((void *)var->value);
    
    if (value) {
        uint32_t value_size = (uint32_t)strlen(value) + 1;
        var->value = calloc(value_size, sizeof(*var->symbol));
        strlcpy((void *)var->value, value, value_size);
    }
    else {
        var->value = NULL;
    }
}

void shell_variable_get(shell_variable_t var, char **value)
{
    assert(var);
    assert(value);
    
    if (!var->value) {
        *value = NULL;
        return;
    }
    
    uint32_t value_size = (uint32_t)strlen(var->value) + 1;
    
    // Need to allocate something for the value and copy it out.
    *value = calloc(value_size, sizeof(**value));
    strlcpy(*value, var->value, value_size);
}
