#define F_CPU 1000000
#define TEMP 1
#define LIGHT 2
#define PEOPLE 3
#define D4 eS_PORTD4
#define D5 eS_PORTD5
#define D6 eS_PORTD6
#define D7 eS_PORTD7
#define RS eS_PORTD2
#define EN eS_PORTD3

#include <avr/io.h>
#include <avr/interrupt.h>
#include "lcd.h"
#include <util/delay.h>
#include <stdio.h>



char intToChar(int x) {return x + '0';}
int index;
char arr1[10];

void doubleToString(int resultInt, char *output, int fromIndex, int type)
{
	int i = 0;
	
	if (type == TEMP || type == LIGHT)
	{
		if (resultInt < 0)
		{
			output[fromIndex++] = '-';
			
			if (resultInt > -10)
			{
				output[fromIndex++] = '0';
			}
			resultInt = -resultInt;
		}
		else {
			if (resultInt < 10) {
				
				output[fromIndex++] = '0';
				output[fromIndex++] = '0';
				if (resultInt == 0) output[fromIndex++] = '0';
				
			}
			else if (resultInt < 100) output[fromIndex++] = '0';
		}
		
		
	}
	else if (type == PEOPLE)
	{
		if (resultInt < 10) output[fromIndex++] = '0';
		if (resultInt == 0) output[fromIndex++] = '0';
	}
	
	while (resultInt > 0)
	{
		arr1[i++] = (resultInt % 10) + '0';
		resultInt /= 10;
		if (i > 10) break;
	}
	
	for (int j = i - 1; j >= 0; j--)
	{
		output[fromIndex++] = arr1[j];
		if (fromIndex > 10) break;
	}
	index = fromIndex;
	
}

void intToString(int val, char *output, int fromIndex)
{
	int i = 0;
	while (val > 0)
	{
		arr1[i++] = (val % 10) + '0';
		val /= 10;
	}
	
	for (int j = i - 1; j >= 0; j--)
	{
		output[fromIndex++] = arr1[j];
	}
	index = fromIndex;
}

volatile float temperature = 31, light = 50;
volatile float multiplier = (5.0 / 1024) * 100;///1000 (mv) / 10;
volatile unsigned int adc;
volatile unsigned char admux;
int humancount = 0, tempThreshold = 30, lightThreshold = 25;
volatile unsigned char state = 0;
volatile int n = 0;
volatile unsigned char x, keyInput = 0, tempAutomatic = 1, lightAutomatic = 1;
volatile unsigned char tempThresholdChange = 0, lightThresholdChange = 0, passwordGiven = 0;
volatile unsigned char overflowCount = 0, timeInterval = 0, doorOpenState = 0, startIndex = 0;
char output1[5];
char password[50] = "1234", enteredPassowrd[50], newPassword[50], validChars[20] = "0123456789/*-+=";
unsigned char enteredPassowrdLength = 0, newPasswordLength = 0, passwordLength = 4;
volatile unsigned char humanCountAdjust = 0, windowState = 0;


ISR(ADC_vect)
{
	adc = ADC;
	admux = ADMUX ;
	admux <<= 5;
	admux >>= 5;
	
	if (admux == 0)
	{
		temperature = adc * multiplier;
		ADMUX |= 1;
	}
	else if (admux == 1)
	{
		light = adc;
		ADMUX ^= 1;
		light = (1050 - light) / 10;
	}
	
	ADCSRA |= (1 << ADSC);
	
}

ISR(INT2_vect) {
	keyInput = 1;
}

ISR(TIMER1_OVF_vect)
{
	overflowCount++;
	if (overflowCount == 15)
	{
		overflowCount = 0;
		timeInterval++;
	}
}


