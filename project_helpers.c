/*
 * project_helpers.c
 *
 *  Created on: Nov 16, 2018
 *      Author: samjhall
 */

//#include "drivers/mss_uart/mss_uart.h"
//#include "drivers/CoreUARTapb/core_uart_apb.h"
//#include "drivers/mss_timer/mss_timer.h"
#include "project_helpers.h"
//#include <stdint.h>

/***	GLOBAL	***/
UART_instance_t apb_uart;

void Global_init() {
	MIDI_init();
	APB_UART_init();
	Timer_init();
	Touchscreen_init();
	Buttons_init();
}

void Update_metronome(struct Loop_Master* loopIn) {
	loopIn->shadow = loopIn->count;
	++loopIn->count;
	if(loopIn->count >= loopIn->metronome_bound) {
		if(loopIn->recordingMode != 0) {
			loopIn->recordingMode = ~loopIn->recordingMode;
		}
		
		clearCharDisplay();
		loopIn->count = 0;
	}
}

void Cycle_channels(struct channel* channelPtrs[16], struct Loop_Master* loopIn) {
	int i = 0;
	while (i < 16) {
		if(channelPtrs[i]->data[loopIn->count] != 0xFF) {
			noteOff(channelPtrs[i], channelPtrs[i]->lastPlayed, 0);
			noteOn(channelPtrs[i], channelPtrs[i]->data[loopIn->count], 40);
		}
		else {//if(channels[i]->data[Loop.count] == 0){
			noteOff(channelPtrs[i], channelPtrs[i]->lastPlayed, 0);
		}
		++i;
	}
	return;
}


/***	MIDI (UART1)	***/

void MIDI_init(){ // midi board on APB UART
	MSS_UART_init(&g_mss_uart1, 31250, MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT );
}

void programChange(struct channel* ch, uint8_t program) {
	uint8_t buffer[2] = {0xC0 | ch->channelNumber, program};
	MSS_UART_polled_tx(&g_mss_uart1, buffer, 2);
	ch->programNumber = program;
}

void noteOn(struct channel* ch, uint8_t pitch, uint8_t attack) {
	uint8_t buffer[3] = {0x90 | ch->channelNumber, pitch, attack};
	MSS_UART_polled_tx(&g_mss_uart1, buffer, 3);
	ch->lastPlayed = pitch;
}

void noteOff(struct channel* ch, uint8_t pitch, uint8_t attack) {
	uint8_t buffer[3] = {0x80 | ch->channelNumber, pitch, attack};
	MSS_UART_polled_tx(&g_mss_uart1, buffer, 3);
}

void allNotesOff() {
	uint8_t clearMIDI[3] = {0xB0, 123, 0};
	MSS_UART_polled_tx(&g_mss_uart1, clearMIDI, 3);
}



/***	APB UART DEVICES	***/

void APB_UART_init(){ // character display and keypad
	UART_init(&apb_uart, BASE, 650, (DATA_8_BITS | NO_PARITY));
}

size_t readKeypad(uint8_t* buffer) {
	return UART_get_rx(&apb_uart, buffer, sizeof(buffer));
}

void sendCharDisplay(uint8_t* buffer, uint8_t size) {
	UART_send(&apb_uart, buffer, size);
}


void clearCharDisplay() {
	// 4 x 20 display
	// This uses control commands defined in the screen's datasheet
	uint8_t clearBuffer[2] = {0xFE, 0x01};
	sendCharDisplay(clearBuffer, 2);
	clearBuffer[2] = 0x80;
	sendCharDisplay(clearBuffer, 2);
}



/***	TIMER	***/

void Timer_init() {
	MSS_TIM1_init( MSS_TIMER_PERIODIC_MODE );
}

void Timer_set_and_start(uint32_t cycle_count) {
	MSS_TIM1_load_immediate(cycle_count);
	MSS_TIM1_enable_irq();
	MSS_TIM1_start();
}

// This should defined in main.c so it is easier to modify
//void Timer1_IRQHandler() {
//	MSS_TIM1_clear_irq();
//}

/* USEFUL FUNCTIONS
 * MSS_TIM1_load_immediate( # of cycles );
 * MSS_TIM1_enable_irq();
 * MSS_TIM1_start();
 * MSS_TIM1_clear_irq();
 */

/***	DISTANCE SENSOR	***/
uint32_t readSensor() {
	volatile uint32_t* read = (uint32_t*)(DISTANCE_SENSOR_ADDRESS);
	//printf("\tDistance (cm): %x\n\r", *read);

	return ((*read) / (58 * 100));
}



