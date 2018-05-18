/*
 * eeprom.h
 *
 * Created: 14.05.2018 13:18:27
 *  Author: Shade
 */ 

#ifndef EEPROM_H_
#define EEPROM_H_

void EEPROM_write(unsigned int uiAddress, unsigned char ucData);
unsigned char EEPROM_read(unsigned int uiAddress);

#endif /* EEPROM_H_ */