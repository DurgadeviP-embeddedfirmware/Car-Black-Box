/*
 * File:   external_eeprom.c
 * Author: DURGA
 * description:CAR BLACK BOX 
 * Created on 19 November, 2024, 8:00 PM
 */


#include "main.h"
#include "external_eeprom.h"
#include "i2c.h"
#include <xc.h>


void external_eeprom_write(unsigned char address, unsigned char data)
{
	i2c_start();
	i2c_write(SLAVE_EEPROM_WRITE);
	i2c_write(address);
	i2c_write(data);
	i2c_stop();
}

unsigned char external_eeprom_read(unsigned char address)
{
	unsigned char data;

	i2c_start();
	i2c_write(SLAVE_EEPROM_WRITE);
	i2c_write(address);
	i2c_rep_start();
	i2c_write(SLAVE_EEPROM_READ);
	data = i2c_read();
	i2c_stop();

	return data;
}