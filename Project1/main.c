/*
 * main.c
 *
 * Created: 28/10/2020 
 *  Author: Damian
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>

#define LOWER_THRESHOLD_VOLTAGE 511
#define ONE_EIGHT_VOLTAGE 128
#define ONE_QUARTER_VOLTAGE 256
#define THREE_EIGHT_VOLTAGE 384
#define HALF_VOLTAGE 512
#define FIVE_EIGHT_VOLTAGE 639
#define THREE_QUARTER_VOLTAGE 767
#define SEVEN_EIGHT_VOLTAGE 895
#define UPPER_THRESHOLD_VOLTAGE 1023

// declare all global variables as volatile to avoid compiler optimization
volatile unsigned int timecount0; // number of overflows
volatile int time_delay; // number of overflows needed
volatile int tcnt0_start; // counter start variable
volatile int adc_flag; // new adc result flag
volatile int display_flag; // check if portb bit 4 is pressed
volatile uint16_t adc_reading; // variable to hold adc reading

volatile unsigned int direction; // direction of cylon eyes travel
volatile unsigned int active_led; // current active led in cylon pattern

/***************************
*initialization function 
***************************/
void init(void)
{
	//Set PortD to all outputs because LEDs are connected to this PORT
	DDRD = 0xff;	// 0xff = 0b11111111; all ones
	PORTD = 0;		// 
	DDRB = 0b00000000; // set PORTB to inputs
	PORTB = 0b00110000; // enable pull up resistors on pins 4 & 5
	
	// initialize global variables
	timecount0 = 0;
	adc_flag = 0;
	display_flag = 1;
	tcnt0_start = 125;
	time_delay = 0;
	direction = 0; // start cylon eyes heading down (7-0)
	active_led = 7; // start cylon eyes at bit 7
	
	TCCR0B = (5<<CS00);	// Set T0 Source = Clock (16MHz)/1024 and put Timer in Normal mode
	
	TCCR0A = 0;			// Not strictly necessary as these are the reset states but it's good
	// practice to show what you're doing
	TCNT0 = tcnt0_start;			// Recall: 256-61 = 195 & 195*64us = 12.48ms, approx 12.5ms
	TIMSK0 = (1<<TOIE0);	// Enable Timer 0 interrupt
	
	// ADC initialization
	ADMUX = ((1<<REFS0) | (0 << ADLAR) | (0<<MUX0));  /* AVCC selected for VREF, ADC0 as ADC input  */
	ADCSRA = ((1<<ADEN)|(1<<ADSC)|(1<<ADATE)|(1<<ADIE)|(6<<ADPS0));
										/* Enable ADC, Start Conversion, Auto Trigger enabled, 
										   Interrupt enabled, Prescale = 64  */
	ADCSRB = (0<<ADTS0); /* Select AutoTrigger Source to Free Running Mode 
						    Strictly speaking - this is already 0, so we could omit the write to
						    ADCSRB, but this is included here so the intent is clear */
	
	sei();				// Global interrupt enable (I=1)
}

/**************************************************************************
*looping cylon pattern function
* end: variable to indicate at which led the function
*		should end (0, meaning full display; 4, meaning half display)
**************************************************************************/
void cylon_loop(int end)
{
	// cylon pattern
	if (direction == 0) { // check if direction is down
		active_led--; // decrement the active led
		if (active_led <= end) { // check if led has reached the start (pin 4 or 0)
			direction = 1; // set direction to up
		}
		PORTD = 0b00000001 << active_led; // set pin of portd to current led
		
		} else if (direction == 1) { // check if direction is up
		active_led++; // increment the active led
		if (active_led >= 7) { // check if led has reached end (pin 7)
			direction = 0; // set direction to down
		}
		PORTD = 0b00000001 << active_led; // set pin of portd to current led
		}
}


