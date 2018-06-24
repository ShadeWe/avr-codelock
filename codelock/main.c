/*
 * codelock.cpp
 *
 * Created: 04.05.2018 15:05:10
 * Author : Shade
 */


// sound
#define DENIED					100
#define ALLOWED					101
#define NEW_PASS_ADDED			102
#define VERIFICATION_STARTED	103

#define NO_PLACE 12

// actions
#define ADD_PASS	0
#define DEL_PASS	1
#define BRUTEFORCE	2
#define CHANGESTATE 3
#define CHANGESOUND 4
#define NOTHING		9

#define F_CPU 8000000UL

#include "lcd/hd44780.h"
#include "eeprom_vars.h"	// simply contains an array containing pointers to EEPROM memory for passwords to be saved at.

#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>

// when the device is turned on, the function readEEPROMpasswords is called and the passwords from the EEPROM are copied into this array.
char passwords[10][10];
char passwordsInTotal = 0;	// the number of free cells

char numberOfFigureEntered = 0;
char passwordEntered[11];

// either of these is copied into the portState variable depending on the situation (when the LED screen is to be turned on/off)
char portStateScreenOn[3] = {0xFB,0xFD,0xFE};
char portStateScreenOff[3] = {0x7B,0x7D,0x7E}; 
	
char portState[3] = {0xFB,0xFD,0xFE};			// the states of the scanning pins
char inputState[4] = {0x40,0x20,0x10,0x8};		// the states of the reading pins

// asterisks are drawn at these positions in the same order:
char drawPositions[10] = {0x47, 0x48, 0x46, 0x49, 0x45, 0x4A, 0x44, 0x4B, 0x43, 0x4C};

char buttonsValues[4][3]= { {'1','2','3'}, {'4','5','6'}, {'7','8','9'}, {'C','0','E'} };

int screenValue = 0;			// is incremented everytime the TIMER0_COMPA_vect interrupt is triggered and reset when there's a keystroke
int setmenuValue = 0;			// is incremented while the button inside of the room is held
	
char isScreenOn = 1;
char isSettingsMenuOn = 0;			
char verification = 0;					// 1 if user verification is in progress
char pickedOption = 0;
char pickedPassword = 0;

char optionBeingPerformed = NOTHING;
char eepromFreeCell = NO_PLACE;			// if this variable is still equal to NO_PLACE after searching for place for a new password - there's no place.

char isBruteforceProtectionOn = 0;
char failsConsecutively = 0;			// the code lock is blocked when there's 3 fails in a row
char defaultLockState = 0;				// 0 if the default state is closed, 1 if open

int systemBlockingTime = 30000;			// it's 30 seconds, but with next 3 times fails this time is multiplied by 2

char pickedSound = 1;					// there's 3 different sounds

char specialKey[10] = "7784525671";

ISR(TIMER0_COMPA_vect) {
	
	// there's a button that opens the door from inside, and this true when it's pressed
	if ((PINC&(1 << PORTC5)) == 0 && isSettingsMenuOn == 0) {
		// while this button is held
		while ((PINC&(1 << PORTC5)) == 0) {
			setmenuValue++;
			if (setmenuValue == 500) {
				verification = 1;
				sound(VERIFICATION_STARTED);
			}
			_delay_ms(4);
		}
		if (setmenuValue < 500)
			accessAllowedScreen();
			
		setmenuValue = 0;
	}
	
	// if the screen is on
	if (isScreenOn == 1) {
		screenValue++;
		if (screenValue >= 500) {
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
					memcpy(portState, portStateScreenOn, sizeof(portState));	// turn on the screen, if it's off
					isScreenOn = 1;
					screenValue = 0;
					_delay_ms(100);		
				}
				else {
					screenValue = 0;
					int temp = 0;
					char skipProcessing = 0;
					while((PINB&inputState[j])!=inputState[j]) {
						if (buttonsValues[j][i] == 'E') {
							temp++;
							if (temp >= 500) {
								if (strcmp(passwordEntered, specialKey) == 0) {
									isSettingsMenuOn = 1;
									settingsMenuScreen(pickedOption);
									memset(passwordEntered, 0, 10);
									numberOfFigureEntered = 0;
									skipProcessing = 1;
								}
							}
							_delay_ms(4);
						}
					}
					if (skipProcessing == 0)
						onButtonPressed(buttonsValues[j][i]);
					
					_delay_ms(5);
				}
			}
		}
	}
}

// PWM
ISR (TIMER2_COMPA_vect)
{
	PORTC = !PORTC;
}	

void asterisksDrawing(char num) {
	lcd_goto(drawPositions[num]);
	lcd_putc('*');
}

void my_delay_ms(int n) {
	while(n--) {
		_delay_ms(1);
	}
}

