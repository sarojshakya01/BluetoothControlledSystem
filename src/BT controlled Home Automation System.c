/*
 * BT_controlled_Home_Automation_System.c
 *
 * Created: 11/1/2015 11:48:50 PM
 *  Author: User
 */ 


#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/interrupt.h>

#define OUTPUT_DDR DDRD
#define OUTPUT_PORT PORTD

#define LCD_DATA_PORT PORTB
#define LCD_DATA_DDR DDRB
#define LCD_RS 5
#define LCD_EN 4

unsigned char received_char,temperature_value, light_intensity;

void LCD_cmnd(unsigned char cmnd) //function to send command to LCD Module
{
	LCD_DATA_PORT = (LCD_DATA_PORT & 0xF0) | (cmnd >> 4); //send upper 4 bit
	LCD_DATA_PORT &= 0b11011111; //RS = 0
	LCD_DATA_PORT |= 0b00010000; //EN = 1
	_delay_us(100);
	LCD_DATA_PORT &= 0b11101111; //EN = 0
	_delay_us(300);
	LCD_DATA_PORT = (LCD_DATA_PORT & 0xF0) | (cmnd & 0x0F); //send lower 4 bit
	LCD_DATA_PORT |= 0b00010000; //EN = 1
	_delay_us(300);
	LCD_DATA_PORT &= 0b11101111; //EN = 0
}
void LCD_data(unsigned char data) //Function to send data to LCD Module
{
	LCD_DATA_PORT = (LCD_DATA_PORT & 0xF0) | (data >> 4); //send upper 4 bit
	LCD_DATA_PORT |= 0b00100000; //RS = 1
	LCD_DATA_PORT |= 0b00010000; //EN = 1
	_delay_us(200);
	LCD_DATA_PORT &= 0b11101111; //EN = 0
	_delay_us(200);
	LCD_DATA_PORT = (LCD_DATA_PORT & 0xF0) | (data & 0x0F); //send lower 4 bit
	LCD_DATA_PORT |= 0b00010000; //EN = 1
	_delay_us(200);
	LCD_DATA_PORT &= 0b11101111; //EN = 0
}
void LCD_initialize(void) //Function to initialize LCD Module
{
	LCD_DATA_DDR |= 0x3F;
	LCD_DATA_PORT &= 0b11101111;
	_delay_ms(200);
	LCD_cmnd(0x33);
	_delay_ms(20);
	LCD_cmnd(0x32);
	_delay_ms(20);
	LCD_cmnd(0x28);
	_delay_ms(20);
	LCD_cmnd(0x0E);
	_delay_ms(20);
	LCD_cmnd(0x01);
	_delay_ms(20);
}
void LCD_clear(void) //Function to clear the LCD Screen
{
	LCD_cmnd(0x01);
	_delay_ms(2);
}
void LCD_print(char * str) //Function to print the string on LCD Screen
{
	unsigned char i=0;
	while(str[i] != 0)
	{
		LCD_data(str[i]);
		i++;
		_delay_ms(5);
	}
}
void LCD_set_curser(unsigned char y, unsigned char x) //Function to move the Curser at (y,x) position
{
	if(y==1)
	LCD_cmnd(0x7F+x);
	else if(y==2)
	LCD_cmnd(0xBF+x);
}
void LCD_num(unsigned char num) //Function to display number
{
	//LCD_data(num/100 + 0x30);
	//num = num%100;
	LCD_data(num/10 + 0x30);
	LCD_data(num%10 + 0x30);
}
void output_port_initialize()
{
	OUTPUT_DDR = 0xFF;
}

void usart_initialize()//USART initialization//
{
	UCSRB = (1<<TXEN) | (1<<RXEN) | (1<<RXCIE); //enable tx and rx pin
	UCSRC = (1<<UCSZ0) | (1<<UCSZ1) | (1<<URSEL); //character size 8, 1 stop bit and reg select bit = 1
	UBRRL = 0x67; //baud rate 9600
}

void usart_send_char(unsigned char txdata)//Function to send single character serially//
{
	while(!(UCSRA&(1<<UDRE)));
	UDR = txdata;
}

void usart_send_string(char *str)//Function to send string serially//
{
	unsigned char i=0;
	while(str[i] != 0)
	{
		usart_send_char(str[i]);
		i++;
		_delay_ms(5);
	}
}

unsigned char usart_receive_char()
{
	while( !(UCSRA & (1<<RXC)) );
	return UDR;
}

void read_temperature()
{
	ADCSRA = 0x87; //Enable ADC and select clk/128
	ADMUX = 0xE0; //0b1110000, 11 for Vref=2.56, 1 for left justified,
	ADCSRA |= 1<<ADSC; //Start conversion in ADC
	while ((ADCSRA & (1<<ADIF)) == 0); //till the end of conversion
	ADCSRA |=(1<<ADIF);
	temperature_value = ADCH;
}

