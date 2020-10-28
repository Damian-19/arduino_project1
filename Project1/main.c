/*
 * Timer0_Ex.c
 *
 * Created: 09/10/2013 23:37:12
 *  Author: Ciaran
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>

#define THRESHOLD_VOLTAGE 600

unsigned int timecount0;
int time_delay = 40;
uint16_t adc_flag = 0;
uint16_t adc_reading;

void init(int timecount)
{
	//Set PortD to all outputs because LEDs are connected to this PORT
	DDRD = 0xff;	// 0xff = 0b11111111; all ones
	PORTD = 0;		// Initialise to all off
	
	timecount0 = timecount;
	TCCR0B = (5<<CS00);	// Set T0 Source = Clock (16MHz)/1024 and put Timer in Normal mode
	
	TCCR0A = 0;			// Not strictly necessary as these are the reset states but it's good
	// practice to show what you're doing
	TCNT0 = 61;			// Recall: 256-61 = 195 & 195*64us = 12.48ms, approx 12.5ms
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
	init(0);
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
	TCNT0 = 61;		//TCNT0 needs to be set to the start point each time
	++timecount0;	// count the number of times the interrupt has been reached
	
	if (timecount0 >= time_delay)	// 40 * 12.5ms = 500ms
	{
		PORTD = ~PORTD;		// Toggle all the bits
		timecount0 = 0;		// Restart the overflow counter
	}
}

ISR (ADC_vect)	/* handles ADC interrupts  */
{
	
	adc_reading = ADC;   /* ADC is in Free Running Mode - you don't have to set up anything for 
						    the next conversion */
	if (adc_reading != 0)
	{
		adc_flag = 1;
	}
	if (adc_reading > THRESHOLD_VOLTAGE)
	{
		time_delay = 10;
	} else {
		time_delay = 40;
	}
}