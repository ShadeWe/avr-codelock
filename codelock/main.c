/*
 * codelock.cpp
 *
 * Created: 04.05.2018 15:05:10
 * Author : Shade
 */ 
#define ALLOWED 10
#define DENIED  11

#define F_CPU 8000000UL

#include "lcd/hd44780.h"

#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>

// the correct password, the only one at the moment
char password[10] = "1234";
char numberOfFigure = 0;						// the number of the entered figure
char passwordEntered[10];

char portStateScreenOn[3] = {0xFB,0xFD,0xFE};
char portStateScreenOff[3] = {0x7B,0x7D,0x7E}; 
	
char portState[3] = {0xFB,0xFD,0xFE};			// the states of the scanning pins
char inputState[4] = {0x40,0x20,0x10,0x8};		// the states of the reading pins

// asterisks are drawn at these positions in the same order as here and it makes them to be closer to the center
char drawPositions[10] = {0x47, 0x48, 0x46, 0x49, 0x45, 0x4A, 0x44, 0x4B, 0x43, 0x4C};

char buttonsValues[4][3]= { {'1','2','3'}, {'4','5','6'}, {'7','8','9'}, {'C','0','E'} };
	
char isScreenOn = 1;
int compareValue = 0;	// when this value reaches 500, the screen is turned off.

ISR(TIMER0_COMPA_vect) {
	
	// if the screen is on
	if (isScreenOn == 1) {
		compareValue++;
		if (compareValue >= 500) {
			memcpy(portState, portStateScreenOff, sizeof(portState));			// turn off the screen
			isScreenOn = 0;
		}
	}
		
	for(char i = 0; i < 3; i++)
	{
		PORTB=portState[i];
		for(char j = 0; j < 4; j++)
		{
			if(((PINB&inputState[j])==0))
			{
				// turn on the screen, but don't process the keystroke
				if (isScreenOn == 0) {
					memcpy(portState, portStateScreenOn, sizeof(portState));	// turn on the screen
					isScreenOn = 1;
					compareValue = 0;
					_delay_ms(500);		// this delay should be shorter
				}
				else {
					// if the button isn't unpressed, wait
					while((PINB&inputState[j])!=inputState[j]);
					onClick(buttonsValues[j][i]);
				}
			}
		}
	}
	
}

// PWM
ISR (TIMER2_COMPA_vect)
{
	if (PORTC == 0b00000001)	PORTC = 0b00000000;
	else						PORTC = 0b00000001;
}	

void asterisksDrawing(char num) {
	lcd_goto(drawPositions[num]);
	lcd_putc('*');
}

void accessDeniedScreen() {
	lcd_clrscr();
	lcd_home();
	lcd_puts(" ACCESS  DENIED");
	
	sound(DENIED);
	_delay_ms(1000);
	clearEnteredPassword();
}

void accessAllowedScreen() {
	lcd_clrscr();
	lcd_home();
	lcd_puts(" ACCESS ALLOWED");
	
	PORTD = 0b10000000;				// this action opens the door
	sound(ALLOWED);
	
	_delay_ms(1000);
	lcd_clrscr();
	lcd_home();
	lcd_puts("THE DOOR WILL BE");
	lcd_goto(0x40);
	lcd_puts("CLOSED IN  5 SEC");
	_delay_ms(1000);
	lcd_goto(0x4B);
	lcd_putc('4');
	_delay_ms(1000);
	lcd_goto(0x4B);
	lcd_putc('3');
	_delay_ms(1000);
	lcd_goto(0x4B);
	lcd_putc('2');
	_delay_ms(1000);
	lcd_goto(0x4B);
	lcd_putc('1');
	_delay_ms(1000);
	
	PORTD = 0b00000000;				// closes the door after 5 seconds passed
	
	clearEnteredPassword();
	enterPasswordScreen();
}

void enterPasswordScreen() {
	lcd_clrscr();
	lcd_home();
	lcd_puts(" ENTER PASSWORD ");
}

void clearEnteredPassword() {
	memset(passwordEntered, 0, 10);
	numberOfFigure = 0;
	enterPasswordScreen();
}

void sound(char sound) {
	
	switch (sound) {
		case ALLOWED: 
			OCR2A = 7;
			TCCR2A = 0b00110011;
			TCCR2B = 0b00001111;
			_delay_ms(80);
			TCCR2A = 0b00000011;
			TCCR2B = 0b00001000;
			_delay_ms(80);
			OCR2A = 5;
			TCCR2A = 0b00110011;
			TCCR2B = 0b00001111;
			_delay_ms(80);
			TCCR2A = 0b00000011;
			TCCR2B = 0b00001000;
		break;
		
		case DENIED:
			OCR2A = 15;
			TCCR2A = 0b00110011;
			TCCR2B = 0b00001111;
			_delay_ms(150);
			TCCR2A = 0b00000011;
			TCCR2B = 0b00001000;
			_delay_ms(150);
			OCR2A = 15;
			TCCR2A = 0b00110011;
			TCCR2B = 0b00001111;
			_delay_ms(150);
			TCCR2A = 0b00000011;
			TCCR2B = 0b00001000;
		break;
	}
}

void onClick(char buttonPressed) {
	switch (buttonPressed) {
		// confirm
		case 'E':
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
			if (numberOfFigure >= 10) break;
			passwordEntered[numberOfFigure] = buttonPressed;
			asterisksDrawing(numberOfFigure);
			numberOfFigure++;
		break;
	}
}

void timerInit() {
	 
	// Interrupt frequency = Fclk/(N*(1+OCR1A))
	// N - the value of the prescaler
	
	// timer 0 initialization
	TCCR0A = (1<<WGM01);				// The CTC mode. The compare interrupt occurs when the compare value is reached
	TCCR0B = (1<<CS00) | (1 << CS02);	// 1024 prescaler
	OCR0A = 200;						// the compare value
	TIMSK0 = (1 << OCIE0A);
	
	sei();
}

int main(void)
{
	timerInit();
	
	// scanning and reading pins
	DDRB = 0b10000111;
	PORTB = 0b01111111;

	// the fourth pin is buzzer, the last pin (the leftmost bit) opens/closes the door.
	DDRD = 0b10001000;
		
	lcd_init();
	
	enterPasswordScreen();
	
	DDRC = 0b00000000;
	PORTC = 0x00;
	
	while (1) {
		
		// there's a button that opens the door from inside, and this true when it's pressed
		if ((PINC&(1 << PORTC5)) == 0) {
			accessAllowedScreen();
		}
	}
}