void accessDeniedScreen() {
	lcd_clrscr();
	lcd_home();
	lcd_puts(" ACCESS  DENIED");
	sound(DENIED);
	_delay_ms(1000);
	
	if (isBruteforceProtectionOn == 1) {;
		failsConsecutively++;
		if (failsConsecutively == 3) {
			lcd_clrscr();
			lcd_home();
			lcd_puts("3 FAILS IN A ROW");
			lcd_goto(0x40);
			lcd_puts(" SYSTEM BLOCKED ");
			_delay_ms(1000);
			PORTB = 0b00000000;			// turn off the screen
			lcd_clrscr();
			lcd_home();
			my_delay_ms(systemBlockingTime);
			failsConsecutively = 0;
			systemBlockingTime = systemBlockingTime * 2;
			PORTB = 0b10000000;			// turn on the screen
		}	
	}
		
	clearEnteredPassword();	
}

void deletePasswordScreen() {
	char buffer[16];
	sprintf(buffer, "%d. %s", pickedPassword, passwords[pickedPassword]);
	
	lcd_clrscr();
	lcd_home();
	lcd_puts(" PRESS E TO DEL ");
	lcd_goto(0x40);
	lcd_puts(buffer);
}
void settingsMenuScreen(char pickedOption) {
	lcd_clrscr();
	lcd_home();
	switch(pickedOption) {
		case 0:
			lcd_puts("ADD PASS");
			lcd_goto(0x40);
			lcd_puts("DEL PASS");
			lcd_goto(0x0F);
			lcd_putc('o');
		break;
		case 1:
			lcd_puts("ADD PASS");
			lcd_goto(0x40);
			lcd_puts("DEL PASS");
			lcd_goto(0x4F);
			lcd_putc('o');
		break;
		case 2:
			if (isBruteforceProtectionOn == 1)
				lcd_puts("BRUTEFORCE on");
			else
				lcd_puts("BRUTEFORCE off");
			lcd_goto(0x0F);
			lcd_putc('o');
			lcd_goto(0x40);
			if (defaultLockState == 0) {
				lcd_puts("NORM. ST. shut");
				} else {
				lcd_puts("NORM. ST. open");
			}
		break;
		case 3:
			if (isBruteforceProtectionOn == 1)
			lcd_puts("BRUTEFORCE on");
			else
			lcd_puts("BRUTEFORCE off");
			lcd_goto(0x40);
			if (defaultLockState == 0) {
				lcd_puts("NORM. ST. shut");				
			} else {
				lcd_puts("NORM. ST. open");			
			}
			lcd_goto(0x4F);
			lcd_puts("o");
			break;
			
		case 4:
			if (pickedSound == 1) {
				lcd_puts("CHANGE SOUND 1 ");
			}
			if (pickedSound == 2) {
				lcd_puts("CHANGE SOUND 2 ");
			}
			if (pickedSound == 3) {
				lcd_puts("CHANGE SOUND 3 ");
			}
			lcd_goto(0x0F);
			lcd_putc('o');
		break;
		default:
		break;
	}
}
void accessAllowedScreen() {
	systemBlockingTime = 30000;
	
	lcd_clrscr();
	lcd_home();
	lcd_puts(" ACCESS ALLOWED");
	
	// depending on the electromagnetic lock used, the door opens either by supplying or removing voltage.
	if (defaultLockState == 0) {
		PORTD = 0b10000000;				// this action opens the door
	}
	else {
		PORTD = 0b00000000;				// this action opens the door
	}
	
	PORTC = 0b00010000;		// luminodiode blinking		
	sound(ALLOWED);
	_delay_ms(500);
	PORTC = 0b00000000;		// luminodiode blinking
	
	lcd_clrscr();
	lcd_home();
	lcd_puts("THE DOOR WILL BE");
	lcd_goto(0x40);
	lcd_puts("CLOSED IN  3 SEC");
	_delay_ms(1000);
	lcd_goto(0x4B);
	lcd_putc('2');
	_delay_ms(1000);
	lcd_goto(0x4B);
	lcd_putc('1');
	_delay_ms(1000);
	
	// the same thing is here
	if (defaultLockState == 0) {
		PORTD = 0b00000000;				// this action opens the door
	}
	else {
		PORTD = 0b10000000;				// this action opens the door
	}
	
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
	numberOfFigureEntered = 0;
	enterPasswordScreen();
}

