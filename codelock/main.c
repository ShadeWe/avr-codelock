/*
 * codelock.cpp
 *
 * Created: 04.05.2018 15:05:10
 * Author : Shade
 */ 

#define F_CPU 8000000UL

#include "lcd/hd44780.h"

#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>

// the correct password, the only one at the moment
char password[10] = "1234";

char pendingChar = 0;		// the number of the entered figure

char passwordEntered[10];

char portState[3]= {0xEF,0xDF,0xBF};			// the states of the scanning pins
char inputState[4] = {0x01,0x02,0x04,0x08};		// the states of the reading pins
	
// asterisks are drawn at these positions in the same order as here and it makes them to be closer to the center
char positions[10] = {0x47, 0x48, 0x46, 0x49, 0x45, 0x4A, 0x44, 0x4B, 0x43, 0x4C};

char buttonsValues[4][3]= { {'1','2','3'}, {'4','5','6'}, {'7','8','9'}, {'C','0','E'} };

ISR(TIMER0_COMPA_vect) {
	
	for(char i = 0; i < 3; i++)
	{
		PORTB=portState[i];
		for(char j = 0; j < 4; j++)
		{
			if(((PINB&inputState[j])==0))
			{
				// waits until a button is unpressed
				while((PINB&inputState[j])!=inputState[j]);
				
				onClick(buttonsValues[j][i]);
			}
		}
	}
		
}

void asterisksDrawing(char num) {
	lcd_goto(positions[num]);
	lcd_putc('*');
}

void accessDeniedScreen() {
	lcd_clrscr();
	lcd_home();
	lcd_puts(" ACCESS  DENIED");
	_delay_ms(1000);
	
	clearEnteredPassword();
}

void accessAllowedScreen() {
	lcd_clrscr();
	lcd_home();
	lcd_puts(" ACCESS ALLOWED");
	_delay_ms(1000);
	lcd_clrscr();
	lcd_home();
}

void enterPasswordScreen() {
	lcd_clrscr();
	lcd_home();
	lcd_puts(" ENTER PASSWORD ");
}

void clearEnteredPassword() {
	memset(passwordEntered, 0, 10);
	pendingChar = 0;
	enterPasswordScreen();
}

void onClick(char buttonPressed) {
	switch (buttonPressed) {
		
		// confirm
		case 'E':
			
			// if the password is correct
			if (strcmp(passwordEntered, password) == 0)
				accessAllowedScreen();
			else
				accessDeniedScreen();

		break;
		
		// clear
		case 'C':	
			clearEnteredPassword();
		break;
		
		// handle numbers
		default:
			
			// passwords are less or 10 characters long
			if (pendingChar >= 10) 
				break;
			
			passwordEntered[pendingChar] = buttonPressed;
			asterisksDrawing(pendingChar);
			pendingChar++;
			
		break;
	}
}

void timerInit() {
	
	TCCR1A = (1<<WGM01);				// The CTC mode. The compare interrupt occurs when the compare value is reached
	TCCR1B |= (1<<CS00);				// Using FCPU
	TCCR0B = (1<<CS00) | (1 << CS02);	// prescaler
	OCR0A = 200;						// the compare value

	// Interrupt frequency = Fclk/(N*(1+OCR1A))
	// N - the value of the prescaler
	
	TIMSK0 = (1 << OCIE0A);
	
	sei();
}



int main(void)
{
	timerInit();
	
	// scanning and reading pins
	PORTB = 0b01111111;
	DDRB = 0b01110000;
	
	lcd_init();
	
	enterPasswordScreen();
	
	while (1);
}

