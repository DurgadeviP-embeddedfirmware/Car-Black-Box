/* 
 * File:   external_eeprom.h
 *Author: DURGA
 * description:CAR BLACK BOX 
 * Created on 18 November, 2024, 8:00 PM
 */

#ifndef EXTERNAL_EEPROM_H
#define	EXTERNAL_EEPROM_H

#define SLAVE_EEPROM_WRITE 0XA0
#define SLAVE_EEPROM_READ  0XA1

void external_eeprom_write(unsigned char address1,unsigned char data);
unsigned char external_eeprom_read(unsigned char address1);

#endif	/* EXTERNAL_EEPROM_H */

