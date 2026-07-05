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

// The location of the vga buffer in memory.
// So it can be written to in order to draw things on the screen.
// Is its position fixed?
volatile uint16_t *vga_buffer = (uint16_t*) 0xB8000;

// The ports of the ps/2 controller for the keyboard.
/// The prt for the statux and c    ommand registers.
const uint8_t keyboard_status_command_register = (uint8_t) 0x60;
/// The data port for the keyboard dara port.
const uint8_t keyboard_data = (uint8_t) 0x64;

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
void term_init()
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

/// @brief Reads a byte of data from a port.
/// @param port Which port to read from.
/// @return The byte.
uint8_t read_port(uint16_t port) 
{
    uint8_t data;
    __asm__ volatile ("inb %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

/// @brief Write a byte to a port.
/// @param port The port to write to.
/// @param data_to_write what to write.
void write_port(uint16_t port, uint8_t data_to_write)
{
    __asm__ volatile ("outb %0, %1" : : "a"(data_to_write), "Nd"(port));
}


char get_input()
{
    int pressed_enter = 0;
    
    print_to_term("\nStarting get input.     ");
    
    while (1) 
    {
        if (read_port(keyboard_status_command_register) & 1) 
        {
            if (read_port(keyboard_data) == 0x02)
            {
                return '1';
            }
        }
    }
}

int ps_port_init()
{
    // Disable the first port.
    write_port(keyboard_status_command_register, 0xAD);
    // Disable the sceond port.
    write_port(keyboard_status_command_register, 0xAD);
    
    read_port(keyboard_data);
    
    // Flushing the bits of the ps/2 controller.
    
    /// Disable IRQ.
    uint8_t flush_bit_0 = 1;
    /// Disables translation of port 1.
    uint8_t flush_bit_6 = 1 << 6;
    /// Enables clock signal.
    uint8_t flush_bit_4 = 1 << 4;
    
    // I feel so smart rn.
    uint8_t total_flush = ~(0 | flush_bit_0 | flush_bit_6 | flush_bit_4);
    
    uint8_t flushed_port = read_port(keyboard_status_command_register) & total_flush;
    
    write_port(keyboard_status_command_register, flushed_port);
    
    write_port(keyboard_status_command_register, 0xAA);
    
    while (!(read_port(keyboard_status_command_register) & 1)) 
    {
        // Waits until results of the are ready.
        //! If there's an infite loop it's probably this.
    }
    
    const uint8_t result_from_self_test = read_port(keyboard_status_command_register);
    
    // Test failed.
    if (result_from_self_test != 0x55)
    {
        // "And now we know."
        print_to_term("\nPs/2 Controller failed self test.");
        
        // Reflects the failure.
        return 0;
    }
    
    // Tries to enable the second port.
    write_port(keyboard_status_command_register, 0xA8);
    
    write_port(keyboard_status_command_register, 0x20);
    
    while (!(read_port(keyboard_status_command_register) & 1)) 
    {
        // Waits until results of the are ready.
        //! If there's an infite loop it's probably this.
    }
    
    const uint8_t result_from_configuration_test = read_port(keyboard_status_command_register);
    
    if (result_from_configuration_test & (1 << 4))
    {
        print_to_term("\nOnly one port ps/2 controller.");
    } else {
        // Disable tbe second port.
        write_port(keyboard_status_command_register, 0xA7);
    } 
}

void kernel_main() 
{
    // Clears the screen to get it ready.
    term_init();
    
    // Printing out some stuff.
    print_to_term("It works!\nEven on a new line!");
    
    char something = get_input();
    
    print_to_term("Finished getting input: ");
    
    term_put_character(something);
}

//TODO: Add variable colours.
//TODO: Keyboard input.
//TODO: Basic curses like interface?