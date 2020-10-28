/*
 * main.c
 *
 * Created: 28/10/2020 
 *  Author: Damian
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>

#define LOWER_THRESHOLD_VOLTAGE 511
#define UPPER_THRESHOLD_VOLTAGE 1023
#define TEST_PIN REGISTER_BIT(PORTD, 1) // testing

unsigned int timecount0;
int time_delay = 0;
int tcnt0_start = 125;
uint16_t adc_flag = 0;
uint16_t adc_reading;

void init(void)
{
	//Set PortD to all outputs because LEDs are connected to this PORT
	DDRD = 0xff;	// 0xff = 0b11111111; all ones
	PORTD = 0;		// Initialise to all off
	DDRB = 0b00000000;
	PORTB = 0;
	
	timecount0 = 0;
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

int main(void)
{
	init();
    while(1)
	{
		if (adc_flag == 1)
		{
			PORTD = PORTD | (1<<PORTD7);
			adc_flag = 0;
		}
	}
}

ISR(TIMER0_OVF_vect)
{
	TCNT0 = tcnt0_start;		// set to 
	++timecount0;	// count the number of times the interrupt has been reached
	
	if (timecount0 >= time_delay)	
	{
		PORTD = ~PORTD;		// Toggle all the bits
		timecount0 = 0;		// Restart the overflow counter
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
		// 1s delay
		tcnt0_start = 125; // set start of timer count
		time_delay = 125; // set number of overflows
	} else if ((adc_reading < UPPER_THRESHOLD_VOLTAGE) && (adc_reading > LOWER_THRESHOLD_VOLTAGE)) // otherwise if adc voltage is between 2.5V-5V
	{
		// 0.5s delay
		tcnt0_start = 142; // set start of timer count
		time_delay = 55; // set number of overflows
	}
}