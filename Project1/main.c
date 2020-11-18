/*
 * main.c
 *
 * Created: 28/10/2020 
 * Author: Damian Larkin & James Cusack
 *
 * Program to satisfy ET4047 Project 1
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>

// adc threshold values
#define LOWER_THRESHOLD_VOLTAGE 511 // 2.5V
#define ONE_EIGHT_VOLTAGE 128
#define ONE_QUARTER_VOLTAGE 256
#define THREE_EIGHT_VOLTAGE 384
#define HALF_VOLTAGE 512
#define FIVE_EIGHT_VOLTAGE 639
#define THREE_QUARTER_VOLTAGE 767
#define SEVEN_EIGHT_VOLTAGE 895
#define MAX_THRESHOLD_VOLTAGE 1023 // 5V

// declare all global variables as volatile to avoid compiler optimization
volatile unsigned int timecount0; // number of overflows reached
volatile unsigned int time_overflow; // number of overflows needed
volatile int tcnt0_start; // counter start
volatile int adc_flag; // new adc result flag
volatile int display_flag; // check if portb bit 4 is pressed
volatile uint16_t adc_reading; // variable to hold adc reading

volatile int direction; // direction of cylon eyes travel
volatile int active_pin; // current active pin in cylon pattern


/********************************
* timer initialization function
*********************************/
void timer_init(void)
{
		timecount0 = 0; // initialize to 0
		tcnt0_start = 125; // begin timer count at 125
		time_overflow = 0; // initialize to 0
		
		TCCR0B = (5<<CS00);	// Set T0 Source = Clock (16MHz)/1024 and put Timer in Normal mode
		
		TCCR0A = 0;			// Not strictly necessary as these are the reset states
		
		TCNT0 = tcnt0_start;	// assign timer count start
		TIMSK0 = (1<<TOIE0);	// Enable Timer 0 interrupt
}

/********************************
* adc initialization function
*********************************/
void adc_init(void)
{
	// initialize global variables

	adc_flag = 0; // set if new adc result available
	display_flag = 1; // initialize to 8-bit thermometer display
	direction = 0; // start cylon eyes heading down (7->0)
	active_pin = 0; // start cylon eyes at bit 7
	
	// ADC initialization
	ADMUX = ((1<<REFS0) | (0 << ADLAR) | (0<<MUX0));  // AVCC selected for VREF, ADC0 as ADC input
	ADCSRA = ((1<<ADEN) | (1<<ADSC) | (1<<ADATE) | (1<<ADIE) | (6<<ADPS0)); /* Enable ADC, Start Conversion, Auto Trigger enabled, 
																		Interrupt enabled, Prescale = 64  */
	ADCSRB = (0<<ADTS0); // Select AutoTrigger Source to Free Running Mode
}

/********************************
* main initialization function 
*********************************/
void port_init(void)
{
	DDRD = 0xff;	// 0xff = 0b11111111; all ones
	PORTD = 0;		// set PORTD pins to 0
	DDRB = 0b00000000; // set PORTB to inputs
	PORTB = 0b00110000; // enable pull up resistors on pins 4 & 5
	
	sei();				// Global interrupt enable
}

/**************************************************************************************************
* looping cylon pattern function
*
* function works by using the direction variable to indicate which way the led's should be moving.
* the pattern starts at bit 7 and works its way down. It knows it has reached the end when the 
* currently active led = end, which is passed into the function from the ISR.
*
* int end: variable to indicate at which led the function should end, passed in from the ISR.
*		  (0, meaning full display; 4, meaning half display)
***************************************************************************************************/
void cylon_loop(int end)
{
	// down
	if (direction == 0) // check if direction is down
	{
		active_pin--; // decrement the active led
		if (active_pin <= end) // check if led has reached the end (pin 4 or 0)
		{
			direction = 1; // set direction to up
		}
		PORTD = 0b00000001 << active_pin; // set pin of portd to current led
		
		// up
	} else if (direction == 1) // check if direction is up
	{
		active_pin++; // increment the active led
		if (active_pin >= 7) // check if led has reached end (pin 7)
		{
			direction = 0; // set direction to down
		}
		PORTD = 0b00000001 << active_pin; // set pin of portd to current led
	}
}


