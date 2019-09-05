// George Dagis, [PARTNER NAME OMITTED]
// 12-02-17
// Program simulated 4-way intersection
// Utilizes 3 inputs, taken from push-button switches
//
// Southwards green,yellow,red light connected to PB0,PB1,PB2
// Westwards green,yellow,red light connected to PB3,PB4,PB5
// Walk,stop light connected to PF3,PF1
// Detectors for westwards,southwards,pedestrian connected to PE0,PE1,PE2
// ***** 1. Pre-processor Directives Section *****

#include "TExaS.h"
#include "tm4c123gh6pm.h"

// ***** 2. Global Declarations Section *****
#define NVIC_ST_CTRL_R (*((volatile unsigned long *) 0xE000E010))
#define NVIC_ST_RELOAD_R (*((volatile unsigned long *) 0xE000E014))
#define NVIC_ST_CURRENT_R (*((volatile unsigned long *) 0xE000E018))
#define TRAFFIC_LIGHTS (*((volatile unsigned long *) 0x400050FC))
#define PEDESTRIAN_LIGHTS (*((volatile unsigned long *) 0x40025028))
#define SENSORS (*((volatile unsigned long *) 0x4002401C))

// FUNCTION PROTOTYPES: Each subroutine defined
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void); // Enable interrupts

// ***** 3. Subroutines Section *****
void ports_Init(void) {

	// Port B, E, & Finitialization
	unsigned long volatile delay;
	SYSCTL_RCGC2_R |= 0x32; // Activate clock for Port B, E, & F
	delay = SYSCTL_RCGC2_R; // Delay for clock to start
	
	// Port B initialization
	GPIO_PORTB_LOCK_R = 0x4C4F434B; // Unlock port
	GPIO_PORTB_CR_R = 0x3F; // Allow changes to PB5-0
	GPIO_PORTB_PCTL_R = 0x00000000; // Clear PCTL
	GPIO_PORTB_AMSEL_R &= ~0x3F; // Disable analog on PB5-0
	GPIO_PORTB_AFSEL_R &= ~0x3F; // Disable alt funct on PB5-0
	GPIO_PORTB_DEN_R |= 0x3F; // Enable digital I/O on PB5-0
	GPIO_PORTB_DIR_R |= 0x3F; // PB5-0 outputs
	
	// Port E initialization
	GPIO_PORTE_LOCK_R = 0x4C4F434B; // Unlock port
	GPIO_PORTE_CR_R = 0x07; // Allow changes to PE2-0
	GPIO_PORTE_PCTL_R = 0x00000000; // Clear PCTL
	GPIO_PORTE_AMSEL_R &= ~0x07; // Disable analog on PE2-0
	GPIO_PORTE_AFSEL_R &= ~0x07; // Disable alt funct on PE2-0
	GPIO_PORTE_PUR_R &= ~0x07; // Disable pull-up on PE2-0
	GPIO_PORTE_DEN_R |= 0x07; // Enable digital I/O on PE2-0
	GPIO_PORTE_DIR_R &= ~0x07; // PE2-0 inputs
	
	// Port F initialization
	GPIO_PORTF_LOCK_R = 0x4C4F434B; // Unlock port
	GPIO_PORTF_CR_R = 0x0A; // Allow changes to PF1 & PF3
	GPIO_PORTF_PCTL_R = 0x00000000; // Clear PCTL
	GPIO_PORTF_AMSEL_R &= ~0x0A; // Disable analog on PF1 & PF3
	GPIO_PORTF_AFSEL_R &= ~0x0A; // Disable alternate function on PF1 & PF3
	GPIO_PORTF_DEN_R |= 0x0A; // Enable digital I/O on PF1 & PF3
	GPIO_PORTF_DIR_R |= 0x0A; // PF1 & PF3 outputs
}

void SysTick_Init(void) {
	// Systick initialization
	NVIC_ST_CTRL_R = 0; // Disable SysTick during setup
	NVIC_ST_CTRL_R = 0x00000005; // Enable SysTick with core clock
}

void SysTick_Wait10ms() {
	// Delay for 10ms
	NVIC_ST_RELOAD_R = 8000000 - 1; // Wait (80Mhz PLL)
	NVIC_ST_CURRENT_R = 0; // Value written to CURRENT is cleared
	while((NVIC_ST_CTRL_R&0x00010000)==0) { // Wait for count flag
	}
}

void SysTick_Wait(unsigned long delay) {
	// Delay
	unsigned long i;
	for(i=0; i < delay; i++)
	SysTick_Wait10ms();
}

typedef struct Stype {
	// Structure of a single state in the Finite State Machine
	unsigned long TrafficOut; // Output for car lights (Port B)
	unsigned long WalkOut; // Output for pedestrian lights (Port F)
	unsigned long Time; // Delay time
	unsigned long Next[8]; // Next state
} SType;

int main(void){
	unsigned long S = 0; // Current state
	SType FSM[11]={
		// States of Finite State Machine
		{0x0C,0x02,20,{0,0,1,1,1,1,1,1}},
		{0x14,0x02,30,{1,0,2,2,4,4,2,2}},
		{0x21,0x02,20,{2,3,2,3,3,3,3,3}},
		{0x22,0x02,30,{3,0,2,0,4,0,4,4}},
		{0x24,0x08,20,{4,5,5,5,4,5,5,5}},
		{0x24,0x00,5,{4,6,6,6,4,6,6,6}},
		{0x24,0x02,5,{4,7,7,7,4,7,7,7}},
		{0x24,0x00,5,{4,8,8,8,4,8,8,8}},
		{0x24,0x02,5,{4,9,9,9,4,9,9,9}},
		{0x24,0x00,5,{4,10,10,10,4,10,10,10}},
		{0x24,0x02,5,{5,0,2,0,4,0,2,0}}
	};

	// Initialization
	ports_Init(); // Initialize ports B, E, & F
	SysTick_Init(); // Initialize systick
	EnableInterrupts();

	// Loop through FSM
	while(1) {
		TRAFFIC_LIGHTS = FSM[S].TrafficOut; // Set car lights
		PEDESTRIAN_LIGHTS = FSM[S].WalkOut; // Set pedestrian lights
		SysTick_Wait(FSM[S].Time); // Delay
		S = FSM[S].Next[SENSORS]; // Next state
	}
} //end main