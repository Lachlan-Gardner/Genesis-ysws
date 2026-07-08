# Genesis-OS

Welcome to Genesis-OS an OS I made for the genesis ysws event (challenge? Puzzle? Not sure what you're supposed to call it). It's an x86 operating system using C and a little assembly (most of which has been completely unchanged from being copy pasted from a tutorial), C is definitely the main attraction here since I'm at least comfortable with the basics there. 

It can currently write out text and receive keyboard input, so very much a basic "OS", but I'm actually quite proud of it considering I'd never done anything so close to the hardware (I'm absolutely been sold on bitwise operations, they're just so cool).

If, for whatever reason, you want to fork or use code from this, feel free and good luck.

### Features:
 - Terminal like interface.
 - Pink text.
 - A bootable iso (I have no idea if it works on real hardware, but it boots. It does work properly on virtualbox).
 - Can recieve some small commands (via one of the hackiest string operations you've ever seen).

Unfortunately there were a couple things I planned to implement and didn't, like actually useful/interesting commands, but I'm still pretty happy with how it turned out.

### Acknowledgement/sources:
 - [osdev wiki](https://wiki.osdev.org/Expanded_Main_Page), especially the bare bones tutorial and the more esoteric stuff like the cursor functions.
 - https://wiki.osdev.org/Bare_Bones Provided a really good base for stuff VGA-wise.
 - [MiyarOS](https://github.com/Qazi-01/Miyar-OS/tree/main). I got the inb and outb functions here which helped so much.
 - https://computers-art.medium.com/writing-a-basic-kernel-6479a495b713 The assembly and linker script are from here. I've never touched either assembly or linker scripts before, so try not to judge too harshly that I copied and pasted.

### Thanks
Thank you to The Great Guac for organising this whole thing and giving me a reason to do this, it's been really cool learning the low level side (I have so much respect for standard libs now).