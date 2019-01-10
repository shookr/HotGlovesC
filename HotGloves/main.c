/*
 * HotGloves.c
 *
 * Created: 14.11.2018
 * Author : Alex Schumann
 * Version : 0.5
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
#include <util/delay.h>

#define LCD_PORT	PORTC
#define NAME_OF_BM	"HotGlove"

#define LEVEL0IN	0b00110000	//ASCII 0
#define LEVEL1IN	0b00110001	//ASCII 1
#define LEVEL2IN	0b00110010	//ASCII 2
#define LEVEL3IN	0b00110011	//ASCII 3
#define LEVEL4IN	0b00110100	//ASCII 4
#define LEVEL5IN	0b00110101	//ASCII 5

#define MAXTEMP1 25
#define MAXTEMP2 35
#define MAXTEMP3 40
#define MAXTEMP4 45
#define MAXTEMP5 55

double dutyCycle = 0;
char* level;
int lvl;
int pwm;

//Function definitions
void init_lcd();
void init_uart();
void init_bluetooth();
void init_ADC();
void init_PWM();
bool isReceivedValueValid(unsigned int receivedValue);
void outputAdcAndVoltAndLevelAndPwmOnLcd();
void setLevel(char value);
void serialSend(char* sendString);
void debugInput();
int main(void);


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
	_delay_ms(500);
	uart_puts_P("$$$\r");
	uart_puts_P("+\r");
	_delay_ms(500);
	
	char renameString[20];
	strcpy(renameString, "sn,");
	strcat(renameString, NAME_OF_BM);
	strcat(renameString, "\r");
	uart_puts(renameString);
	_delay_ms(500);
	
	uart_puts_P("\rR,1\r");
	_delay_ms(500);
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

bool isReceivedValueValid(unsigned int receivedValue) {
	if ( receivedValue & UART_FRAME_ERROR || receivedValue & UART_OVERRUN_ERROR || receivedValue & UART_BUFFER_OVERFLOW ) {
		lcd_puts("UARTERR");
		return false;
	}
	return true;
}

void outputAdcAndVoltAndLevelAndPwmOnLcd() {	
	lcd_clrscr();
	
	lcd_gotoxy(0,0);
	lcd_puts("ADC ");
	char adcChar[16];
	itoa( ADC , adcChar, 10);
	lcd_puts(adcChar);
	
	lcd_gotoxy(8,0);
	float volt = ((((float)ADC)*4.0)/1024)*1000;
	lcd_puts("mV ");
	char voltChar[16];
	itoa( volt , voltChar, 10);
	lcd_puts(voltChar);
	
	lcd_gotoxy(0,1);
	lcd_puts("Level ");
	lcd_puts(level);
	
	lcd_gotoxy(8,1);
	lcd_puts("PWM ");
	char pwmChar[16];
	itoa( pwm , pwmChar, 10);
	lcd_puts(pwmChar);
}

void setLevel(char value) {	
	switch (value) {
		case LEVEL0IN:
			level = "0";
			lvl = 0;
			serialSend("L0L");
			break;
		case LEVEL1IN:
			level = "1";
			lvl = 1;
			serialSend("L1L");
			break;
		case LEVEL2IN:
			level = "2";
			lvl = 2;
			serialSend("L2L");
			break;
		case LEVEL3IN:
			level = "3";
			lvl = 3;
			serialSend("L3L");
			break;
		case LEVEL4IN:
			level = "4";
			lvl = 4;
			serialSend("L4L");
			break;
		case LEVEL5IN:
			level = "5";
			lvl = 5;
			serialSend("L5L");
			break;
		default:
			return;
	}
}

void serialSend(char* sendString){
	for (int i = 0; i < strlen(sendString); i++){
		while (( UCSR0A & (1<<UDRE0))  == 0){};
		UDR0 = sendString[i];
	}
}

void debugInput(){
	if(!(PINA & _BV(PINA2))) {
		setLevel(0b00110000);
		_delay_ms(500);
		serialSend("T12T");
	}
	if(!(PINA & _BV(PINA3))) {
		setLevel(0b00110001);
		_delay_ms(500);
		serialSend("T24T");
	}
	if(!(PINA & _BV(PINA4))) {
		setLevel(0b00110010);
		_delay_ms(500);
		serialSend("T35T");
	}
	if(!(PINA & _BV(PINA5))) {
		setLevel(0b00110011);
		_delay_ms(500);
		serialSend("B99B");
	}
	if(!(PINA & _BV(PINA6))) {
		setLevel(0b00110100);
		_delay_ms(500);
		serialSend("B78B");
	}
	if(!(PINA & _BV(PINA7))) {
		setLevel(0b00110101);
		_delay_ms(500);
		serialSend("B32B");
	}
}

int main(void) {
	DDRA = 0x00; // cheat
	
	init_lcd();
	init_uart();
	init_bluetooth();
	init_PWM();
	init_ADC();
		
	TCCR0B =  (1<<CS00); //| (1<<CS01) ;//(1<<CS02); 
	sei(); // set external interrupts - has to be done last
	
	unsigned int valueReceivedViaBT;
	unsigned char cachedValue = 0b11111111;
	
    while (1) {
	    debugInput();	// cheat
		
        valueReceivedViaBT = uart_getc();
		if ( valueReceivedViaBT & UART_NO_DATA) {
			// no data available from UART
		} else {
			isReceivedValueValid(valueReceivedViaBT); // no need to do that
			
			if (cachedValue != (unsigned char) valueReceivedViaBT) {
				cachedValue = (unsigned char) valueReceivedViaBT;
				setLevel(cachedValue);
			}
		}
		
		
		float volt = (((float)ADC)*4.0)/1024;
		float celcius = 701.41*volt*volt*volt - 2838.1*volt*volt + 3839*volt - 1700.1;
		int maxTempNotReached = 1;
		
		switch (lvl) {
			case 1:
				if ( celcius > MAXTEMP1 ) {
					maxTempNotReached = 0;
				} else {
					maxTempNotReached = 1;
				}
				break;
			case 2:
				if ( celcius > MAXTEMP2 ) {
					maxTempNotReached = 0;
					} else {
					maxTempNotReached = 1;
				}
				break;
			case 3:
				if ( celcius > MAXTEMP3 ) {
					maxTempNotReached = 0;
					} else {
					maxTempNotReached = 1;
				}
				break;
			case 4:
				if ( celcius > MAXTEMP4 ) {
					maxTempNotReached = 0;
					} else {
					maxTempNotReached = 1;
				}
				break;
			case 5:
				if ( celcius > MAXTEMP5 ) {
					maxTempNotReached = 0;
					} else {
					maxTempNotReached = 1;
				}
				break;
		}
		
		pwm = lvl * 51 * maxTempNotReached;
		
		OCR0A = pwm;	
		outputAdcAndVoltAndLevelAndPwmOnLcd();
    }
}

//Interrupts
ISR (TIMER0_OVF_vect) {

}

ISR (ADC_vect) {

}