void keyboardInput()
{
	char inputNumber = PINC & 0b00001111;
	keyInput = 0;
	
	if (inputNumber == 15)
	{
		x = 'z';
		if (tempThresholdChange)
		{
			Lcd4_Clear();
			Lcd4_Set_Cursor(1,1);
			Lcd4_Write_String("closing temp. threshold change mode");
			_delay_ms(30);
			show_LCD_default();
		}
		else if ( lightThresholdChange)
		{
			Lcd4_Clear();
			Lcd4_Set_Cursor(1,1);
			Lcd4_Write_String("closing light threshold change mode");
			_delay_ms(30);
			show_LCD_default();
		}
		else if (humanCountAdjust)
		{
			Lcd4_Clear();
			Lcd4_Set_Cursor(1,1);
			Lcd4_Write_String("Human count adjusted");
			_delay_ms(30);
			show_LCD_default();
		}
		else if (doorOpenState == 3 && passwordGiven == 0)
		{
			enteredPassowrd[startIndex++] = '\0';
			unsigned char passwordOk = 1;
			
			if (passwordLength != enteredPassowrdLength)
			{
				passwordOk = 0;
			}
			
			for (int i = 0; password[i]; i++)
			{
				if (password[i] != enteredPassowrd[i]) {
					passwordOk = 0;
					break;
				}
			}
			
			if (passwordOk)
			{
				passwordGiven = 1;
			}
			else
			{
				Lcd4_Clear();
				Lcd4_Set_Cursor(1,1);
				Lcd4_Write_String("Wrong Password!");
				_delay_ms(30);
				Lcd4_Clear();
				Lcd4_Set_Cursor(1,1);
				Lcd4_Write_String("Enter Password: ");
				startIndex = 0;
			}
		}
		
		lightThresholdChange = 0;
		tempThresholdChange = 0;
		humanCountAdjust = 0;
		enteredPassowrdLength = 0;
	}
	else if (inputNumber == 8)
	{
		
		
		x = '-';
		
		if (doorOpenState == 3 && passwordGiven == 0)
		{
			enteredPassowrd[startIndex++] = '-';
			enteredPassowrdLength++;
			Lcd4_Write_Char('*');
			return;
		}
		
		if (tempThresholdChange)
		{
			//Lcd4_Write_String("Temperature Threshold: ");
			tempThreshold--;
			
			if (tempThreshold < 0) tempThreshold = 0;
			
			Lcd4_Set_Cursor(1, 9);
			doubleToString(tempThreshold, output1, 0, TEMP);
			output1[index] = '\0';
			Lcd4_Write_String(output1);
			
		}
		else if (lightThresholdChange)
		{
			lightThreshold -= 5; 
			if (lightThreshold < 0) lightThreshold = 0;
			
			Lcd4_Set_Cursor(2, 9);
			doubleToString(lightThreshold, output1, 0, LIGHT);
			output1[index] = '\0';
			Lcd4_Write_String(output1);
		}
		else if (humanCountAdjust)
		{
			humancount--;
			Lcd4_Set_Cursor(1, 13);
			doubleToString(humancount, output1, 0, PEOPLE);
			output1[index] = '\0';
			Lcd4_Write_String(output1);
		}
	}
	else if (inputNumber == 12)
	{
		x = '+';
		
		if (doorOpenState == 3 && passwordGiven == 0)
		{
			enteredPassowrd[startIndex++] = '+';
			Lcd4_Write_Char('*');
			enteredPassowrdLength++;
			return;
		}
		
		if (tempThresholdChange)
		{
			//Lcd4_Write_String("Temperature Threshold: ");
			tempThreshold++;
			Lcd4_Set_Cursor(1, 9);
			doubleToString(tempThreshold, output1, 0, TEMP);
			output1[index] = '\0';
			Lcd4_Write_String(output1);
			
		}
		else if (lightThresholdChange)
		{
			lightThreshold+= 5;
			Lcd4_Set_Cursor(2, 9);
			doubleToString(lightThreshold, output1, 0, LIGHT);
			output1[index] = '\0';
			Lcd4_Write_String(output1);
		}
		else if (humanCountAdjust)
		{
			humancount++;
			Lcd4_Set_Cursor(1, 13);
			doubleToString(humancount, output1, 0, PEOPLE);
			output1[index] = '\0';
			Lcd4_Write_String(output1);
		}
	}
	
	else if (inputNumber == 14)
	{
		x = '0';
		
		if (doorOpenState == 3 && passwordGiven == 0)
		{
			enteredPassowrd[startIndex++] = '0';
			enteredPassowrdLength++;
			Lcd4_Write_Char('*');
			return;
		}
		
		Lcd4_Clear();
		Lcd4_Set_Cursor(1,1);
		
		if (tempAutomatic)
		{
			Lcd4_Write_String("Temperature Manual");
			tempAutomatic = 0;
		}
		else 
		{
			Lcd4_Write_String("Temperature Automatic");
			tempAutomatic = 1;
		}
		_delay_ms(30);
		
		show_LCD_default();
	}
	else if (inputNumber == 11)
	{
		x = '1';
		
		if (doorOpenState == 3 && passwordGiven == 0)
		{
			enteredPassowrd[startIndex++] = '1';
			Lcd4_Write_Char('*');
			enteredPassowrdLength++;
			return;
		}
		
		if (tempAutomatic == 0)
		{
			PORTB ^= 0b00000001;
		}
	}
	else if (inputNumber == 10)
	{
		x = '2';
		
		if (doorOpenState == 3 && passwordGiven == 0)
		{
			enteredPassowrd[startIndex++] = '2';
			enteredPassowrdLength++;
			Lcd4_Write_Char('*');
			return;
		}
		
		tempThresholdChange = 1;
		Lcd4_Clear();
		Lcd4_Set_Cursor(1,1);
		Lcd4_Write_String("+/- to change temperature threshold");
		_delay_ms(30);
		show_LCD_default();
	}
	else if (inputNumber == 9)
	{
		x = '3';
		
		if (doorOpenState == 3 && passwordGiven == 0)
		{
			enteredPassowrd[startIndex++] = '3';
			Lcd4_Write_Char('*');
			enteredPassowrdLength++;
			return;
		}
		
		Lcd4_Clear();
		Lcd4_Set_Cursor(1,1);
		
		if (lightAutomatic)
		{
			Lcd4_Write_String("Light Manual");
			lightAutomatic = 0;
		}
		else
		{
			Lcd4_Write_String("Light Automatic");
			lightAutomatic = 1;
		}
		_delay_ms(30);
		
		show_LCD_default();
	}
	else if (inputNumber == 7)
	{
		
		x = '4';
		
		if (doorOpenState == 3 && passwordGiven == 0)
		{
			enteredPassowrd[startIndex++] = '4';
			enteredPassowrdLength++;
			Lcd4_Write_Char('*');
			return;
		}
		
		if (lightAutomatic == 0)
		{
			PORTB ^= 0b00000010;
		}
	}
	else if (inputNumber == 6)
	{
		x = '5';
		
		if (doorOpenState == 3 && passwordGiven == 0)
		{
			enteredPassowrd[startIndex++] = '5';
			Lcd4_Write_Char('*');
			enteredPassowrdLength++;
			return;
		}
		
		lightThresholdChange = 1;
		Lcd4_Clear();
		Lcd4_Set_Cursor(1,1);
		Lcd4_Write_String("+/- to change light threshold");
		_delay_ms(30);
		show_LCD_default();
	}
	else if (inputNumber == 5)
	{
		if (doorOpenState == 3 && passwordGiven == 0)
		{
			enteredPassowrd[startIndex++] = '6';
			enteredPassowrdLength++;
			Lcd4_Write_Char('*');
			return;
		}
		x = '6';
		humanCountAdjust = 1;
		Lcd4_Clear();
		Lcd4_Set_Cursor(1,1);
		Lcd4_Write_String("+/- to adjust human count");
		_delay_ms(30);
		show_LCD_default();
	}
	else if (inputNumber ==3)
	{
		if (doorOpenState == 3 && passwordGiven == 0)
		{
			enteredPassowrdLength++;
			enteredPassowrd[startIndex++] = '7';
			Lcd4_Write_Char('*');
			return;
		}
		x = '7';
		
		if (windowState == 0)
		{
			windowState = 1;
		}
		else if (windowState == 2)
		{
			windowState = 3;
		}
	}
	else if (inputNumber == 2)
	{
		x = '8';
		if (doorOpenState == 3 && passwordGiven == 0)
		{
			enteredPassowrd[startIndex++] = '8';
			enteredPassowrdLength++;
			Lcd4_Write_Char('*');
			return;
		}
	}
	else if (inputNumber ==1)
	{
		x = '9';
		if (doorOpenState == 3 && passwordGiven == 0)
		{
			enteredPassowrd[startIndex++] = '9';
			enteredPassowrdLength++;
			Lcd4_Write_Char('*');
			return;
		}
	}
	
	else if (inputNumber == 4)
	{
		x = '*';
		if (doorOpenState == 3 && passwordGiven == 0)
		{
			enteredPassowrd[startIndex++] = '*';
			enteredPassowrdLength++;
			Lcd4_Write_Char('*');
			return;
		}
	}
	else if (inputNumber == 0)
	{
		x = '/';
		if (doorOpenState == 3 && passwordGiven == 0)
		{
			enteredPassowrd[startIndex++] = '/';
			enteredPassowrdLength++;
			Lcd4_Write_Char('*');
			return;
		}
	}
	else if (inputNumber == 13)
	{
		x = '=';
		if (doorOpenState == 3 && passwordGiven == 0)
		{
			enteredPassowrd[startIndex++] = '=';
			enteredPassowrdLength++;
			Lcd4_Write_Char('*');
			return;
		}
	}
	
	
}


