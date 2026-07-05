// GCC provides these, so they can still be used and keep the program freestanding.
#include <stddef.h>
#include <stdint.h>

// Checks whether the code is being compiled correctly.
// elif can be used because the compilation will stop anyway if the first is true.
#if defined (_linux_)
    #error "This code must be compiled using a cross compiler"
// #elif !defined(_i686_)
//     #error "This code must be compiled with an x86-elf compiler"
#endif

/// Defines some common things.
#define PS2_DATA_PORT 0x60
#define PS2_STATUS_AND_COMMAND_PORT 0x64
#define PS2_OUTPUT_READY_BIT 0x01

// The location of the vga buffer in memory.
// So it can be written to in order to draw things on the screen.
// Is its position fixed?
volatile uint16_t *vga_buffer = (uint16_t*) 0xB8000;

// The vga has 80x25 characters by default.
// Can it be increased?
// And is it just characters I can use? No pixels?
const int VGA_COLS = 80;
const int VGA_ROWS = 25;

// Where the text initial starts getting displayed.
int term_col = 0;
int term_row = 0;
// Makes the colour black background, white text.
// The vga has sixteen colours, and we're setting the foreground and background using this.
// 0 for black, F for white.
uint8_t term_colour = 0x0D;

/// @brief Initialises the terminal by writing every character as a space.
void term_init(void)
{
    // Iterates through every character in the vga buffer.
    for (int col = 0; col < VGA_COLS; col++)
    {
        for (int row = 0; row < VGA_ROWS; row++)
        {
            // size_t because it is the largest size the system can hold.
            // Something to do with 64 to 32 bit conversion?
            // The vga buffer has an index the size of (VGA_COLS * VGA_ROWS), so we're finding the index for the character that is being written.
            const size_t index = (VGA_COLS * row) + col;
            
            // Writes a black background over the character of that index in the vga buffer.
            // Entries in the buffer are in binary looking like BBBBFFFFCCCCCCCC, where B is the background colour, F is foreground, and C is the character.
            // The first part converts the uint8 to 16 bit and moves it 8 bits to the left (background and foreground colours), then add the binary of the character onto the end using or.
            vga_buffer[index] = ((uint16_t)term_colour << 8) | ' ';
        }
    }
}

/// @brief Puts a character on the screen.
/// @param character The character to add. Is it bad practice to call a char character?
void term_put_character(char character) 
{
    switch(character) 
    {
        // If enter is pressed, set the cursor to 0 and go down a row.
        case '\n':
        {
            term_col = 0;
            term_row++;
            break;
        }
        // Anything else gets appended to the column.
        default:
        {
            // Calculate where in the buffer to put the character.
            // Same as the clear function, except the position is from where the cursour currently is.
            const size_t index = (VGA_COLS * term_row) + term_col;
            // Same as clearing the screen, except we're now writing this new character.
            // It also should be white if it isn't a space.
            vga_buffer[index] = ((uint16_t)term_colour << 8) | character;
            // Move the cursor along.
            term_col++;
            
            break;
        }
        
        // So the text isn't being written outside the bounds.
        if (term_col >= VGA_COLS) 
        {
            // Set it back up to the start of the row.
            term_col = 0;
            
            // Move down one row.
            term_row++;
        }
        
        // If the printing has reached the bottom of the vga buffer.
        if (term_row >= VGA_ROWS) 
        {
            // Resets back up to the top left corner once the bottom of the screen is reached.
            term_col = 0;
            term_row = 0;
            
            // Clears the screen, though it doesn't save any text.
            // To implement saving, you'd have to save all output to one long string, then loop through it to print it back out on scroll.
            term_init();
        }
    }
}

/// @brief Print a string to the terminal.
/// @param str The string to print.
void print_to_term(const char *str) 
{
    // '/0' is the null character, which is what you would get at the end of the string.
    for (size_t i = 0; str[i] != NULL; i++) 
    {
        // Write that terminal to the buffer.
        // When does the buffer actually get written to the screen?
        term_put_character(str[i]);
    }
}

/// @brief Reads a byte from the port.
/// @param port The port to be read from.
/// @return The byte.
static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    // A bit of assembly.
    // I did not write this, it's from MiyarOS, so thank you to the author of that.
    asm volatile ("inb %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

/// @brief Writes a byte to the specified port.
/// @param port The port to write to.
/// @param data The data that is being written.
static inline void outb(uint16_t port, uint8_t data)
{
    // I did not write this, it's from MiyarOS, so thank you to the author of that.
    asm volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}

/// @brief Checks whether the keyboard has a buffer to be read for scancodes.
/// @return 1 if it is ready, 0 if it isn't.
int keyboard_buffer_ready(void)
{
    // Bitwise operations make me feel so smart.
    return inb(PS2_STATUS_AND_COMMAND_PORT) & PS2_OUTPUT_READY_BIT;
}

uint8_t keyboard_get_scancode(void) {
    while (!keyboard_buffer_ready()) {
        // Just wait until there is data to read.
        // This feels so wrong, but I can add interrupts later if there's time.
    }
    return inb(PS2_DATA_PORT);
}

/// @brief Converts the scancode to char.
/// @param scancode The scancode.
/// @return The converted char. Returns 0 if it isn't a valid character.
char scancode_conversion(uint8_t scancode)
{    
    /// 0s represent keys that don't map to characters, like escape or error code.
    static char scancode_table[58] = {0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '};
    
    return scancode_table[scancode];
}

void kernel_main() 
{
    // Clears the screen to get it ready.
    term_init();
    
    // Printing out some stuff.
    print_to_term("It works!\nEven on a new line!");
    
    while (1)
    {
        uint8_t scancode = keyboard_get_scancode();
        
        char something;
        
        if (!(scancode & 0x80) && scancode <= 58)
        {
            something = scancode_conversion(scancode);
        }
        term_put_character(something);
    }
}

//TODO: Add variable colours.
//TODO: Keyboard input.
//TODO: Basic curses like interface?