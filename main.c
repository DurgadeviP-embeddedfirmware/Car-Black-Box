/* File:main.c
 * Author: DURGA
 * description:CAR BLACK BOX 
 * Created on 20 November, 2024, 8:00 PM
 */
#include <xc.h> 
#include "main.h"
#include "matrix_keypad.h"
#include "clcd.h"
#include "adc.h"
#include "eeprom.h"
#include "external_eeprom.h"
#include "i2c.h"
#include "ds1307.h"
#include "uart.h"

// Define constants for various operations
#define period 1023
unsigned char temp[3];  // Temp array for storing speed-related values
unsigned int duty_cycle, speed, gear = 0, sc = 0, db_flag = 0, events = 0, star_flag = 0, v_flag = 0, d_flag = 0, c = 0, shift = 0, e_flag = 0;
unsigned long int b;  // Used in log management and events counting
unsigned char key;  // Variable to store key press states
unsigned int count = 0, blink = 0, swap = 0, h = 0, m = 0, s = 0, t_flag = 1, down_flag = 0;
unsigned char previous_gear = 0xFF;  // To store the previous gear for comparison
char digit[9][3] = {"ON", "GN", "GR", "G1", "G2", "G3", "G4", "G5", "C "};  // Array for gear state descriptions
char scrolling[4][17] = {"VIEW LOG ", "SET TIME ", "DOWNLOAD LOG ", "CLEAR LOG "};  // Scrolling text for the LCD menu
unsigned char time[9], clock_reg[3];  // Time variables to store current time and clock registers
void scroll(void);  // Function prototype for scrolling text on LCD
unsigned char str[10][15];  // To store log data
char buffer[1][15];  // Temporary buffer for log handling
unsigned char *eeprom_index = 0x00, *a = 0x00;  // EEPROM index pointers for data storage/retrieval

// Function to store data in external EEPROM
void store_data_in_eeprom(unsigned char *time, unsigned char *gear_str, unsigned char *speed_str) {
    if (count++ > 10) {
        eeprom_index = 0x00;  // Reset EEPROM index after 10 entries
        count = 0;
    }
    e_flag = 0;  // Clear flag
    external_eeprom_write(eeprom_index++, time[0]);  // Write time to EEPROM
    external_eeprom_write(eeprom_index++, time[1]);
    external_eeprom_write(eeprom_index++, time[3]);
    external_eeprom_write(eeprom_index++, time[4]);
    external_eeprom_write(eeprom_index++, time[6]);
    external_eeprom_write(eeprom_index++, time[7]);
    external_eeprom_write(eeprom_index++, gear_str[0]);  // Write gear and speed to EEPROM
    external_eeprom_write(eeprom_index++, gear_str[1]);
    external_eeprom_write(eeprom_index++, speed_str[0]);
    external_eeprom_write(eeprom_index++, speed_str[1]);
}

// Function to get the current time from the DS1307 RTC (Real-Time Clock)
static void get_time(void) {
    clock_reg[0] = read_ds1307(HOUR_ADDR);  // Read hours
    clock_reg[1] = read_ds1307(MIN_ADDR);  // Read minutes
    clock_reg[2] = read_ds1307(SEC_ADDR);  // Read seconds

    if (clock_reg[0] & 0x40) {  // 12-hour format
        time[0] = '0' + ((clock_reg[0] >> 4) & 0x01);  // Extracting hour part
        time[1] = '0' + (clock_reg[0] & 0x0F);  // Extracting minute part
    } else {  // 24-hour format
        time[0] = '0' + ((clock_reg[0] >> 4) & 0x03);  // Extracting hour part
        time[1] = '0' + (clock_reg[0] & 0x0F);  // Extracting minute part
    }
    time[2] = ':';  // Add separator for time format
    time[3] = '0' + ((clock_reg[1] >> 4) & 0x0F);  // Extracting minutes
    time[4] = '0' + (clock_reg[1] & 0x0F);
    time[5] = ':';  // Separator
    time[6] = '0' + ((clock_reg[2] >> 4) & 0x0F);  // Extracting seconds
    time[7] = '0' + (clock_reg[2] & 0x0F);
    time[8] = '\0';  // End of string
}

