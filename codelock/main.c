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

char portState[3]= {0xEF,0xDF,0xBF};
char inputState[4] = {0x01,0x02,0x04,0x08};
char i, j = 0;

char mass2[4][3]={{'1','2','3'},
				  {'4','5','6'},
				  {'7','8','9'},
				  {'C','0','E'}};

ISR(TIMER1_COMPA_vect) {
		
	for(i=0; i<3; i++)
	{
		PORTB=portState[i];
		for(j=0; j<4; j++)
		{
			if(((PINB&inputState[j])==0))
			{
				while((PINB&inputState[j])!=inputState[j]){};
				lcd_putc(mass2[j][i]);
			}
		}
	}
		
}

void timerInit() {
	
	// Timer 1
	TCCR1B = (1<<WGM12);    // The CTC mode. The compare interrupt occurs when the compare value is reached
	TCCR1B |= (1<<CS10);	// Using FCPU
	
	/* prescalers */
	
	TCCR1B |= (1<<CS10)|(1<<CS11);			// CLK/64
	
	// TCCR1B |= (1<<CS11);					// CLK/8
	// TCCR1B |= (1<<CS12);					// CLK/256
	// TCCR1B |= (1<<CS10)|(1<<CS12);		// CLK/1024

	OCR1A = 1000;          // The compare value
	
	// Interrupt frequency = Fclk/(N*(1+OCR1A))
	// N - the value of the prescaler
	
	TIMSK1 = (1<<OCIE1A);  // enable the compare interrupt
	
	sei();
}

int main(void)
{
	timerInit();
	
	lcd_init();
	lcd_clrscr();
	
	PORTB = 0b01111111;
	DDRB = 0b01110000;
	
	
    while (1)
    {
		
    }
}