/***********************************************
* adc thermometer display
*
* int display_flag: sets display to full 8-bit, 
*				or half 4-bit mode 
***********************************************/
void adc_display(int display_flag)
{
	if (display_flag) // full 8-bit display, this takes control of PORTD
	{
		// test adc value and alter pins
		if (adc_reading <= ONE_EIGHT_VOLTAGE)
		{
			PORTD = 0b00000000;
		} else if (adc_reading <= ONE_QUARTER_VOLTAGE)
		{
			PORTD = 0b00000001;
		} else if (adc_reading <= THREE_EIGHT_VOLTAGE)
		{
			PORTD = 0b00000011;
		} else if (adc_reading <= HALF_VOLTAGE)
		{
			PORTD = 0b00000111;
		} else if (adc_reading <= FIVE_EIGHT_VOLTAGE)
		{
			PORTD = 0b00001111;
		} else if (adc_reading <= THREE_QUARTER_VOLTAGE)
		{
			PORTD = 0b00011111;
		} else if (adc_reading <= SEVEN_EIGHT_VOLTAGE)
		{
			PORTD = 0b00111111;
		} else if (adc_reading < MAX_THRESHOLD_VOLTAGE)
		{
			PORTD = 0b01111111;
		} else if (adc_reading == MAX_THRESHOLD_VOLTAGE)
		{
			PORTD = 0b11111111;
		}
	} else if (!display_flag) // half 4-bit display, this only alters bits 0-3
	{
		PORTD &= ~0b00001111; // clear bits 0-3
		// test adc value and alter pins
		if (adc_reading <= ONE_QUARTER_VOLTAGE)
		{
			PORTD |= 0b00000000;
		} else if (adc_reading <= HALF_VOLTAGE)
		{
			PORTD |= 0b00000001;
		} else if (adc_reading <= THREE_QUARTER_VOLTAGE)
		{
			PORTD |= 0b00000011;
		} else if (adc_reading < MAX_THRESHOLD_VOLTAGE)
		{
			PORTD |= 0b00000111;
		} else if (adc_reading == MAX_THRESHOLD_VOLTAGE)
		{
			PORTD |= 0b00001111;
		}
	}
}

/****************************************************
* main function
*
* checks if buttons (PINB5 & PINB4) are pressed, 
* and sets adc display to the appropriate mode
****************************************************/
int main(void)
{
	timer_init(); // call initialisation for timer registers/variables
	adc_init(); // call initialisation for adc registers/variables
	port_init(); // call initialisation for port registers
    while(1)
	{
		if (adc_flag) // check new adc result available
		{
			if ((PINB & 0b00100000) == 0b00100000) // check PINB bit 5 is 1 (not pressed)
			{
				if ((PINB & 0b00010000) == 0) // check PINB bit 4 is 0 (pressed)
				{
					adc_display(display_flag = 1); // set to full 8-bit mode
					adc_flag = 0; // reset
				}
			} else if ((PINB & 0b00100000) == 0) // check if PINB bit 5 is 0 (pressed)
			{
				adc_display(display_flag = 0); // set to half 4-bit mode
				adc_flag = 0;
			}
		}
	}
}

/******************************************************
* Timer0 overflow vector ISR
*
* controls the time delay for the cylon pattern &
* checks if the cylon should be in 8 or 4-bit mode
******************************************************/
ISR(TIMER0_OVF_vect) // timer0 ISR
{
	TCNT0 = tcnt0_start; // set to start value based on 0.125s or 0.5s 
	++timecount0; // count the number of times the overflow has been reached
	
	if (timecount0 >= time_overflow) // check if amount of overflows equals adc setting
	{
		if ((PINB & 0b00100000) == 0b00100000) // check PINB bit 5 is 1 (not pressed)
		{
			if ((PINB & 0b00010000) == 0b00010000) // check PINB bit 4 is 1 (not pressed)
			{
				cylon_loop(0); // start 8-bit cylon pattern
				timecount0 = 0;	// Restart the overflow counter
			}
		} else {
			cylon_loop(4); // start 4-bit cylon pattern
			timecount0 = 0;
		}
	}
}
/******************************************************************
* ADC vector ISR
*
* controls the timer0 start & overflow for the cylon pattern &
* saves the adc value for the adc thermometer display
******************************************************************/
ISR (ADC_vect) // adc ISR
{
	
	adc_reading = ADC; // store current adc value in variable
	adc_flag = 1; // set flag
	
	if (adc_reading < LOWER_THRESHOLD_VOLTAGE) // check adc voltage is between 0V-2.5V
	{
		// 0.125s delay
		tcnt0_start = 39; // for 0.125s delay we start the timer count at 39
		time_overflow = 9; // for 0.125s delay we want 9 overflows to trigger an interrupt
		
	} else if (adc_reading < MAX_THRESHOLD_VOLTAGE) // otherwise if adc voltage is between 2.5V-5V
	{
		// 0.5s delay
		tcnt0_start = 142; // for 0.5s delay we start the timer count at 142
		time_overflow = 55; // for 0.5s delay we want 55 overflows to trigger an interrupt
	}
}