// Function to display dashboard information (time, speed, and gear)
void dash_board(void) {
    clcd_print(" TIME EV SP ", LINE1(0));  // Print header on LCD
    clcd_print(time, LINE2(0));  // Display current time
    clcd_putch(' ', LINE2(8));  // Empty space for padding

    // Speed part: Read ADC and calculate speed
    duty_cycle = read_adc(CHANNEL4);  // Read from the ADC channel
    speed = duty_cycle / 10;  // Calculate speed
    if (speed > 99) speed = 99;  // Limit speed to 99
    temp[0] = ((speed / 10) + '0');  // Store tens digit of speed
    temp[1] = ((speed % 10) + '0');  // Store ones digit of speed
    temp[2] = '\0';  // Null terminate the string
    clcd_putch(temp[0], LINE2(13));  // Display speed on LCD
    clcd_putch(temp[1], LINE2(14));

    // Gear part: Display current gear
    if (gear == 0) clcd_print(digit[gear], LINE2(9));

    key = read_switches(STATE_CHANGE);  // Read the switches for gear changes
    if (key != ALL_RELEASED) {
        if ((key == MK_SW1 || key == MK_SW2) && gear == 8) {
            gear = 1;  // Change gear
        }
        else if (key == MK_SW1) {
            gear++;  // Increase gear
            if (gear >= 7) gear = 7;  // Limit to gear 7
        }
        else if (key == MK_SW2 && gear > 1) {
            gear--;  // Decrease gear
            if (gear <= 1) gear = 1;  // Limit to gear 1
        }
        else if (key == MK_SW3) {
            gear = 8;  // Special case for gear 8
        }
        else if (key == MK_SW11) {
            CLEAR_DISP_SCREEN;  // Clear screen
            db_flag = 1;  // Switch to dashboard flag
        }

        // Store data in EEPROM if gear changes
        if (gear != previous_gear) {
            events++;  // Increment event count
            store_data_in_eeprom(time, digit[gear], temp);  // Store time, gear, and speed in EEPROM
            previous_gear = gear;  // Update previous gear
        }

        clcd_print(digit[gear], LINE2(9));  // Display gear on LCD
        clcd_print(" ", LINE2(11));  // Padding
    }
}

// Scrolling function for menu navigation
void scroll(void) {
    key = read_switches(STATE_CHANGE);  // Read the switches for menu navigation
    if (key == MK_SW2) {
        if (star_flag == 1) sc++;  // Increase menu index
        if (sc >= 2) sc = 2;  // Limit to index 2
        star_flag = 1;
    }
    else if (key == MK_SW1) {
        if (star_flag == 0) {
            if (sc > 0) sc--;  // Decrease menu index
        }
        star_flag = 0;
    }
    else if (key == MK_SW12) {
        CLEAR_DISP_SCREEN;  // Clear screen
        db_flag = 0;  // Reset dashboard flag
    }

    // Menu selection based on the current menu index (sc)
    if (key == MK_SW11) {
        if (sc == 0 && star_flag == 0) {
            CLEAR_DISP_SCREEN;  // Clear screen
            db_flag = 2;  // Show view logs
        } else if (sc == 0 && star_flag == 1) {
            CLEAR_DISP_SCREEN;  // Clear screen
            t_flag = 1;
            db_flag = 3;  // Set time screen
        } else if (sc == 1 && star_flag == 1) {
            CLEAR_DISP_SCREEN;  // Clear screen
            db_flag = 4;  // Download logs
        } else if (sc == 2 && star_flag == 1) {
            CLEAR_DISP_SCREEN;  // Clear screen
            db_flag = 5;  // Clear logs
        } else if (sc == 2 && star_flag == 0) {
            CLEAR_DISP_SCREEN;  // Clear screen
            db_flag = 5;  // Clear logs
        }
    }

    clcd_print(scrolling[sc], LINE1(1));  // Display the current scrolling menu item
    clcd_print(scrolling[sc+1], LINE2(1));  // Display the next scrolling item

    // Set the current state indicator (*) based on the menu navigation
    if (star_flag == 0) {
        clcd_putch('*', LINE1(0));  // Show indicator on line 1
        clcd_putch(' ', LINE2(0));  // Hide indicator on line 2    
    } else {
        clcd_putch('*', LINE2(0));  // Show indicator on line 2
        clcd_putch(' ', LINE1(0));  // Hide indicator on line 1
    }
}
// Custom string copy function to copy content from arr2 to arr1
void my_strcpy(unsigned char *arr1, unsigned char *arr2) {
    // Loop through each character of arr2 until the null-terminator is encountered
    for (int b = 0; arr2[b] != '\0'; b++) {
        arr1[b] = arr2[b];  // Copy each character from arr2 to arr1
    }
}

