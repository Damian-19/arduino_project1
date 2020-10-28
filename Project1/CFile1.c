/*
 * Timer0_Ex.c
 *
 * Created: 09/10/2013 23:37:12
 *  Author: Ciaran
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>

unsigned int timecount0;
void init(void)
{
	//Set PortD to all outputs because LEDs are connected to this PORT
	DDRD = 0xff;	// 0xff = 0b11111111; all ones
	PORTD = 0;		// Initialise to all off
	
	// Timer initialization
	TCCR0B = (5<<CS00);	// Set T0 Source = Clock (16MHz)/1024 and put Timer in Normal mode
	
	TCCR0A = 0;			// Not strictly necessary as these are the reset states but it's good
	// practice to show what you're doing
	TCNT0 = 61;			// Recall: 256-61 = 195 & 195*64us = 12.48ms, approx 12.5ms
	TIMSK0 = (1<<TOIE0);	// Enable Timer 0 interrupt
	
	// ADC initialization
	ADMUX = ((1<<REFS0) | (0 << ADLAR) | (0<<MUX0));  /* AVCC selected for VREF, ADC0 as ADC input  */
	ADCSRA = ((1<<ADEN)|(1<<ADSC)|(1<<ADATE)|(1<<ADIE)|(5<<ADPS0));
										/* Enable ADC, Start Conversion, Auto Trigger enabled, 
										   Interrupt enabled, Prescale = 32  */
	ADCSRB = (0<<ADTS0); /* Select AutoTrigger Source to Free Running Mode 
						    Strictly speaking - this is already 0, so we could omit the write to
						    ADCSRB, but this is included here so the intent is clear */
	
	sei();				// Global interrupt enable (I=1)
}

int main(void)
{
	init();
	timecount0 = 0;	// Initialize the overflow count. Note its scope
    while(1)
		;			// Do nothing loop
}

ISR(TIMER0_OVF_vect)
{
	TCNT0 = 61;		//TCNT0 needs to be set to the start point each time
	++timecount0;	// count the number of times the interrupt has been reached
	
	if (timecount0 >= 40)	// 40 * 12.5ms = 500ms
	{
		PORTD = ~PORTD;		// Toggle all the bits
		timecount0 = 0;		// Restart the overflow counter
	}
}