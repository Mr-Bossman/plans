#define F_CPU 20000000UL
#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#define HOLDTIME 50
#define BRIGHTTIME 259
#define DELAYTIME 40
// a b c d e f g
// 5 7 4 0 2 1 6
static const uint8_t LUT[16] = {0b01001000,0b01101111,0b00011010,0b00001110,0b00101101,0b10001100,0b10001000,0b01001111,0b00001000,0b00001100,0b00001001,0b10101000,0b10111010,0b00101010,0b10011000,0b10011001};
static volatile uint8_t state = 0,sales = 0,plans = 0;
static volatile	uint16_t perc = 0;
static volatile uint8_t bright = 255;
static inline void sel_seg(uint8_t index,uint8_t on) {
	PORTA.OUT = (PORTA.OUT & 0xf) | ((on << index) << 4);
}

static inline void display(uint8_t val,uint8_t point){
	PORTC.OUT = (PORTC.OUT & 0xf0) | ((LUT[val] & point) & 0xf);
	PORTB.OUT = (PORTB.OUT & 0xf0) | ((LUT[val] & point) >> 4);
}

static inline uint8_t Log10(uint8_t a){
	if(a >= 100) return 2;
	if(a >= 10) return 1;
	else return 0;
}
static uint16_t decidiv(uint16_t den, uint8_t sor){
	uint8_t i = 0;
	uint16_t ret = 0;
	if(!sor) return 0;
	den *= 100;
	uint8_t whole = den/sor;
	if(!i) i = Log10(whole) + 1;
	do {
		ret >>= 4;
		ret |= whole%10 << (i-1)*4;
	} while ( whole /= 10);
	den = (den % sor)  * 10;
	for (; i < 4; i++) {
		ret <<= 4;
		ret |= den/sor;
		den = (den % sor) * 10;
	}
	return ret;
}

static uint16_t dec(uint8_t dev){
	if(!dev) return 0;
	uint8_t i = 0;
	uint16_t ret = 0;
	if(!i) i = Log10(dev) + 1;
	do {
		ret >>= 4;
		ret |= dev % 10 << (i - 1) * 4;
	} while ( dev /= 10);
	return ret;
}

static void displayall(uint16_t val, uint8_t point){
	for(uint8_t i = 0; i < 4; i++) {
		PORTA.OUT = (PORTA.OUT & 0xf);
		display((val >> i*4) & 0xf, ((point >> i) & 1)? ~(1 << 3)  : 0xff);
		sel_seg(i, 1);
		_delay_loop_2(59*bright);
	}
	PORTA.OUT = (PORTA.OUT & 0xf);
}

static void run_buttons(uint8_t buttons){
	if (buttons & 1) {
		switch (state) {
			case 0:if(plans < 255)plans++;
			break;
			case 1:if(plans < 255) plans++;
			break;
			case 2:if(sales < 255) sales++;
			break;
		}
	}
	if (buttons & 2) {
		switch (state) {
			case 0:if(sales < 255) sales++;
			break;
			case 1:if(plans) plans--;
			break;
			case 2:if(sales) sales--;
			break;
		}
	}
	perc = decidiv(plans, sales);
}

int main (void) {
	CPU_CCP = 0xD8;
	CLKCTRL_MCLKCTRLA = CLKCTRL_CLKSEL_OSC20M_gc;
	CPU_CCP = 0xD8;
	CLKCTRL_MCLKCTRLB = 0;
	PORTB.DIR = 0xf;
	PORTC.DIR = 0xf;
	PORTA.DIR = 0xf0;
	PORTA.PIN1CTRL = (1 << 3) | 1;
	PORTA.PIN2CTRL = (1 << 3) | 1;
	PORTA.PIN3CTRL = (1 << 3) | 1;
	sei();
	uint16_t i = BRIGHTTIME;
	uint8_t b = 0;
	while (1) {
		while (!bright)
			i = BRIGHTTIME;
		if(bright < 255)
			_delay_loop_2(15045 - (59*bright));
		switch (state) {
			case 1: displayall(dec(plans),0);
			break;
			case 2: displayall(dec(sales),0);
			break;
			default: displayall(perc,(sales)?1 << (3 - Log10((100*(uint16_t)(plans))/sales)):0);
		}
		b++;
		if(b == DELAYTIME){
			b = 0;
			if(i > 0)
				i--;
		}
		if(i <= 255)
			bright = i;

	}
}

ISR(PORTA_PORT_vect) {
	bright = 255;
	if(PORTA.INTFLAGS & PORTA.IN){
		PORTA.INTFLAGS = 0xff;
		_delay_loop_2(0);
		return;
	}
	PORTA.INTFLAGS = 0xff;
	uint8_t i = 0;
	while (~PORTA.IN & 2) {
		i++;
		_delay_loop_2(0);
		if (i > HOLDTIME) break;
	}
	if (i > HOLDTIME) {
		state = sales = plans = 0;
		} else if (i) {
		state++;
		if (state > 2) state = 0;
	}
	run_buttons((~PORTA.IN >> 2) & 3);
	_delay_loop_2(0);
}