// Function to manage and manipulate logs from the EEPROM
void logs(void) {
    a = 0x00;  // Reset the EEPROM address pointer to 0
    int m, n;
    
    // Loop through 10 log entries
    for (m = 0; m < 10; m++) {
        // Read each character from the EEPROM and store it in 'str'
        for (n = 0; n < 14; n++) {
            if (n == 2 || n == 5) {
                str[m][n] = ':';  // Insert ':' at fixed positions (e.g., time format)
            } else if (n == 8 || n == 11) {
                str[m][n] = ' ';  // Insert space at specific positions
            } else {
                str[m][n] = external_eeprom_read(a++);  // Read data from EEPROM
            }
        }
        str[m][n] = '\0';  // Null-terminate the string
    }

    // If more than 10 events are recorded, shift the logs in the array
    if (events > 10) {
        shift = events % 10;  // Calculate the number of logs to shift
        for (int f = 0; f < shift; f++) {
            my_strcpy(buffer, str[0]);  // Store the first log in a temporary buffer
            
            // Shift logs down by one position
            for (int g = 0; g < 9; g++) {
                my_strcpy(str[g], str[g + 1]);
            }
            my_strcpy(str[9], buffer);  // Insert the buffered log at the last position
        }
        shift = 0;  // Reset the shift count
    }
}

// Function to set the time on the DS1307 RTC module
void set_time(void) {
    if (t_flag == 1) {
        // Extract hour, minute, and second from the 'time' array (ASCII to integer conversion)
        h = (time[0] - 48) * 10 + (time[1] - 48);
        m = (time[3] - 48) * 10 + (time[4] - 48);
        s = (time[6] - 48) * 10 + (time[7] - 48);
    }

    blink = !blink;  // Toggle the blink flag to alternate time digit visibility
    clcd_print("HH:MM:SS ", LINE1(0));  // Print time header on the LCD screen

    key = read_switches(STATE_CHANGE);  // Read the state of the switches (buttons)

    if (key == MK_SW2) {  // Switch to the next time field (hour, minute, second)
        if (swap >= 0 && swap < 2)
            swap++;
        else if (swap >= 2)
            swap = 0;
    }

    // Handle time field adjustments based on button presses (MK_SW1)
    if (key == MK_SW1) {  
        if (swap == 0) {
            if (h >= 0 && h <= 23)
                h++;  // Increment hour (wrap around to 0 if it exceeds 23)
            else
                h = 0;
        }
        if (swap == 1) {
            if (m >= 0 && m <= 59)
                m++;  // Increment minute (wrap around to 0 if it exceeds 59)
            else
                m = 0;
        }
        if (swap == 2) {
            if (s >= 0 && s <= 59)
                s++;  // Increment second (wrap around to 0 if it exceeds 59)
            else
                s = 0;
        }
    }

    // Display hour, minute, and second on the LCD screen
    clcd_putch('0' + (h / 10), LINE2(0));
    clcd_putch('0' + (h % 10), LINE2(1));
    clcd_putch(':', LINE2(2));
    clcd_putch('0' + (m / 10), LINE2(3));
    clcd_putch('0' + (m % 10), LINE2(4));
    clcd_putch(':', LINE2(5));
    clcd_putch('0' + (s / 10), LINE2(6));
    clcd_putch('0' + (s % 10), LINE2(7));
    clcd_print(" ", LINE2(8));

    // Blink the active time field (hour, minute, second)
    if (swap == 0) {
        if (blink) {
            clcd_putch('0' + (h / 10), LINE2(0));
            clcd_putch('0' + (h % 10), LINE2(1)); 
        } else {
            clcd_putch(' ', LINE2(0));
            clcd_putch(' ', LINE2(1));
        }
    }

    if (swap == 1) {
        if (blink) {
            clcd_putch('0' + (m / 10), LINE2(3));
            clcd_putch('0' + (m % 10), LINE2(4)); 
        } else {
            clcd_putch(' ', LINE2(3));
            clcd_putch(' ', LINE2(4));
        }
    }

    if (swap == 2) {
        if (blink) {
            clcd_putch('0' + (s / 10), LINE2(6));
            clcd_putch('0' + (s % 10), LINE2(7)); 
        } else {
            clcd_putch(' ', LINE2(6));
            clcd_putch(' ', LINE2(7));
        }
    }

    // Delay for stability
    for (unsigned long int i = 100000; i--;);  

    // Save the time settings to DS1307 when MK_SW11 is pressed
    if (key == MK_SW11) {
        write_ds1307(HOUR_ADDR, (h / 10) << 4 | (h % 10));
        write_ds1307(MIN_ADDR, (m / 10) << 4 | (m % 10));
        write_ds1307(SEC_ADDR, (s / 10) << 4 | (s % 10));
        h = 0;  // Reset time after saving it to DS1307
        m = 0;
        s = 0;
        swap = 0;
        db_flag = 0;  
    }

    // Reset time settings when MK_SW12 is pressed
    if (key == MK_SW12) {
        h = 0;
        m = 0;
        s = 0;
        swap = 0;
        db_flag = 1;
    }

    t_flag = 0;  // Reset the flag to indicate time setting is completed
}