void read_light_intensity()
{
	ADCSRA = 0x87;
	ADMUX = 0xC1;
	ADCSRA |= 1<<ADSC;
	while ((ADCSRA & (1<<ADIF)) == 0);
	ADCSRA |=(1<<ADIF);
	light_intensity = ADC;
}

void fan_on()
{
	OUTPUT_PORT |= 0b00000100;
	usart_send_string("FAN ON");
	LCD_set_curser(2,1);
	LCD_print("   FAN ON");
	_delay_ms(2000);
}
void fan_off()
{
	OUTPUT_PORT &= 0b00001011;
	usart_send_string("FAN OFF");
	LCD_set_curser(2,1);
	LCD_print("   FAN OFF");
	_delay_ms(2000);
}
void light_on()
{
	OUTPUT_PORT |= 0b00001000;
	usart_send_string("LIGHT ON");
	LCD_set_curser(2,1);
	LCD_print("   LIGHT ON");
	_delay_ms(2000);
}
void light_off()
{
	OUTPUT_PORT &= 0b00000111;
	usart_send_string("LIGHT OFF");
	LCD_set_curser(2,1);
	LCD_print("   LIGHT OFF");
	_delay_ms(2000);
}
unsigned char mannual_on;
void automatic_mode()
{
	if(received_char == 'a')
	{
		sei();
		usart_send_string("System in auto mode!");
		LCD_set_curser(2,1);
		LCD_print("  Auto Mode");
		_delay_ms(2000);
		while(1)
		{
			if (mannual_on == 1) break;
			read_temperature();
			read_temperature();
			read_light_intensity();
			read_light_intensity();
			if (temperature_value > 25)
			{
				OUTPUT_PORT |= 0b00000100;
				//usart_send_string("FAN ON");
				LCD_clear();
				LCD_print("   FAN ON");
			}
			if (temperature_value < 24)
			{	
				OUTPUT_PORT &= 0b00001011;
				//usart_send_string("FAN OFF");
				LCD_clear();
				LCD_print("   FAN OFF");
			}
			if (light_intensity > 149)
			{
				OUTPUT_PORT |= 0b00001000;
				//usart_send_string("LIGHT ON");
				LCD_set_curser(2,1);
				LCD_print("   LIGHT ON");
			}
			if (light_intensity < 150)
			{
				OUTPUT_PORT &= 0b00000111;
				//usart_send_string("LIGHT OFF");
				LCD_set_curser(2,1);
				LCD_print("   LIGHT OFF");
			}
			_delay_ms(2000);
		}
		mannual_on = 0;
		usart_send_string("System in mannual mode!");
		LCD_set_curser(2,1);
		LCD_print("Mannual Mode");
		_delay_ms(2000);
	}
}

int main(void)
{
	mannual_on = 0;
	OUTPUT_DDR = 0xFC;
	output_port_initialize();
	LCD_initialize();
	LCD_clear();
	LCD_cmnd(0x0C);
	LCD_print("   hello!!!");
	LCD_set_curser(2,1);
	LCD_print("   Welcome!!!");
	_delay_ms(2000);
	usart_initialize();
	_delay_ms(10);
	sei();
	usart_send_string("Hello!!");
	LCD_clear();
	while(1)
	{
		read_temperature();
		read_temperature();
		read_light_intensity();
		read_light_intensity();
		LCD_print("Temp:");
		LCD_num(temperature_value);
		LCD_data(0xDF);
		LCD_data('C');
		_delay_ms(1000);
		LCD_clear();
		sei();
	}
	
}

ISR(USART_RXC_vect)
{
	received_char = UDR;
	if (received_char == 'a')
	{
		LCD_clear();
		LCD_print("Received char=");
		LCD_data(received_char);
		cli();
		automatic_mode();
	}
	if (received_char == 'A')
	{
		LCD_clear();
		LCD_print("Received char=");
		LCD_data(received_char);
		cli();
		mannual_on = 1;
		//automatic_mode();
	}
	if (received_char == 'f')
	{
		LCD_clear();
		LCD_print("Received char=");
		LCD_data(received_char);
		cli();
		fan_on();
	}
	if (received_char == 'l')
	{
		LCD_clear();
		LCD_print("Received char=");
		LCD_data(received_char);
		cli();
		light_on();
	}
	if (received_char == 'F')
	{
		LCD_clear();
		LCD_print("Received char=");
		LCD_data(received_char);
		cli();
		fan_off();
	}
	if (received_char == 'L')
	{
		LCD_clear();
		LCD_print("Received char=");
		LCD_data(received_char);
		light_off();
	}
	else
	{
		cli();
		sei();
	}
}