/***	TOUCHSCREEN		***/
void Touchscreen_init() {
	ACE_init();
	MSS_GPIO_config( MSS_GPIO_0, MSS_GPIO_INOUT_MODE );
	MSS_GPIO_config( MSS_GPIO_1, MSS_GPIO_INOUT_MODE );
	MSS_GPIO_config( MSS_GPIO_2, MSS_GPIO_INOUT_MODE );
	MSS_GPIO_config( MSS_GPIO_3, MSS_GPIO_INOUT_MODE );
	adc_handler4 = ACE_get_channel_handle((const uint8_t *)"ADCDirectInput_4");
	adc_handler5 = ACE_get_channel_handle((const uint8_t *)"ADCDirectInput_5");
}
uint16_t getX() {
	//Set Xri and Xle as 0 and 5 volts and then set other two as highz(read from adc 4 which is connected to Yhi)
	MSS_GPIO_drive_inout( MSS_GPIO_0, MSS_GPIO_HIGH_Z );
	MSS_GPIO_drive_inout( MSS_GPIO_1, MSS_GPIO_HIGH_Z );
	MSS_GPIO_drive_inout( MSS_GPIO_2, MSS_GPIO_DRIVE_HIGH );
	MSS_GPIO_drive_inout( MSS_GPIO_3, MSS_GPIO_DRIVE_LOW );


	//Read Y2 as the output from ADC3
	uint16_t adc_data = ACE_get_ppe_sample(adc_handler4);
	return adc_data;
}
uint16_t getY() {
	//Set Ylo and Yhi to be 0 and 5 volts then set other two as highz(read from adc 5 which is connected to Xle).
	MSS_GPIO_drive_inout( MSS_GPIO_2, MSS_GPIO_HIGH_Z );
	MSS_GPIO_drive_inout( MSS_GPIO_3, MSS_GPIO_HIGH_Z );
	MSS_GPIO_drive_inout( MSS_GPIO_0, MSS_GPIO_DRIVE_HIGH );
	MSS_GPIO_drive_inout( MSS_GPIO_1, MSS_GPIO_DRIVE_LOW );
	//Set y1(GPIO3) to high and y2(GPIO1) to low



	//Read from adc at x1 or GPIO0
	uint16_t adc_data = ACE_get_ppe_sample(adc_handler5);
	return adc_data;
}

/*
void addNewVal(uint16_t* old, uint16_t newVal) {
	int i =0;
	while(i<15){
		old[15-i] = old[15-i-1];
		i++;
	}
	old[0] = newVal;
}

int checkPress(uint16_t* value) {
	int count =0;
	int i =0;
	while(i<15){
		int delta = value[15-i] - value[15-i-1];
															// was 975		// was 875
		if(((delta < 300) && (delta > -300)) && (value[15-i] >1700 || value[15-i] <1400))
			count++;
		i++;
	}
	return count>8;
}
*/

uint8_t parseTouch() {

	volatile uint16_t x = 0;
	volatile uint16_t y = 0;

	int j = 0;
	int i =0;
	while( j<1000 ) {
		if(i > 2 * ACE_SAMPLE_SIZE){
			i=0;
		}
		if(i<ACE_SAMPLE_SIZE){
			x = getX(adc_handler4);
		}
		//printf("X: %d  \n\r", x);
		else{
			y = getY(adc_handler5);
		}
		//printf("Y: %d  \n\r", y);

		printf("Y: %d  X: %d\n\r", y, x);
		++i;
		++j;
	}
	return 0;
}



/***	BUTTONS 	***/
void Buttons_init() {
	MSS_GPIO_config( MSS_GPIO_10, MSS_GPIO_INPUT_MODE );
	MSS_GPIO_config( MSS_GPIO_11, MSS_GPIO_INPUT_MODE );
	MSS_GPIO_config( MSS_GPIO_12, MSS_GPIO_INPUT_MODE );
	MSS_GPIO_config( MSS_GPIO_13, MSS_GPIO_INPUT_MODE );
}
uint8_t readButtons() {
	// PLAY/PAUSE GPIO 10
	// CLEAR CHANNEL GPIO 11
	// CLEAR ALL GPIO 12
	// RECORD GPIO 13
	return (uint8_t)(0x0000000F & (MSS_GPIO_get_inputs() >> 10));
}
