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
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "shell.h"
#include "vfs.h"
#include "vfs_directory.h"
#include "vfs_node.h"
#include "fat12.h"

#define SHELL_BUFFER_LEN 1024


// Shell Commands

struct shell_command;
struct shell_command {
    const char *name;
    void(*impl)(int argc, const char *argv[]);
    struct shell_command *next;
};
typedef struct shell_command *shell_command_t;

shell_command_t shell_command_init(const char *name,
                                   void(*impl)(int, const char *[]))
{
    shell_command_t cmd = calloc(1, sizeof(*cmd));
    cmd->name = calloc(strlen(name), sizeof(*cmd->name));
    strncpy((void *)cmd->name, name, strlen(name));
    cmd->impl = impl;
    return cmd;
}


// Shell Globals

struct shell_globals {
    shell_command_t first_command;
    vfs_t fs;
    uint32_t buffer_max;
    uint8_t *import_buffer;
    uint32_t import_size;
    uint8_t running:1;
    uint8_t reserved:7;
};
static struct shell_globals global;

void shell_add_command(const char *name, void(*impl)(int, const char *[]))
{
    shell_command_t cmd = shell_command_init(name, impl);
    shell_command_t current = global.first_command;
    cmd->next = current;
    global.first_command = cmd;
}



// User Prompts and Input

void shell_user_prompt(const char *prompt, char *input)
{
    printf("%s", prompt);
    fgets(input, global.buffer_max, stdin);
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') {
        input[len - 1] = '\0';
    }
}


// Shell Command Implementations

void shell_run(const char *name, int argc, const char *argv[])
{
    shell_command_t cmd = global.first_command;
    while (cmd) {
        if (strcmp(cmd->name, name) == 0) {
            cmd->impl(argc, argv);
            return;
        }
        cmd = cmd->next;
    }
    
    fprintf(stderr, "Unrecognised command: %s\n", name);
}

void shell_exit(int argc, const char *argv[])
{
    global.running = 0;
}

void shell_echo(int argc, const char *argv[])
{
    for (int i = 0; i < argc; ++i) {
        printf("%s\n", argv[i]);
    }
}

void shell_format(int argc, const char *argv[])
{
    // the format of the command is: <fs>
    if (argc != 1) {
        fprintf(stderr, "Expected a single argument for the file system.\n");
        return;
    }
    
    // load in the new file system.
    char *label = "";
    void *old_fs = global.fs->filesystem_interface;
    global.fs->filesystem_interface = NULL;
    
    if (strcmp(argv[0], "fat12") == 0) {
        label = "UNTITLED   ";
        global.fs->filesystem_interface = fat12_init();
    }
    else {
        fprintf(stderr, "Unrecognised file system: %s\n", argv[0]);
        global.fs->filesystem_interface = old_fs;
    }
    
    // perform the formatting
    if (vfs_format_device(global.fs, label)) {
        printf("Disk image was successfully formatted\n");
    }
    
    // clean up
    vfs_interface_destroy(old_fs);
}

void shell_mount(int argc, const char *argv[])
{
    vfs_mount(global.fs);
}

void shell_unmount(int argc, const char *argv[])
{
    vfs_unmount(global.fs);
}

void shell_touch(int argc, const char *argv[])
{
    if (argc != 1) {
        fprintf(stderr, "Expected a single argument for the file name.\n");
        return;
    }

    vfs_touch(global.fs, argv[0]);
}

void shell_import(int argc, const char *argv[])
{
    if (argc != 1) {
        fprintf(stderr, "Expected a single argument for the path to import.\n");
        return;
    }

    if (global.import_buffer) {
        free(global.import_buffer);
        global.import_buffer = NULL;
    }

    // Open the file, get the size and read the data
    FILE *f = fopen(argv[0], "r");
    fseek(f, 0L, SEEK_END);
    global.import_size = (uint32_t)ftell(f);
    fseek(f, 0L, SEEK_SET);

    global.import_buffer = calloc(global.import_size, sizeof(uint8_t));
    fread(global.import_buffer, sizeof(uint8_t), global.import_size, f);

    printf("Imported %d bytes to internal buffer\n", global.import_size);
}

void shell_write(int argc, const char *argv[])
{
    if (argc != 1) {
        fprintf(stderr, "Expected a single argument for the file name.\n");
        return;
    }

    vfs_write(global.fs, argv[0], global.import_buffer, global.import_size);
}

void shell_ls(int argc, const char *argv[])
{
    vfs_directory_t dir = vfs_list_directory(global.fs);
    if (!dir) {
        fprintf(stderr, "Unable to list directory\n");
        return;
    }

    vfs_node_t node = dir->first;
    while (node && node->state == vfs_node_used) {
        // Get some meta data to help with the display.
        printf("%c", node->is_directory ? 'D' : '-');
        printf("%c", node->is_hidden ? 'H' : '-');
        printf("%c", node->is_readonly ? 'R' : '-');
        printf("%c", node->is_system ? 'S' : '-');
        printf(" %08dB ", node->size);
        printf("%s\n", node->name);
        node = node->next;
    }
}

void shell_splice(int argc, const char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: sector-splice <sector> <start> <size>\n");
        return;
    }

    uint32_t sector = atoi(argv[0]);
    uint32_t start = atoi(argv[1]);
    uint32_t size = atoi(argv[2]);

    // Check to ensure the import buffer is a sector in size.
    if (global.fs->device->sector_size != global.import_size) {
        fprintf(stderr, "Import buffer is incorrect size.\n");
        return;
    }

    // Read the sector from the device
    uint8_t *sector_data = device_read_sector(global.fs->device, sector);

    // Splice...
    memcpy(sector_data + start, global.import_buffer, size);

    // Write out to the device
    device_write_sector(global.fs->device, sector, sector_data);

    // Clean up
    free(sector_data);
}


// The Shell

void shell_parse_input(const char *input)
{
    if (strlen(input) == 0) {
        return;
    }

    char *cmd = NULL;
    char **argv = malloc(10 * sizeof(void *));
    memset(argv, 0, 10 * sizeof(void *));
    int argc = 0;
    
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
            if (!cmd) {
                cmd = token;
            }
            else {
                argv[argc++] = token;
                argv = realloc(argv, sizeof(*argv) * argc);
            }
            
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
    
    // Ready to execute the command
    shell_run(cmd, argc, (const char **)argv);
}

void shell_init(vfs_t vfs)
{
    // Configure the shell
    global.buffer_max = SHELL_BUFFER_LEN;
    global.running = 1;
    global.fs = vfs;
    
    // Setup shell commands
    shell_add_command("exit", shell_exit);
    shell_add_command("echo", shell_echo);
    shell_add_command("format", shell_format);
    shell_add_command("mount", shell_mount);
    shell_add_command("unmount", shell_unmount);
    shell_add_command("touch", shell_touch);
    shell_add_command("import", shell_import);
    shell_add_command("write", shell_write);
    shell_add_command("ls", shell_ls);
    shell_add_command("sector-splice", shell_splice);
    
    // Start the shell run loop
    char *buffer = malloc(global.buffer_max * sizeof(*buffer));
    while (global.running) {
        printf("\n");
        
        memset(buffer, 0, global.buffer_max * sizeof(buffer));
        
        char *prompt = calloc(255, sizeof(*prompt));
        strcpy(prompt, vfs_pwd(vfs));
        strcpy(prompt + strlen(prompt), " # ");
        
        shell_user_prompt(prompt, buffer);
        shell_parse_input(buffer);

        free(prompt);
    }
}