void readPIRSensor()
{
	
	unsigned char inPIRreading = PINC & 0b01000000;
	unsigned char outPIRreading = PINC & 0b10000000;
	inPIRreading >>= 6;
	outPIRreading >>= 7;
	
	if (!(inPIRreading || outPIRreading))
	{
		state = 0;
		if (doorOpenState == 4) doorOpenState = 1;
		if (doorOpenState == 3) 
		{
			doorOpenState = 0;
			show_LCD_default();
		}
	}
	else if (outPIRreading == 1 && inPIRreading == 0)
	{
		if (state == 0)
		{
			doorOpenState = 3;
			passwordGiven = 0;
			startIndex = 0;
			Lcd4_Clear();
			Lcd4_Set_Cursor(1,1);
			Lcd4_Write_String("Enter Password: ");
		}
		state = 1;
		
		
	}
	else if (state == 1 && inPIRreading == 1 && doorOpenState == 4)
	{
		state = 2;
		humancount++;
		doorOpenState = 1;
	}
	else if (outPIRreading == 0 && inPIRreading == 1)
	{
		if (state == 0) doorOpenState = 2;
		state = 3;
	}
	else if (state == 3 && outPIRreading == 1 && doorOpenState == 4)
	{
		state = 4;
		humancount--;
		doorOpenState = 1;
	}
	
}

