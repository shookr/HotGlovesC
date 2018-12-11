/*
 * HotGloves.c
 *
 * Created: 14.11.2018
 * Author : Alex Schumann
 * Version : 0.3
 */ 

#define F_CPU	3686400UL	// also manually defined in delay.h
#define UART_BAUD_RATE	9600	//115200

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


#define LCD_PORT	PORTC
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

double dutyCycle = 0;
int lvl;

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

void init_ADC() {
	ADMUX |= (1<<REFS0) | (1<<MUX0);
	ADCSRA |= (1<<ADEN) | (1<<ADSC) | (1<<ADATE) | (1<<ADIF) | (1<<ADIE) | (1<<ADPS2) |  (1<<ADPS1);
}

void init_PWM() {
	DDRB = 0xFF; // output
	TCCR0A = (1<<COM0A1) | (1<<WGM00) | (1<<WGM01);
	TIMSK0 = (1<<TOIE0);
	OCR0A = (dutyCycle/100) * 255.0;
}

// I don't think this works at all - however it seems to be needless anyway
// I think it's because of the '&' which wouldn't work used on a param instead of the original received value
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

void outputADC() {	
	lcd_gotoxy(0,0);
	lcd_puts("ADC ");
	char value[16];
	itoa( ADC , value, 10);
	lcd_puts(value);
}

void setLevel(char value) {
 	lcd_gotoxy(0,1);
 	lcd_puts("Level ");
 	lcd_gotoxy(6,1);
	char* level;
	
	switch (value) {
		case LEVEL0IN:
			level = "0";
			lvl = 0;
			break;
		case LEVEL1IN:
			level = "1";
			lvl = 1;
			break;
		case LEVEL2IN:
			level = "2";
			lvl = 2;
			break;
		case LEVEL3IN:
			level = "3";
			lvl = 3;
			break;
		case LEVEL4IN:
			level = "4";
			lvl = 4;
			break;
		case LEVEL5IN:
			level = "5";
			lvl = 5;
			break;
		default:
			return;
	}
	lcd_puts(level);
	uart_puts(level);
}

void debugInput(){
	if(!(PINA & _BV(PINA2))) {
		setLevel(0b00110000);
	}
	if(!(PINA & _BV(PINA3))) {
		setLevel(0b00110001);
	}
	if(!(PINA & _BV(PINA4))) {
		setLevel(0b00110010);
	}
	if(!(PINA & _BV(PINA5))) {
		setLevel(0b00110011);
	}
	if(!(PINA & _BV(PINA6))) {
		setLevel(0b00110100);
	}
	if(!(PINA & _BV(PINA7))) {
		setLevel(0b00110101);
	}
}

int main(void)
{
	DDRA = 0x00; // quickinput for debugging
	//init_led();
	init_lcd();
	init_uart();
	init_bluetooth();
	init_PWM();
	init_ADC();
		
	TCCR0B =  (1<<CS00); //| (1<<CS01) ;//(1<<CS02); //starts timer?
	sei(); // set external interrupts
		
	OCR0A = 128;
	
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
				setLevel(value);
			}
		}
		
		debugInput();
		
		OCR0A = lvl * 51;	
		
		outputADC();
    }
}

ISR (TIMER0_OVF_vect) {
 		
}

ISR (ADC_vect) {
 
}

