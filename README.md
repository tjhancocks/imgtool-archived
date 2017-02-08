#imgtool

![Basic VFS Support](https://img.shields.io/badge/VFS-Basic-green.svg)
![Partial FAT12 Support](https://img.shields.io/badge/FAT12-Partial-yellow.svg)
![No FAT16 Support](https://img.shields.io/badge/FAT16-None-red.svg)
![No FAT32 Support](https://img.shields.io/badge/FAT32-None-red.svg)
![No VFAT Support](https://img.shields.io/badge/VFAT-None-red.svg)
![No ExFAT Support](https://img.shields.io/badge/ExFAT-None-red.svg)
![No EXT2 Support](https://img.shields.io/badge/ext2-None-red.svg)

A simple tool for working with disk images and performing changes to them in a sandboxed environment.

###Purpose
The original purpose of this tool is for my own OS Development work. Assembling
disk images during a build of the OS/Kernel can be a tedious if not painful task sometimes. Setting up a file system, device loops, GRUB installation, etc. It can get a bit tedious and isn't guarnteed to work between different host environments. For example, GRUB is unsupported on macOS even though a few tools exist for it in the likes of homebrew.

Further to that on macOS if you so much as mount it in the system then you'll end up with a ton of junk being put in the filesystem. Not really ideal.

All of this is a bit of pain. I want something relatively simple to use to get the job done.

###"Virtual" Virtual File System
So how is this solved? The answer is a custom disk editing tool that runs its own virtual file system and concrete file system drivers. This keeps it entirely seperate from the host file system.

The added bonus here is that the concrete file system drivers will hopefully be useful inside the OS/Kernel this is being made to support.

###The Current State
This tool is far from done. A foundation has been laid, but the rest of the thing needs building. Below is a list of features that I have planned for this.

- [x] Basic shell for interacting with the disk image
- [x] Tool to format disk image to a specific file system
- [x] Virtual File System to abstract away from concrete drivers
- [x] Emulate a real device by operating on "physical sectors"
- [ ] Concrete FAT12 driver *(Partially Implemented)*
- [ ] Concrete FAT16 driver
- [ ] Concrete FAT32 driver
- [ ] Concrete EXT2 Driver
- [ ] `grub install` functionality for GRUB Legacy.

###License
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