void doorControl()
{
	if (doorOpenState == 0)
	{
		PORTB &= 0b11001111;
	}
	else if (doorOpenState == 1)
	{
		PORTB &= 0b11001111;
		PORTB |= 0b00100000;
		timeInterval = 0;
		overflowCount = 0;
		while (timeInterval == 0){}
		PORTB &= 0b11001111;
		timeInterval = 0;
		doorOpenState = 0;
	}
	else if (doorOpenState == 2)
	{
		PORTB &= 0b11001111;
		PORTB |= 0b00010000;
		timeInterval = 0;
		overflowCount = 0;
		while (timeInterval == 0){}
		PORTB &= 0b11001111;
		timeInterval = 0;
		doorOpenState = 4;
	}
	else if (doorOpenState ==3 && passwordGiven == 1)
	{
		PORTB &= 0b11001111;
		PORTB |= 0b00010000;
		timeInterval = 0;
		overflowCount = 0;
		while (timeInterval == 0){}
		PORTB &= 0b11001111;
		timeInterval = 0;
		doorOpenState = 4;
		passwordGiven = 0;
		show_LCD_default();
	}
}

void windowControl()
{
	if (windowState == 1)
	{
		PORTB &= 0b00111111;
		PORTB |= 0b01000000;
		timeInterval = 0;
		overflowCount = 0;
		while (timeInterval == 0){}
		PORTB &= 0b00111111;
		windowState = 2;
	}
	else if (windowState == 3)
	{
		PORTB &= 0b00111111;
		PORTB |= 0b10000000;
		timeInterval = 0;
		overflowCount = 0;
		while (timeInterval == 0){}
		PORTB &= 0b00111111;
		windowState = 0;
	}
}

