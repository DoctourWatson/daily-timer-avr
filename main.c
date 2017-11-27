/*
 * main.c
 *
 *  Created on: 15.01.2017
 *      Author: drwatson
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>

// Display
#define LED_a 0x40
#define LED_b 0x20
#define LED_c 0x01
#define LED_d 0x04
#define LED_e 0x02
#define LED_f 0x10
#define LED_g 0x08
#define LED_h 0x80

#define LED_SP 0
#define LED_0 (LED_a|LED_b|LED_c|LED_d|LED_e|LED_f)
#define LED_1 (LED_b|LED_c)
#define LED_2 (LED_a|LED_b|LED_d|LED_e|LED_g)
#define LED_3 (LED_a|LED_b|LED_c|LED_d|LED_g)
#define LED_4 (LED_b|LED_c|LED_f|LED_g)
#define LED_5 (LED_a|LED_c|LED_d|LED_f|LED_g)
#define LED_6 (LED_a|LED_c|LED_d|LED_e|LED_f|LED_g)
#define LED_7 (LED_a|LED_b|LED_c)
#define LED_8 (LED_a|LED_b|LED_c|LED_d|LED_e|LED_f|LED_g)
#define LED_9 (LED_a|LED_b|LED_c|LED_d|LED_f|LED_g)
#define LED_A (LED_a|LED_b|LED_c|LED_e|LED_g|LED_g)
#define LED_B (LED_c|LED_d|LED_e|LED_f|LED_g)
#define LED_C (LED_a|LED_d|LED_e|LED_f)
#define LED_D (LED_b|LED_c|LED_d|LED_e|LED_g)
#define LED_E (LED_a|LED_d|LED_e|LED_f|LED_g)
#define LED_F (LED_a|LED_e|LED_f|LED_g)

volatile uint8_t LedBuffer[3], FlashBuffer[3];

#define SEGMENT_MASK 0x7F
#define DIGIT_OFF 0x00
#define DIGIT_MASK 0x07
#define FLASH_BIT_MASK 0x01

const uint8_t DigitLine[3] = {0x01, 0x02, 0x04};

volatile uint8_t DispDigit, FlashCounter;

const uint8_t Digits[15] = {LED_0, LED_1, LED_2, LED_3, LED_4, LED_5, LED_6, LED_7, LED_8,
	LED_9, LED_A, LED_B, LED_C, LED_E, LED_F};

#define SetDigit(x) PORTB = (PORTB & (~(DIGIT_MASK))) | ((x) & DIGIT_MASK)
#define SetSegment(x) PORTA = (PORTA & (~(SEGMENT_MASK))) | ((x) & SEGMENT_MASK)

// Buttons
#define SECPARTMAX 2

#define BUTTONLONG 0x01

#define BLUEBUTTON 0x80
#define BLUEBUTTONLONG 0x20
#define REDBUTTON 0x40
#define REDBUTTONLONG 0x10

volatile uint8_t LastKey, PrevKey, KeyCntr;

// Timer
volatile uint8_t Hour, Min, Sec, HourNew, DayNew, Timeout;


EEMEM uint8_t eTimerState, eHoursOn, eDays;

void InitPeriph(void);
uint8_t RedButton(void);
uint8_t RedButtonLong(void);
uint8_t BlueButton(void);
uint8_t BlueButtonLong(void);
void ShowDays(void);
void SetTimer(void);
void ShowNum(uint8_t num);

#define ENABLE_INT sei()
#define DISABLE_INT cli()
#define SLEEP sleep_mode()

#define LoadOn() PORTB |= (1<<3)
#define LoadOff() PORTB &= ~(1<<3)

// #define ReadEEPROM(x) eeprom_read_byte(x)
// #define WriteEEPROM(x,y) eeprom_write_byte(x,y)



int main(void)
{
	uint8_t hours_on;

	PORTA = 0x80;
	DDRA = 0x7F;

	PORTB = 0x40;
	DDRB = 0x0F;

	TCCR0 = 0x03; // Prescaler = 64

	OCR1C = 249; // Divide 250
	TCCR1A = 0x00;
	TCCR1B = 0x8E; // Prescaler = 8192

	TIMSK = 0x06; // Interrupts T0, T1

	MCUCR = 0x00;

	//DayNew = 0;
	//Hour = 0;
	//Min = 0;
	//Sec = 0;
	//SecPart = 0;
	HourNew = 1;
	if(eeprom_read_byte(&eHoursOn) > 24)
	{
		eeprom_write_byte(&eHoursOn, 20);
		eeprom_write_byte(&eTimerState, 0);
		eeprom_write_byte(&eDays, 0);
	}
	hours_on = eeprom_read_byte(&eHoursOn);
	ENABLE_INT;
	while(1)
	{
		if(eeprom_read_byte(&eTimerState))
		{
			while(!RedButtonLong())
			{
				if(RedButton())
				{
					Hour++;
					if(Hour >= 24) Hour = 0;
					HourNew = 1;

				}
				if(HourNew)
				{
					if(DayNew)
					{
						DayNew = 0;
						eeprom_write_byte(&eDays, eeprom_read_byte(&eDays)+1);
					}
					if(Hour < hours_on)
					{
						LoadOn();
						ShowNum(hours_on - Hour);
						LedBuffer[0] = LED_b;
					}
					else
					{
						LoadOff();
						ShowNum(24 - Hour);
						LedBuffer[0] = LED_c;
					}
					FlashBuffer[0] = LED_1;
					HourNew = 0;
				}
				SLEEP;
				if(BlueButton())
				{
					ShowDays();
					HourNew = 1;
				}
				if(BlueButtonLong())
				{
					SetTimer();
					hours_on = eeprom_read_byte(&eHoursOn);
					HourNew = 1;
				}
			}
			eeprom_write_byte(&eTimerState, 0);
		}
		ShowNum(hours_on);
		LoadOff();
		while(!RedButtonLong())
		{
			SLEEP;
			if(BlueButton())
			{
				ShowDays();
				ShowNum(hours_on);
			}
			if(BlueButtonLong())
			{
				SetTimer();
				hours_on = eeprom_read_byte(&eHoursOn);
				ShowNum(hours_on);
			}
		}
		DISABLE_INT;
		Hour = 0;
		Min = 0;
		Sec = 0;
		//SecPart = 0;
		eeprom_write_byte(&eTimerState, 1);
		HourNew = 1;
		ENABLE_INT;
	}

}

uint8_t RedButton(void)
{
	if(LastKey == REDBUTTON)
	{
		LastKey = 0x00;
		return 1;
	}
	return 0;

}

uint8_t RedButtonLong(void)
{
	if(LastKey == REDBUTTONLONG)
	{
		LastKey = 0x00;
		return 1;
	}
	return 0;

}

uint8_t BlueButton(void)
{
	if(LastKey == BLUEBUTTON)
	{
		LastKey = 0x00;
		return 1;
	}
	return 0;
}

uint8_t BlueButtonLong(void)
{
	if(LastKey == BLUEBUTTONLONG)
	{
		LastKey = 0x00;
		return 1;
	}
	return 0;
}

void ShowDays(void)
{
	uint8_t d;
	d = eeprom_read_byte(&eDays);
	ShowNum(d);
	if(d < 100)
	{
		LedBuffer[0] = LED_1;
		FlashBuffer[0] = LED_1;
	}
	else FlashBuffer[0] = 0;
	Timeout = 10;
	while(Timeout && (!BlueButton()))
	{
		SLEEP;
		if(BlueButtonLong() && (d>0))
		{
			d = 0;
			Timeout = 10;
			eeprom_write_byte(&eDays, 0);
			ShowNum(0);
		}
	}
}

void SetTimer(void)
{
	uint8_t time, state;
	time = eeprom_read_byte(&eHoursOn);
	FlashBuffer[1] = LED_8;
	FlashBuffer[2] = LED_8;
	FlashBuffer[0] = LED_1;
	ShowNum(time);
	if(eeprom_read_byte(&eTimerState))
	{
		if(Hour < time) state = LED_b;
		else state = LED_c;
	}
	else state = 0;
	LedBuffer[0] = state;
	Timeout = 10;
	while(Timeout)
	{
		SLEEP;
		if(BlueButton())
		{
			time++;
			if(time > 24) time = 0;
			Timeout = 10;
			ShowNum(time);
			LedBuffer[0] = state;
		}
		if(BlueButtonLong())
		{
			eeprom_write_byte(&eHoursOn, time);
			break;
		}
	}
	FlashBuffer[1] = 0;
	FlashBuffer[2] = 0;
}

void ShowNum(uint8_t num)
{
	if(num > 99)
	{
		LedBuffer[0] = Digits[num/100];
		num %= 100;
	}
	else LedBuffer[0] = LED_SP;
	if(num > 9)
	{
		LedBuffer[1] = Digits[num/10];
		num %= 10;
	}
	else LedBuffer[1] = LED_SP;
	LedBuffer[2] = Digits[num];
}

ISR(TIMER1_OVF1_vect)
//void TimerTime(void)
{
	FlashCounter++;
	if(FlashCounter >= SECPARTMAX)
	{
		Sec++;
		FlashCounter = 0;
		if(Timeout) Timeout--;
		if(Sec >= 60)
		{
			Sec = 0;
			Min++;
			if(Min >= 60)
			{
				Min = 0;
				Hour++;
				HourNew = 1;
				if(Hour >= 24)
				{
					Hour = 0;
					DayNew = 1;
				}
			}
		}
	}
}

//void INTERRUPT DisplayInt(void)
ISR(TIMER0_OVF0_vect)
{
	uint8_t key;
	SetDigit(DIGIT_OFF);
	if(FlashCounter & FLASH_BIT_MASK)
	{
		SetSegment(LedBuffer[DispDigit] & (~FlashBuffer[DispDigit]));

	}
	else SetSegment(LedBuffer[DispDigit]);
	SetDigit(DigitLine[DispDigit]);
	DispDigit++;
	if(DispDigit >= 3)
	{
		DispDigit = 0;

		// Scan Buttons
		key = ((PINA & 0x80) | (PINB & 0x40)) ^ 0xC0;
		if(key)
		{
			if(key == PrevKey)
			{
				if(KeyCntr < 240)
				{
					KeyCntr++;
				}
				if(KeyCntr == 240)
				{
					LastKey = (key >> 2);
					KeyCntr++;
				}
			}
			else
			{
				KeyCntr = 0;
			}
		}
		else
		{
			if((KeyCntr > 8) && (KeyCntr < 240))
			{
				LastKey = PrevKey;
			}
			KeyCntr = 0;
		}
		PrevKey = key;
	}
}