// Function to clear logs stored in memory
void clear_log(void) {
    events = 0;  // Reset the event count
    clcd_print("Clearing Logs..", LINE1(0));  // Display clearing message on LCD
    clcd_print("Just a minute ", LINE2(0));  // Inform user about the process
    for (unsigned long int k = 500000; k--;);  // Simulate a delay for clearing logs
    db_flag = 1;  // Return to main dashboard after clearing logs
}

// Function to download logs through UART interface
void download_log(void) {
    if (events == 0) {
        puts("No Logs Available here \n\r");  // No logs to download
        clcd_print("No logs to ", LINE1(0));
        clcd_print("Download....", LINE2(4));  // Display message if no logs available
        for (unsigned long int i = 500000; i--;);  // Simulate delay
        db_flag = 1;  // Return to main dashboard
    } else {
        if (events < 10) {
            down_flag = events;  // Set the download flag to the number of events if less than 10
        } else {
            down_flag = 10;  // Limit download to 10 logs
        }
        logs();  // Fetch the logs from the memory
        clcd_print("DOWNLOADING.....", LINE1(0));  // Display download message
        clcd_print("THROUGH UART..!!", LINE2(0));  // Display UART transfer message
        puts("# TIME EV SP\n\r");
        for (int z = 0; z < down_flag; z++) {
            putch('0' + z);  // Print log number
            putch(' ');
            putch(' ');
            putch(' ');
            puts(str[z]);  // Print the log data through UART
            puts("\n\r");
        }
        for (unsigned long int i = 500000; i--;);  // Simulate delay
        db_flag = 1;  // Return to main dashboard after download
    }
}

// Function to view logs on the LCD screen
void view_log(void) {
    if (events == 0) {
        clcd_print("NO LOGS ", LINE1(0));  // Display message if no logs to view
        clcd_print("TO DISPLAY...:-(", LINE2(0));
        for (b = 500000; b--;);  // Simulate delay
        db_flag = 1;  // Return to main dashboard
    }
    
    if (e_flag == 0) {
        logs();  // Fetch logs if not already done
        e_flag = 1;  // Set flag to indicate logs have been fetched
    }

    if (events > 0) {
        clcd_print("# VIEW LOGS : ", LINE1(0));  // Display log viewing message
        if (c == 1) {
            d_flag = 0;
            c = 0;
            
        }
        key = read_switches(STATE_CHANGE);
        if(events < 10)
            v_flag = 1;
        if(key == MK_SW2){
            if(v_flag == 1){
                if(d_flag < events-1){
                    d_flag++;
                }
            }
            else{
                if(d_flag < 9){
                    d_flag++;
                }
            }
            
        }
         if(key == MK_SW1){
             if(d_flag>0){
                 d_flag--;
             }
             
       
       
         }
       // logs();
        
        clcd_putch('0'+ d_flag,LINE2(0));
        clcd_putch(' ',LINE2(1));
        
        clcd_print(str[d_flag],LINE2(2));
        if(key == MK_SW12){
            CLEAR_DISP_SCREEN;
            db_flag = 1;
        }
        
    }
     
}
void init_config(void)
{
 init_matrix_keypad();
 init_clcd();
    init_adc();
    init_i2c();
 init_ds1307();
    init_uart();
	
}
void main(void) {
    init_config();  // Call the initialization function to set up peripherals and hardware
    while (1) {  // Infinite loop to keep the program running
        get_time();  // Retrieve the current time from the RTC or other source

        // Based on the current state (db_flag), different functions are called
        if (db_flag == 0) {  // Main dashboard view
            dash_board();  // Display the main dashboard screen
        } else if (db_flag == 1) {  // Scrolling mode
            scroll();  // Display and scroll through different screens or logs
        } else if (db_flag == 2) {  // View logs
            view_log();  // Show the stored logs to the user
        } else if (db_flag == 3) {  // Set time mode
            set_time();  // Allow the user to set the time
        } else if (db_flag == 4) {  // Download logs mode
            download_log();  // Download logs via UART or another method
        } else if (db_flag == 5) {  // Clear logs mode
            clear_log();  // Clear the stored logs
        }
    }
}

