#include <avr/eeprom.h>

uint8_t EEMEM password0[10] = "1234";	// the default password
uint8_t EEMEM password1[10] = "none";
uint8_t EEMEM password2[10] = "none";
uint8_t EEMEM password3[10] = "none";
uint8_t EEMEM password4[10] = "none";
uint8_t EEMEM password5[10] = "none";
uint8_t EEMEM password6[10] = "none";
uint8_t EEMEM password7[10] = "none";
uint8_t EEMEM password8[10] = "none";
uint8_t EEMEM password9[10] = "none";

uint8_t EEMEM normalLockState[10] = "closed";

uint8_t * pointers[11] = { &password0, 
						   &password1, 
						   &password2, 
						   &password3, 
						   &password4, 
						   &password5, 
						   &password6, 
						   &password7, 
						   &password8, 
						   &password9,
						   &normalLockState
						 };