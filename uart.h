/* 
 * File:   uart.h
 * Author: DURGA
 * description:CAR BLACK BOX 
 * Created on 18 November, 2024, 8:00 PM
 */




#ifndef SCI_H
#define SCI_H

#define RX_PIN					TRISC7
#define TX_PIN					TRISC6

void init_uart(void);
void putch(unsigned char byte); //transmit 1 byte of data
int puts(const char *s); //transmit a string
unsigned char getch(void); //receive 1 byte of data
//unsigned char getch_with_timeout(unsigned short max_time);
unsigned char getche(void);//receive, transmit 1 byte of data

#endif