/***********************************************
* adc ouput thermometer display
* display_flag: checks if display should be in 
*				full 8-bit, or half 4-bit mode 
***********************************************/
void adc_display(int display_flag)
{
	if (display_flag == 1)
	{
		if ((adc_reading >= 0) && (adc_reading <= ONE_EIGHT_VOLTAGE))
		{
			PORTD = 0b00000000;
		} else if ((adc_reading >= ONE_EIGHT_VOLTAGE) && (adc_reading <= ONE_QUARTER_VOLTAGE))
		{
			PORTD = 0b00000001;
		} else if ((adc_reading >= ONE_QUARTER_VOLTAGE) && (adc_reading <= THREE_EIGHT_VOLTAGE))
		{
			PORTD = 0b00000011;
		} else if ((adc_reading >= THREE_EIGHT_VOLTAGE) && (adc_reading <= HALF_VOLTAGE))
		{
			PORTD = 0b00000111;
		} else if ((adc_reading >= HALF_VOLTAGE) && (adc_reading <= FIVE_EIGHT_VOLTAGE))
		{
			PORTD = 0b00001111;
		} else if ((adc_reading >= FIVE_EIGHT_VOLTAGE) && (adc_reading <= THREE_QUARTER_VOLTAGE))
		{
			PORTD = 0b00011111;
		} else if ((adc_reading >= THREE_QUARTER_VOLTAGE) && (adc_reading <= SEVEN_EIGHT_VOLTAGE))
		{
			PORTD = 0b00111111;
		} else if ((adc_reading >= SEVEN_EIGHT_VOLTAGE) && (adc_reading < UPPER_THRESHOLD_VOLTAGE))
		{
			PORTD = 0b01111111;
		} else if (adc_reading == UPPER_THRESHOLD_VOLTAGE)
		{
			PORTD = 0b11111111;
		}
	} else if (display_flag == 0)
	{
		if ((adc_reading >= 0) && (adc_reading <= ONE_QUARTER_VOLTAGE))
		{
			PORTD |= 0b00000000;
		} else if ((adc_reading >= ONE_QUARTER_VOLTAGE) && (adc_reading <= HALF_VOLTAGE))
		{
			PORTD |= 0b00000001;
		} else if ((adc_reading >= HALF_VOLTAGE) && (adc_reading <= THREE_QUARTER_VOLTAGE))
		{
			PORTD |= 0b00000011;
		} else if ((adc_reading >= THREE_QUARTER_VOLTAGE) && (adc_reading < UPPER_THRESHOLD_VOLTAGE))
		{
			PORTD |= 0b00000111;
		} else if (adc_reading == UPPER_THRESHOLD_VOLTAGE)
		{
			PORTD |= 0b00001111;
		}
	}
}

int main(void)
{
	init();
    while(1)
	{
		if (adc_flag)
		{
			if ((PINB & 0b00100000) == 0b00100000)
			{
				if ((PINB & 0b00010000) == 0b00000000)
				{
					display_flag = 1;
					adc_display(display_flag);
				}
			} else if ((PINB & 0b00100000) == 0b00000000)
			{
				display_flag = 0;
				adc_display(display_flag);
			}
		}
	}
}

ISR(TIMER0_OVF_vect)
{
	TCNT0 = tcnt0_start;		// set to start value based on 0.125s or 0.5s 
	++timecount0;	// count the number of times the interrupt has been reached
	
	if (timecount0 >= time_delay)	// check if amount of overflows equals adc setting
	{
		if ((PINB & 0b00100000) == 0b00100000)
		{
			if ((PINB & 0b00010000) == 0b00010000)
			{
				cylon_loop(0); // start 8-bit cylon pattern
				timecount0 = 0;		// Restart the overflow counter
			}
		} else {
			cylon_loop(4); // start 4-bit cylon pattern
			timecount0 = 0;
		}
	}
}

ISR (ADC_vect)	/* handles ADC interrupts  */
{
	
	adc_reading = ADC;   /* ADC is in Free Running Mode - you don't have to set up anything for 
						    the next conversion */
	if (adc_reading != 0) // check if new adc reading available
	{
		adc_flag = 1; // set flag
	}
	
	if ((adc_reading < LOWER_THRESHOLD_VOLTAGE) && (adc_reading > 0)) // check adc voltage is between 0V-2.5V
	{
		// 0.125s delay
		tcnt0_start = 39; // for 0.125s delay we start the timer count at 39
		time_delay = 9; // for 0.125s delay we want 9 overflows to trigger an interrupt
		
	} else if ((adc_reading < UPPER_THRESHOLD_VOLTAGE) && (adc_reading > LOWER_THRESHOLD_VOLTAGE)) // otherwise if adc voltage is between 2.5V-5V
	{
		// 0.5s delay
		tcnt0_start = 142; // for 0.5s delay we start the timer count at 142
		time_delay = 55; // for 0.5s delay we want 55 overflows to trigger an interrupt
	}
}