void sound(char sound) {
	
	switch (sound) {
		case VERIFICATION_STARTED:
			OCR2A = 7;
			TCCR2A = 0b00110011;
			TCCR2B = 0b00001111;
			_delay_ms(80);
			TCCR2A = 0b00000011;
			TCCR2B = 0b00001000;
		break;
		case ALLOWED: 
			if (pickedSound == 1) {
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
			}
			
			if (pickedSound == 2) {
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
				OCR2A = 3;
				TCCR2A = 0b00110011;
				TCCR2B = 0b00001111;
				_delay_ms(110);
				TCCR2A = 0b00000011;
				TCCR2B = 0b00001000;
			}
			
			if (pickedSound == 3) {
				OCR2A = 5;
				TCCR2A = 0b00110011;
				TCCR2B = 0b00001111;
				_delay_ms(80);
				TCCR2A = 0b00000011;
				TCCR2B = 0b00001000;
				_delay_ms(80);
				OCR2A = 3;
				TCCR2A = 0b00110011;
				TCCR2B = 0b00001111;
				_delay_ms(150);
				TCCR2A = 0b00000011;
				TCCR2B = 0b00001000;
			}
		break;
		
		case DENIED:
			if (pickedSound == 1) {
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
			}
			if (pickedSound == 2) {
				OCR2A = 15;
				TCCR2A = 0b00110011;
				TCCR2B = 0b00001111;
				_delay_ms(80);
				TCCR2A = 0b00000011;
				TCCR2B = 0b00001000;
				_delay_ms(80);
				OCR2A = 15;
				TCCR2A = 0b00110011;
				TCCR2B = 0b00001111;
				_delay_ms(80);
				TCCR2A = 0b00000011;
				TCCR2B = 0b00001000;
				_delay_ms(80);
				OCR2A = 15;
				TCCR2A = 0b00110011;
				TCCR2B = 0b00001111;
				_delay_ms(80);
				TCCR2A = 0b00000011;
				TCCR2B = 0b00001000;
			}
			if (pickedSound == 3) {
				OCR2A = 15;
				TCCR2A = 0b00110011;
				TCCR2B = 0b00001111;
				_delay_ms(110);
				TCCR2A = 0b00000011;
				TCCR2B = 0b00001000;
				_delay_ms(110);
				OCR2A = 15;
				TCCR2A = 0b00110011;
				TCCR2B = 0b00001111;
				_delay_ms(110);
				TCCR2A = 0b00000011;
				TCCR2B = 0b00001000;
			}

		break;
		
		case NEW_PASS_ADDED:
			OCR2A = 7;
			TCCR2A = 0b00110011;
			TCCR2B = 0b00001111;
			_delay_ms(60);
			TCCR2A = 0b00000011;
			TCCR2B = 0b00001000;
			_delay_ms(60);
			OCR2A = 7;
			TCCR2A = 0b00110011;
			TCCR2B = 0b00001111;
			_delay_ms(60);
			TCCR2A = 0b00000011;
			TCCR2B = 0b00001000;
		break;
	}
}


void readEEPROMpasswords() {
	char pass[10];
	for (char i = 0; i < 10; i++) {
		eeprom_read_block((void*)&pass, (const void*)pointers[i], 10);
		strcpy(passwords[i], pass);
		if (strcmp(passwords[i], "none"))
			passwordsInTotal++;
	}
	
	char state[10];
	eeprom_read_block((void*)&state, (const void*)&normalLockState, 10);
	if (strcmp(state, "closed") == 0) {
		defaultLockState = 0;
	} else {
		defaultLockState = 1;
	}
}