void show_LCD_default()
{
	Lcd4_Clear();
	
	Lcd4_Set_Cursor(1, 1);
	if (tempAutomatic) Lcd4_Write_String("T-A:");
	else Lcd4_Write_String("T-M:");
	
	Lcd4_Set_Cursor(1, 8);
	Lcd4_Write_String("-");
	Lcd4_Set_Cursor(1, 9);
	intToString(tempThreshold, output1, 0);
	output1[index] = '\0';
	Lcd4_Write_String(output1);
	
	Lcd4_Set_Cursor(2, 1);
	
	if (lightAutomatic) Lcd4_Write_String("L-A:");
	else (Lcd4_Write_String("L-M:"));
	
	Lcd4_Set_Cursor(2, 8);
	Lcd4_Write_String("-");
	Lcd4_Set_Cursor(2, 9);
	intToString(lightThreshold, output1, 0);
	output1[index] = '\0';
	Lcd4_Write_String(output1);
}

void USART_init()
{
	UCSRA = 0;
	UCSRA |= (1 << U2X);
	UCSRB = 0b00011000;
	UCSRB |= (1 << RXCIE);
	UCSRC = 0b10000110; 
	
	UBRRH = 0;//baud rate 9600
	UBRRL = 12;
}

volatile unsigned char inputFromUart = 0, uartData;
unsigned char fireHazard = 0, newPasswordState = 0;

ISR(USART_RXC_vect)
{
	inputFromUart = 1;
	uartData = UDR;
}

unsigned char uart_send(unsigned char data)
{
	while ((UCSRA & (1 << UDRE)) == 0){}
	UDR = data;
}

int isValidChar()
{
	for (int i = 0; validChars[i]; i++)
	{
		if (uartData == validChars[i]) return 1;
	}
	
	return 0;
}

void processUartInput()
{
	inputFromUart = 0;
	if ((uartData == 'p' || uartData == 'P') & newPasswordState == 0)
	{
		newPasswordState = 1;
		Lcd4_Clear();
		Lcd4_Set_Cursor(1, 1);
		Lcd4_Write_String("Password reset mode!");
		printf("\n\rNew Password: ");
		newPasswordLength = 0;
		
	}
	else if (uartData == '\n' || uartData == ' ' || uartData == '\r')
	{
		if (newPasswordState == 1)
		{
			if (newPasswordLength < 4)
			{
				printf("Password too short\n\r");
				show_LCD_default();
				newPasswordState = 0;
				return;
			}
			passwordLength = newPasswordLength;
			
			for (int i = 0; i < newPasswordLength; i++)
			{
				password[i] = newPassword[i];
			}
			
			show_LCD_default();
			printf("Password reset successful!\n\r");
			newPasswordState = 0;
		}
	}
	else if (newPasswordState && isValidChar() && newPasswordLength < 49)
	{
		newPassword[newPasswordLength++] = uartData;
	}
	else if (newPasswordState)
	{
		if (uartData == 'c' || uartData == 'C')
		{
			printf("\n\rPassword unchanged\n\r");
			newPasswordState = 0;
			show_LCD_default();
		}
	}
	else if (uartData == 'w' || uartData == 'W')
	{
		uartData = 'x';
		if (windowState == 0)
		{
			windowState = 1;
		}
		else if (windowState == 2)
		{
			windowState = 3;
		}
		printf("\n\r");
	}
	else if (uartData == '0')
	{
		printf("\n\r");
		Lcd4_Clear();
		Lcd4_Set_Cursor(1,1);
		
		if (tempAutomatic)
		{
			Lcd4_Write_String("Temperature Manual");
			tempAutomatic = 0;
		}
		else
		{
			Lcd4_Write_String("Temperature Automatic");
			tempAutomatic = 1;
		}
		_delay_ms(30);
		
		show_LCD_default();
	}
	else if (uartData == '1')
	{
		if (tempAutomatic == 0)
		{
			PORTB ^= 0b00000001;
		}
		printf("\n\r");
	}
	else if (uartData == '3')
	{
		printf("\n\r");
		
		Lcd4_Clear();
		Lcd4_Set_Cursor(1,1);
		
		if (lightAutomatic)
		{
			Lcd4_Write_String("Light Manual");
			lightAutomatic = 0;
		}
		else
		{
			Lcd4_Write_String("Light Automatic");
			lightAutomatic = 1;
		}
		_delay_ms(30);
		
		show_LCD_default();
	}
	else if (uartData == '4') 
	{
		printf("\n\r");	
		if (lightAutomatic == 0)
		{
			PORTB ^= 0b00000010;
		}
	}
	else if (uartData == 'd' || uartData == 'D')
	{
		printf("\n\r");
		if (doorOpenState == 4)
		{
			PORTB &= 0b11001111;
			PORTB |= 0b00100000;
			timeInterval = 0;
			overflowCount = 0;
			while (timeInterval == 0){}
			PORTB &= 0b11001111;
			timeInterval = 0;
			doorOpenState = 0;
		}
		else
		{
			doorOpenState = 4;	
			PORTB &= 0b11001111;
			PORTB |= 0b00010000;
			timeInterval = 0;
			overflowCount = 0;
			while (timeInterval == 0){}
			PORTB &= 0b11001111;
			timeInterval = 0;
			passwordGiven = 0;
			show_LCD_default();
		}
	}
}

