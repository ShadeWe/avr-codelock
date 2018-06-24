# avr-codelock
This project contains a firmware for a simple digital code lock written in C. Access is controlled by a 3x4 matrix keyboard. 
Information is displayed on a LED module based on the Hitachi HD44780 LCD controller. The user is able to add and delete passwords, turn on/off bruteforce protection, change producing sounds.
The device can also be configured for using different electromagnetic locks which default states (without power supply) can be either open or shut.
The microcontroller used in this device is _ATmega328P_, but _ATmega8_ can also be used. Since the device's power supply is 12V (electromagnetic locks usually take 12V), there's a 5V voltage regulator for supplying the logic part.

_The circuit diagram of this code lock:_
<img src="https://lh6.googleusercontent.com/q_g6ilXKKerlRdi1DlZqE8ke4TGtYyuDD5x3gi6AFBSH6ftueQnygeeHY1AeY0dMpkISRz6OfBA1MAttWBEo=w1366-h635">