void performOption() {
	if (optionBeingPerformed == ADD_PASS) {
		eeprom_write_block(passwordEntered, pointers[eepromFreeCell], 10);
		strcpy(passwords[eepromFreeCell], passwordEntered);
		
		eepromFreeCell = NO_PLACE;
		
		lcd_clrscr();
		lcd_puts(" A NEW PASSWORD ");
		lcd_goto(0x40);
		lcd_puts(" HAS BEEN ADDED ");
		sound(NEW_PASS_ADDED);
		_delay_ms(1500);
		
		memset(passwordEntered, 0, 10);
		numberOfFigureEntered = 0;
		passwordsInTotal++;
	}
	
	if (optionBeingPerformed == DEL_PASS) {
		
		if (passwordsInTotal == 1) {
			lcd_clrscr();
			lcd_puts("YOU CAN'T DELETE");
			lcd_goto(0x40);
			lcd_puts(" THE LAST PASS ");
			sound(DENIED);
			_delay_ms(1500);
		}
		else {
			eeprom_update_block("none", pointers[pickedPassword], 10);
			strcpy(passwords[pickedPassword], "none");
			
			lcd_clrscr();
			lcd_puts("  THE PASSWORD  ");
			lcd_goto(0x40);
			lcd_puts("HAS BEEN DELETED");
			
			sound(NEW_PASS_ADDED);
			_delay_ms(1500);
			pickedPassword = 0;
			passwordsInTotal--;
		}
	}
	
	optionBeingPerformed = NOTHING;
	settingsMenuScreen(0);
	isSettingsMenuOn = 1;
	pickedOption = 0;
}
void onButtonPressed(char buttonPressed) {
	switch (buttonPressed) {
		
		// confirm
		case 'E':
					
			// there's an option to be performed
			if (optionBeingPerformed != NOTHING) {
				performOption();
				break;
			}
			
			if (isSettingsMenuOn == 1) {
				if (pickedOption == ADD_PASS) {
					// looking for free place for a new password
					for (char i = 0; i < 10; i++) {
						if (strcmp(passwords[i], "none") == 0) {
							eepromFreeCell = i;
							break;
						}
					}
					
					// if there's place for a new password
					if (eepromFreeCell != 12) {
						optionBeingPerformed = ADD_PASS;
						lcd_clrscr();
						lcd_home();
						lcd_puts(" ENTER NEW PASS ");
						isSettingsMenuOn = 0;
						break;
					} else {
						lcd_clrscr();
						lcd_home();
						lcd_puts("THERE'S NO PLACE");
						lcd_goto(0x40);
						lcd_puts(" FOR A NEW PASS");
						sound(DENIED);
						_delay_ms(1500);
						settingsMenuScreen(0);
					}
				}
				
				if (pickedOption == CHANGESOUND) {
					if (pickedSound != 3) {
						pickedSound++;
						settingsMenuScreen(CHANGESOUND);
						
					} else if (pickedSound == 3) {
						pickedSound = 1;
						settingsMenuScreen(CHANGESOUND);
					}
					
					break;
				}
				
				if (pickedOption == CHANGESTATE) {
					
					if (defaultLockState == 1) {
						defaultLockState = 0;
						settingsMenuScreen(3);
						eeprom_write_block("closed", &normalLockState, 10);
						} else {
						defaultLockState = 1;
						settingsMenuScreen(3);
						eeprom_write_block("open", &normalLockState, 10);
					}
					
					break;
				}
				
				if (pickedOption == BRUTEFORCE) {
					
					if (isBruteforceProtectionOn == 1) {
						isBruteforceProtectionOn = 0;
						settingsMenuScreen(2);
					} else {
						isBruteforceProtectionOn = 1;
						settingsMenuScreen(2);
					}
						
					break;
				}
			
				if (pickedOption == DEL_PASS) {
					optionBeingPerformed = DEL_PASS;
					deletePasswordScreen();
					break;
				}
			}
			
			for (char i = 0; i < 10; i++) {
				
				if (strcmp(passwordEntered, passwords[i]) == 0) {
					if (verification == 0) {
						accessAllowedScreen();
						return 0;
					}
					else {
						isSettingsMenuOn = 1;
						verification = 0;
						
						settingsMenuScreen(pickedOption);
						
						// reset the entered password
						memset(passwordEntered, 0, 10);
						numberOfFigureEntered = 0;
						
						return 0;
					}

				}
				
			}
			accessDeniedScreen();
		break;
		
		// clear
		case 'C':
			if (optionBeingPerformed != NOTHING) {
				
				optionBeingPerformed = NOTHING;
				isSettingsMenuOn = 1;
				numberOfFigureEntered = 0;
				pickedOption = 0;
				pickedPassword = 0;
				
				memset(passwordEntered, 0, 10);
				settingsMenuScreen(0);
			} else {
				clearEnteredPassword();
				if (isSettingsMenuOn == 1)
					isSettingsMenuOn = 0;
			}
		break;
		
		// handle numeric buttons
		default:
			if (isSettingsMenuOn == 0) {
				// passwords are less or 10 characters long, and if the user tries to enter some more - ignore it
				if (numberOfFigureEntered >= 10) break;

				passwordEntered[numberOfFigureEntered] = buttonPressed;
				asterisksDrawing(numberOfFigureEntered);
				numberOfFigureEntered++;		
			} 
			else {
				
				if (buttonPressed == '5' && pickedOption == CHANGESOUND) {
					sound(ALLOWED);
					_delay_ms(1500);
					sound(DENIED);
					break;
				}
				
				if (buttonPressed == '2') {
					if (optionBeingPerformed == DEL_PASS) {
						if (pickedPassword != 0) {
							pickedPassword--;
							deletePasswordScreen();
						}
						break;
					}
					
					if (pickedOption != 0) {
						pickedOption--;
						settingsMenuScreen(pickedOption);
					}
				}
				if (buttonPressed == '8') {
					if (optionBeingPerformed == DEL_PASS) {
						if (pickedPassword < 9) {
							pickedPassword++;
							deletePasswordScreen();
						}
						break;
					}
					
					if (pickedOption != 4) {
						pickedOption++;
						settingsMenuScreen(pickedOption);
					}
				}
			}
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
	readEEPROMpasswords();
	
	if (defaultLockState == 0) {
		PORTD = 0b00000000;
	} else {
		PORTD = 0b10000000;
	}

	enterPasswordScreen();
		
	DDRC = 0b00010000;
	PORTC = 0x00;
	
	while (1) {
		
	}
}

