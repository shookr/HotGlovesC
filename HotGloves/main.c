/*
 * HotGloves.c
 *
 * Created: 14.11.2018
 * Author : Alex Schumann
 * Version : 0.3
 */ 

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <uart.h>
#include <lcd.h>
#include <stdbool.h>
#include <string.h>
//#include <avr/pgmspace.h>
//#include <util/delay.h>
//#include <time.h>

#define F_CPU	3686400UL	// also manually defined in delay.h
#define UART_BAUD_RATE	9600	//115200
#define LCD_PORT	PORTA
#define NAME_OF_BM	"HotGlove"

#define LEVEL0IN	0b00110000
#define LEVEL1IN	0b00110001
#define LEVEL2IN	0b00110010
#define LEVEL3IN	0b00110011
#define LEVEL4IN	0b00110100
#define LEVEL5IN	0b00110101

#define LEVEL0OUT	0b11111111
#define LEVEL1OUT	0b11111110
#define LEVEL2OUT	0b11111100
#define LEVEL3OUT	0b11111000
#define LEVEL4OUT	0b11110000
#define LEVEL5OUT	0b11100000


void init_led() {
	DDRC = 0xFF;    // LEDs - output
	PORTC = 0b11111111; // LEDs - off
}

void init_lcd() {
	lcd_init(LCD_DISP_ON);
	lcd_clrscr();
	lcd_puts(NAME_OF_BM);
	lcd_puts("\n");
	lcd_puts("LCD for debugging");
}

void init_uart() {
	uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) );
	sei();
}

void init_bluetooth(){
	uart_puts_P("$$$\r");
	uart_puts_P("+\r");
	char renameString[20];
	strcpy(renameString, "sn,");
	strcat(renameString, NAME_OF_BM);
	strcat(renameString, "\r");
	uart_puts(renameString);
	uart_puts_P("R,1\r");
}

// I don't think this works at all - however it seems needless anyway
// I think it's because of the '&' which wouldn't work used on a param instad of he original received value
bool isReceivedValueValid(unsigned int receivedValue) {
	char errorMsg[20];
	bool error = false;
	if ( receivedValue & UART_FRAME_ERROR ) {
		strcpy(errorMsg, "UART Frame Error");
		error = true;
    } 
	if ( receivedValue & UART_OVERRUN_ERROR ) {
        strcpy(errorMsg, "UART Overrun Error");
        error = true;
	} 
	if ( receivedValue & UART_BUFFER_OVERFLOW ) {
		strcpy(errorMsg, "Buffer overflow error");
		error = true;
    }
			
   	if (error) {
   		lcd_puts(errorMsg);
   		uart_puts(errorMsg);
   		return false;
   	} 
	return true;
}

void outputNewValue(unsigned char value) {
	//output on LCD
	char asciiNumberOfReceivedValue[3];
	itoa( value , asciiNumberOfReceivedValue, 10);
	lcd_clrscr();
	lcd_puts("Received: ");
	lcd_puts(asciiNumberOfReceivedValue);
	
	// output on LED
	//PORTC = value;
	
	// output on UART
	uart_putc(value);
}

void setLevel(char value) {
	lcd_gotoxy(0,1);
	lcd_puts("Level ");
	lcd_gotoxy(11,1);
	char* level;
	
	switch (value) {
		case LEVEL0IN:
			PORTC = LEVEL0OUT;
			level = "0";
			break;
		case LEVEL1IN:
			PORTC = LEVEL1OUT;
			level = "1";
			break;
		case LEVEL2IN:
			PORTC = LEVEL2OUT;
			level = "2";
			break;
		case LEVEL3IN:
			PORTC = LEVEL3OUT;
			level = "3";
			break;
		case LEVEL4IN:
			PORTC = LEVEL4OUT;
			level = "4";
			break;
		case LEVEL5IN:
			PORTC = LEVEL5OUT;
			level = "5";
			break;
		default:
			return;
	}
	lcd_puts(level);
	uart_puts(level);
}

int main(void)
{
	init_led();
	init_lcd();
	init_uart();
	init_bluetooth();
	
	unsigned int receivedValue;
	unsigned char value = 0b11111111;
	
    while (1) 
    {
        receivedValue = uart_getc();
		
		// '&' in C means 'address-of' - but what the hell does it mean here?
		if ( receivedValue & UART_NO_DATA) {
			// no data available from UART
		} else {
			isReceivedValueValid(receivedValue);
			
			// is the new input a new value?
			if (value != (unsigned char) receivedValue) {
				value = (unsigned char) receivedValue;
				outputNewValue(value);
				setLevel(value);
			}
		}
    }
}


