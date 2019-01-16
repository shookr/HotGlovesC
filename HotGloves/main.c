/*
 * HotGloves.c
 *
 * Created: 14.11.2018
 * Author : Alex Schumann
 * Version : 0.6
 */ 

#define F_CPU	3686400UL	// also manually defined in delay.h
#define UART_BAUD_RATE	/*9600*/	115200

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <uart.h>
#include <lcd.h>
#include <stdbool.h>
#include <string.h>
#include <util/delay.h>
#include <ctype.h>

#define LCD_PORT	PORTC
#define NAME_OF_BM	"HotGloveDev"

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

const float refVolt = 3.3;

double dutyCycle = 0;
char* level;
int lvl;
int pwm;
float celcius;
float voltTemp;

//Function definitions
void init_lcd();
void init_uart();
void init_bluetooth();
void init_ADC();
void init_PWM();
bool isReceivedValueValid(unsigned int receivedValue);
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
	_delay_ms(100);
	
	char renameString[20];
	strcpy(renameString, "sn,");
	strcat(renameString, NAME_OF_BM);
	strcat(renameString, "\r");
	uart_puts(renameString);
	_delay_ms(100);
	
	uart_puts_P("\rR,1\r");
	_delay_ms(500);
}

void init_ADC() {
	ADCSRA |= 1<<ADPS2;
	ADMUX |= 1<<REFS0 | 1<<REFS1;
	ADCSRA |= 1<<ADIE;
	ADCSRA |= 1<<ADEN;
}

void init_PWM() {
	DDRB = 0xFF; // output
	TCCR0A = (1<<COM0A1) | (1<<WGM00) | (1<<WGM01);
	TIMSK0 = (1<<TOIE0);
	OCR0A = (dutyCycle/100) * 255.0;
}

bool isReceivedValueValid(unsigned int receivedValue) {
	if ( receivedValue & UART_FRAME_ERROR || receivedValue & UART_OVERRUN_ERROR || receivedValue & UART_BUFFER_OVERFLOW ) {
		return false;
	}
	return true;
}

void setLevel(char value) {	
	switch (value) {
		case LEVEL0IN:
			level = "0";
			lvl = 0;
			//serialSend("L0L");
			break;
		case LEVEL1IN:
			level = "1";
			lvl = 1;
			//serialSend("L1L");
			break;
		case LEVEL2IN:
			level = "2";
			lvl = 2;
			//serialSend("L2L");
			break;
		case LEVEL3IN:
			level = "3";
			lvl = 3;
			//serialSend("L3L");
			break;
		case LEVEL4IN:
			level = "4";
			lvl = 4;
			//serialSend("L4L");
			break;
		case LEVEL5IN:
			level = "5";
			lvl = 5;
			//serialSend("L5L");
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
 	if(!(PINA & _BV(PINA6))) {
		setLevel(0b00110000);
 	}
 	if(!(PINA & _BV(PINA7))) {
 		setLevel(0b00110101);
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
	
	ADCSRA |= 1<<ADSC;
	
	unsigned int valueReceivedViaBT = 0b11111111;
	unsigned int valueReceivedCache = 0b11111111;
	setLevel(0b00110000);
	lcd_clrscr();
	
	_delay_ms(2000);
	
	sei(); // set external interrupts - has to be done last
	// ------------------------------
	// Everything beyond processes very very very slow
	// ------------------------------
	_delay_ms(10); // wait for several seconds...
	
    while (1) {
	    debugInput();	// cheat
		
        valueReceivedViaBT = uart_getc();
		if ( valueReceivedViaBT & UART_NO_DATA) {
			// no data available from UART
		} else {
			if (isReceivedValueValid(valueReceivedViaBT)) {
					if (valueReceivedCache != valueReceivedViaBT) {
						valueReceivedCache = valueReceivedViaBT;
						setLevel(valueReceivedViaBT);
					}
				}
		}
		
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
    }
}

//Interrupts
ISR (TIMER0_OVF_vect) {

}


const int counterRange = 2000;
int counter = 0;
int counter2 = 1000;
ISR (ADC_vect) {	
	switch (ADMUX) {
		case 0xC1:
			lcd_gotoxy(0,0);
			lcd_puts("ADC1 ");
			char adcChar[16];
			itoa( ADC, adcChar, 10);
			lcd_puts(adcChar);
			
			voltTemp = ((((float)ADC)*refVolt)/1024)*1000;
			lcd_gotoxy(9,0);
			lcd_puts("mV ");
			char voltChar[16];
			itoa( voltTemp , voltChar, 10);
			lcd_puts(voltChar);
			
			if (counter == counterRange) {
				float v = voltTemp/1000;
				celcius = 701.41*v*v*v - 2838.1*v*v + 3839*v - 1700.1;
			
				char t[10] = "T"; 
				char tempChar[10];
				itoa( celcius , tempChar, 10);
				strncat(t, tempChar, 10);
				strcat(t, "T");
				serialSend(t);
				counter = 0;
			} else {
				counter = counter + 1;
			}
			
			ADMUX = 0xC0;
			break;
		case 0xC0:
			lcd_gotoxy(0,1);
 			lcd_puts("ADC0 ");
 			char adc0Char[16];
			itoa( ADC, adc0Char, 10);
			lcd_puts(adc0Char);
					
			if (counter2 == counterRange) {
				float voltBatt = ((((float)ADC)*refVolt)/1024)*1000;
				float battLevel = (voltBatt - (3700/2)) * 0.2;
				
				char b[10] = "B";
				char battChar[10];
				itoa( battLevel , battChar, 10);
				strncat(b, battChar, 10);
				strcat(b, "B");
				serialSend(b);
				
				_delay_ms(500);
				char l[10] = "L";
				char levelChar[10];
				itoa( lvl , levelChar, 10);
				strncat(l, levelChar, 10);
				strcat(l, "L");
				serialSend(l);
				
				counter2 = 0;
			} else {
				counter2 = counter2 + 1;
			}
			
			ADMUX = 0xC1;
			break;
		default:
			break;
			//Default code
	}
	
	if (counter % 20 == 0) {
		lcd_clrscr();
		
		lcd_gotoxy(9,1);
		lcd_puts("L");
		lcd_puts(level);
		
		lcd_gotoxy(12,1);
		lcd_puts("P");
		char pwmChar[16];
		itoa( pwm , pwmChar, 10);
		lcd_puts(pwmChar);
	}
	
	ADCSRA |= 1<<ADSC;
}

