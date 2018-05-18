
/*
 * eeprom.c
 *
 * Created: 14.05.2018 13:18:39
 *  Author: Shade
 */ 

#include "eeprom.h"

void EEPROM_write(unsigned int uiAddress, unsigned char ucData)

{

	while(EECR & (1<<EEPROM_write())); //���� ������������ ����� ��������� ��������� ��������� � �������

	EEAR = uiAddress; //������������� �����

	EEDR = ucData; //����� ������ � �������

	EECR |= (1<<EEMWE); //��������� ������

	EECR |= (1<<EEWE); //����� ���� � ������

}

unsigned char EEPROM_read(unsigned int uiAddress)

{

	while(EECR & (1<<EEWE));  //���� ������������ ����� ��������� ��������� ��������� � �������

	EEAR = uiAddress; //������������� �����

	EECR |= (1<<EERE); //��������� �������� ���������� �� ������ � ������� ������

	return EEDR; //���������� ���������

}