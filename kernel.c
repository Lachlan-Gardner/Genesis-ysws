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
const uint8_t ps2_data_port = 0x60;
const uint8_t ps2_status_and_command_port = 0x64;
const uint8_t ps2_output_ready_bit = 0x01;

// The location of the vga buffer in memory.
// So it can be written to in order to draw things on the screen.
// Is its position fixed?
volatile uint16_t *vga_buffer = (uint16_t*) 0xB8000;

// The vga has 80x25 characters by default.
// Can it be increased?
// And is it just characters I can use? No pixels?
const int vga_columns = 80;
const int vga_rows = 25;

// Where the text initial starts getting displayed.
int term_col = 0;
int term_row = 0;
// Makes the colour black background, white text.
// The vga has sixteen colours, and we're setting the foreground and background using this.
// 0 for black, F for white.
uint8_t term_colour = 0x0D;
    
const uint8_t term_prompt_colour = 0x0C;

const uint16_t prompt_character = ((uint16_t)term_prompt_colour << 8) | ':';

// Location of a string in memory.
typedef struct 
{
    size_t start_index;
    size_t end_index;
} typed_string;

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

/// @brief Updates the cursor location.
void update_cursor()
{
    // No idea why it has to be vga_rows * 3 + t, it just works for whatever reason.
    // I think it's something to do with cursor locations and vga size mismatch.
    uint16_t position = term_row * (vga_rows * 3 + 5) + term_col;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (position & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((position >> 8) & 0xFF));
}

