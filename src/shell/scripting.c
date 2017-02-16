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

#include <shell/shell.h>
#include <shell/scripting.h>
#include <shell/parser.h>
#include <shell/variable.h>


#pragma mark - Shell Commands

shell_command_t shell_command_create(const char *name, shell_command_imp_t imp)
{
    shell_command_t cmd = calloc(1, sizeof(*cmd));
    cmd->name = name;
    cmd->impl = imp;
    return cmd;
}

void shell_command_destroy(shell_command_t cmd)
{
    if (cmd) {
        shell_command_destroy(cmd->next);
    }
    free(cmd);
}


shell_command_t shell_command_for(shell_t shell, const char *name)
{
    assert(shell);
    shell_command_t cmd = shell->first_command;
    while (cmd) {
        if (strcmp(cmd->name, name) == 0) {
            return cmd;
        }
        cmd = cmd->next;
    }
    return NULL;
}


#pragma mark - Shell Statements

shell_statement_t shell_statement_create(const char *raw_statement)
{
    assert(raw_statement);
    
    // Create a new statement from the raw statement provided.
    shell_statement_t stmt = calloc(1, sizeof(*stmt));
    shell_parse(raw_statement, &stmt->argc, (char ***)&stmt->argv);
    
    // If the statement contained no arguments then it is most likely either
    // empty or a comment. Tear down the statement structure and return NULL.
    if (!stmt->argv) {
        free(stmt);
        stmt = NULL;
    }
    
    return stmt;
}

void shell_statement_destroy(shell_statement_t stmt)
{
    if (stmt) {
        free(stmt->argv);
    }
    free(stmt);
}


#pragma mark - Shell Script

shell_script_t shell_script_open(const char *path)
{
    if (!path) {
        return NULL;
    }
    
    // Open the script and convert each line to a statement.
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Shell script \"%s\" did not exist.\n", path);
        return NULL;
    }
    
    shell_script_t script = calloc(1, sizeof(*script));
    
    char raw_statement[1024] = {0};
    while (fgets(raw_statement, 1024, f)) {
        
        // Check for a terminating \n and strip it from the raw_statement
        uint32_t len = (uint32_t)strlen(raw_statement);
        if (raw_statement[len - 1] == '\n') {
            raw_statement[len - 1] = '\0';
        }
        
        // Construct an actual statement. Some statements may be produced
        // as NULL however, so check to ensure it is a valid statement before
        // adding it!
        shell_statement_t stmt = shell_statement_create(raw_statement);
        if (!stmt) {
            continue;
        }
        
        if (script->last) {
            script->last->next = stmt;
        }
        script->last = stmt;
        
        if (!script->first) {
            script->first = stmt;
        }
        
    }
    
    return script;
}

void shell_script_destroy(shell_script_t script)
{
    if (script) {
        shell_statement_t stmt = script->first;
        while (stmt) {
            shell_statement_t tmp = stmt->next;
            shell_statement_destroy(stmt);
            stmt = tmp;
        }
    }
    free(script);
}


#pragma mark - Shell Script Execution

int shell_script_execute(shell_t shell, shell_script_t script)
{
    assert(shell);
    assert(script);
    
    shell_statement_t stmt = script->first;
    while (stmt) {
        shell_statement_resolve(shell, stmt);
        int err = shell_statement_execute(shell, stmt);
        if (err != SHELL_OK) {
            return err;
        }
        stmt = stmt->next;
    }

    return SHELL_OK;
}

void shell_statement_resolve(shell_t shell, shell_statement_t stmt)
{
    assert(shell);
    assert(stmt);
    
    // To resolve a statement we need to check each argument to see if
    // it contains a variable. If it does then it needs to be substituted for
    // its actual value.
    
    for (int i = 0; i < stmt->argc; ++i) {
        
        // If the argument is a variable then swap out the entire argument.
        // If the variable is not found then leave the symbol in place.
        if (stmt->argv[i][0] == '$') {
            const char *symbol = stmt->argv[i];
            shell_variable_t var = shell_find_variable(shell, symbol + 1);
            
            if (!var) {
                continue;
            }
            
            shell_variable_get(var, (char **)&stmt->argv[i]);
            free((void *)symbol);
        }
    }
}

int shell_statement_execute(shell_t shell, shell_statement_t stmt)
{
    assert(shell);
    assert(stmt);
    
    // We're going to execute the statement. The first argument is the command.
    if (stmt->argc == 0) {
        fprintf(stderr, "Malformed statement. Skipping.\n");
        return SHELL_TERMINATE;
    }
    
    // Search for the command and then execute it.
    shell_command_t cmd = shell_command_for(shell, stmt->argv[0]);
    if (!cmd) {
        fprintf(stderr, "Unrecognised command: %s\n", stmt->argv[0]);
        return SHELL_ERROR_CODE;
    }
    return cmd->impl(shell, stmt->argc, stmt->argv);
}
