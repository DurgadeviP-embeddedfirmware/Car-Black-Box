/* 
 * File:   eeprom.h
 * Author: DURGA
 * description:CAR BLACK BOX 
 * Created on 18 November, 2024, 8:00 PM
 */

#ifndef EEPROM_H
#define EEPROM_H

void write_internal_eeprom(unsigned char address, unsigned char data); 
unsigned char read_internal_eeprom(unsigned char address);

#endif