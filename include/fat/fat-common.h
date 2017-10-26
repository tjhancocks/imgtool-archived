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

#include <stdint.h>

#ifndef FAT_COMMON
#define FAT_COMMON

struct fat_sfn {
	uint8_t name[11];
	uint8_t attribute;
	uint8_t nt_reserved;
	uint8_t ctime_ms;
	uint16_t ctime;
	uint16_t cdate;
	uint16_t adate;
	uint16_t unused;
	uint16_t mtime;
	uint16_t mdate;
	uint16_t first_cluster;
	uint32_t size;
} __attribute__((packed));
typedef struct fat_sfn * fat_sfn_t;

struct fat_directory_buffer {
	struct fat_sfn sfn;
	uint32_t index;
	struct vfs_node *first_child;
	struct vfs_node *last_child;
};

#endif