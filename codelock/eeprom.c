
/*
 * eeprom.c
 *
 * Created: 14.05.2018 13:18:39
 *  Author: Shade
 */ 

#include "eeprom.h"

void EEPROM_write(unsigned int uiAddress, unsigned char ucData)

{

	while(EECR & (1<<EEPROM_write())); //ждем освобождения флага окончания последней операцией с памятью

	EEAR = uiAddress; //Устанавливаем адрес

	EEDR = ucData; //Пищем данные в регистр

	EECR |= (1<<EEMWE); //Разрешаем запись

	EECR |= (1<<EEWE); //Пишем байт в память

}

unsigned char EEPROM_read(unsigned int uiAddress)

{

	while(EECR & (1<<EEWE));  //ждем освобождения флага окончания последней операцией с памятью

	EEAR = uiAddress; //Устанавливаем адрес

	EECR |= (1<<EERE); //Запускаем операцию считывания из памяти в регистр данных

	return EEDR; //Возвращаем результат

}