int main(void)
{
	/* Replace with your application code */
	
	MCUCSR = (1<<JTD); 
	MCUCSR = (1<<JTD);

	
	DDRB = 0b11111011;
	DDRD = 0b11111100;
	DDRC = 0x00;
	ADMUX = 0b01000000;
	ADCSRA = 0b10001100;
	
	TCCR1A = 0b00000000;//normal mode
	TCCR1B = 0b00000001;//no prescalar, internal clock
	TIMSK = 0b00000100;//enable overflow interrupt
	
	GICR |= (1 << INT2);
	MCUCSR |= (1 << ISC2);
	
	USART_init();
	
	stdout = fdevopen(uart_send, NULL);
	
	Lcd4_Init();
	char tempout[5]="a";
	char output[5] = "n";
	
	sei();
	
	ADCSRA |= (1 << ADSC);
	
	show_LCD_default();
	
	while (1)
	{
		ADCSRA |= (1 << ADSC);
		
		
		if (doorOpenState != 3 && fireHazard == 0 && newPasswordState == 0)
		{
			doubleToString(temperature, tempout, 0, TEMP);
			tempout[index] = '\0';
			Lcd4_Set_Cursor(1, 5);
			Lcd4_Write_String(tempout);
		
		
		
			doubleToString(light, output, 0, LIGHT);
			output[index] = '\0';
			
			Lcd4_Set_Cursor(2,5);
			Lcd4_Write_String(output);
		
		}
		
		if (inputFromUart) processUartInput();
		
		readPIRSensor();
		doorControl();
		windowControl();
		
		if (doorOpenState != 3 && fireHazard == 0 && newPasswordState == 0)
		{
			doubleToString(humancount, output, 0, PEOPLE);
			output[index] = '\0';
			Lcd4_Set_Cursor(1, 13);
			Lcd4_Write_String(output);
		}
		
		
		if (keyInput)
		{
			keyboardInput();
		}
		
		if (humancount > 0 && tempAutomatic && temperature >= tempThreshold) PORTB |= 1;
		else if (tempAutomatic) PORTB &= 0b11111110;
		
		if (humancount > 0 && lightAutomatic && light <= lightThreshold) PORTB |= 0b00000010;
		else if (lightAutomatic) PORTB &= 0b11111101;
		
		if (temperature >= 100 || PINC & 16 || PINC & 32)
		{
			PORTB |= 8;
			if (fireHazard == 0)
			{
				printf("Fire Hazard Detected!\n\r");
				
				Lcd4_Clear();
				Lcd4_Set_Cursor(1,1);
				Lcd4_Write_String("Fire Hazard Detected!");
				fireHazard = 1;
			}
			
		} 
		else {
			PORTB &= 0b11110111;
			if (fireHazard)
			{
				show_LCD_default();
				printf("No fire hazard.\n\r");
			}
			fireHazard = 0;
		}
	}
}