/// @brief Enables the cursor so people can see where they're typing.
void enable_cursor()
{
    // How many lines in the cursor.
    const int cursor_start = 0;
    const int cursor_end = 15;
    
    // Copied pretty much verbatim from https://wiki.osdev.org/Text_Mode_Cursor
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

/// @brief Disables the cursor
void disable_cursor()
{
    // Copied from the osdev wiki.
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

/// @brief Initialises the terminal by writing every character as a space.
void term_init()
{
    // Iterates through every character in the vga buffer.
    for (int col = 0; col < vga_columns; col++)
    {
        for (int row = 0; row < vga_rows; row++)
        {
            // size_t because it is the largest size the system can hold.
            // Something to do with 64 to 32 bit conversion?
            // The vga buffer has an index the size of (vga_columns * vga_rows), so we're finding the index for the character that is being written.
            const size_t index = (vga_columns * row) + col;
            
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
    // If it's a newline, set the cursor to 0 and go down a row.
    if (character == '\n')
    {
        term_col = 0;
        term_row++;
        
        // Updates the cursor
        update_cursor();
        
        return;
    }
    
    // Calculate where in the buffer to put the character.
    // Same as the clear function, except the position is from where the cursour currently is.
    const size_t index = (vga_columns * term_row) + term_col;
    // Same as clearing the screen, except we're now writing this new character.
    // It also should be white if it isn't a space.
    vga_buffer[index] = ((uint16_t)term_colour << 8) | character;
    // Move the cursor along.
    term_col++;

    // So the text isn't being written outside the bounds.
    if (term_col >= vga_columns) 
    {
        // Set it back up to the start of the row.
        term_col = 0;

        // Move down one row.
        term_row++;
    }
    
    // If the printing has reached the bottom of the vga buffer.
    if (term_row >= vga_rows) 
    {
        // Resets back up to the top left corner once the bottom of the screen is reached.
        term_col = 0;
        term_row = 0;
        
        // Clears the screen, though it doesn't save any text.
        // To implement saving, you'd have to save all output to one long string, then loop through it to print it back out on scroll.
        term_init();
    }

    update_cursor();
}

/// @brief Print a string to the terminal.
/// @param string The string to print.
void print_to_term(const char *string) 
{
    // '/0' is the null character, which is what you would get at the end of the string.
    for (size_t i = 0; string[i] != '\0'; i++) 
    {
        // Write that terminal to the buffer.
        // When does the buffer actually get written to the screen?
        term_put_character(string[i]);
    }
}

/// @brief Checks whether the keyboard has a buffer to be read for scancodes.
/// @return 1 if it is ready, 0 if it isn't.
int keyboard_buffer_ready()
{
    // Bitwise operations make me feel so smart.
    return inb(ps2_status_and_command_port) & ps2_output_ready_bit;
}

uint8_t keyboard_get_scancode() {
    while (!keyboard_buffer_ready()) {
        // Just wait until there is data to read.
        // This feels so wrong, but I can add interrupts later if there's time.
    }
    return inb(ps2_data_port);
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

/// @brief Finds the length of a string.
/// @param string 
/// @return The length of the string, which is the index that the NULL character is on.
int string_length(const char *string) 
{
    int index;
    // Increments index until it reaches the end of the string.
    for (index = 0; string[index] != NULL; index++);
    return index;
}

/// @brief Removes the last character.
void backspace()
{ 
    // Calculate where in the buffer to put the empty space character.
    // It's just one less than the current location.
    const size_t index = (vga_columns * term_row) + term_col - 2;
    
    const uint32_t character_next = vga_buffer[index];
    
    if (character_next != prompt_character)
    {
        // Move the cursor along.
        term_col--;
        
        size_t index = (vga_columns * term_row) + term_col;
        
        // Same as clearing the screen, except we're now writing this new character.
        // It also should be white if it isn't a space.
        vga_buffer[index] = ((uint16_t)term_colour << 8) | ' ';
        
        update_cursor();
    }
}

/// @brief Get a string input from the keyboard.
/// @return Where in the vga buffer the string is.
typed_string get_input()
{
    int enter_pressed = 0;
    // The vga index before typing
    typed_string string = {.start_index = (vga_columns * term_row) + term_col};
    
    // Probably bad practice, but it'll survive. My teacher would kill me if he ever saw this though.
    // The loop repeats until enter is pressed, which will break the loop.
    while (1)
    {
        const uint8_t scancode = keyboard_get_scancode();
        
        // If enter is pressed.
        if (scancode == 0x1C) {
            // Break the loop.
            break;
        }
        
        if (scancode== 0x0E)
        {
            backspace();
          // If the scancode isn't released or an irrelevant/other key.
        } else if (scancode < 0x80 && scancode < 58) 
        {
            char typed_character = scancode_conversion(scancode);
    
            // Write to the buffer.
            term_put_character(typed_character);
        }
    }
    
    // Finds the current index after typing.
    string.end_index = (vga_columns * term_row) + term_col;
    
    return string;
}

/// @brief Checks whether the vga string matches the target. This cannot be the best way to do this, but idk how else to pass around strings without memory allocation, and i do not want to do that freestanding.
/// @param string The struct containing the vga info.
/// @param target The target that it has to match.
/// @return true if it's a match, 0 for a mismatch.
int check_string(typed_string string, const char *target)
{
    // In case enter was just pressed and there is nothing to interate through.
    if (string.start_index == string.end_index){
        return 0;
    }
    
    for (int i = 0; string.start_index + i < string.end_index; i++)
    {
        // Clears the last 8 bits in the vga buffer index.
        char char_in_vga = (vga_buffer[string.start_index +i] << 8) >> 8;
        
        // Checks whether it's a match to that part of the target.
        if (char_in_vga != target[i])
        {
            return 0;
        }
    }
    
    return 1;
}

/// @brief Writes the terminal prompt to the terminal.
void terminal_prompt()
{
    //TODO: Change this to use different colour.
    
    const char *prompt = "Shell";

    // '/0' is the null character, which is what you would get at the end of the string.
    for (size_t i = 0; prompt[i] != '\0'; i++) 
    {
        // Write that terminal to the buffer.
        // When does the buffer actually get written to the screen?
        term_put_character(prompt[i]);
    }
    
    const size_t index = (vga_columns * term_row) + term_col;
    
    // Writes it to the terminal with a different colour so that it knows not to backspace it.
    vga_buffer[index] = prompt_character;
    term_col++;
    
    term_put_character(' ');
    
    update_cursor();
}

/// @brief Prints the help menu.
void print_help_menu()
{
    print_to_term("\nexit - Exits the shell");
}

void print_about()
{
    print_to_term("\nThis is an OS I made for the genesis ysws challenge(? event? what do you call it?).\nI had no prior experience with any low level stuff, so I learnt a lot doing this project (it should also explain if anything looks weird or is done strangely)");
}

/// @brief Makes a new line on the terminal with a prompt.
void new_terminal_line()
{
    term_put_character('\n');
    terminal_prompt();
}

/// @brief Basic shell. I won't lie, it's not the best, but it works-ish.
void basic_shell()
{
    // Have while loop. Get input and check to see if it matches a command. Execute command.
    int in_shell = 1;
    
    // So the first line isn't on the first bit.
    terminal_prompt();
    
    while (in_shell)
    {        
        typed_string user_input = get_input();
        
        if (check_string(user_input, "help"))
        {
            print_help_menu();
        } else if (check_string(user_input, "exit"))
        {
            // Exits the shell.
            in_shell = 0;
            
            return;
        } else if (check_string(user_input, "about"))
        {
            print_about();
        } else
        {
            print_to_term("\nThat isn't a valid option. Enter \"help\" to see the options.");
        }
        
        new_terminal_line();
    }
    
}

int kernel_main() 
{
    // Clears the screen to get it ready.
    term_init();
    
    enable_cursor();
    
    update_cursor();
    
    basic_shell();
    
    // Clears the screen and gets rid of the cursor.
    // Exiting the program doesn't seem to work rn, so this is the best we get.
    term_init();
    disable_cursor();
    
    return 0;
}