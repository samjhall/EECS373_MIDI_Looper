/*
 * project_helpers.c
 *
 *  Created on: Nov 16, 2018
 *      Author: samjhall
 */
#include "project_helpers.h"

/***	GLOBAL	***/
UART_instance_t apb_uart;

void Global_init() {
	MIDI_init();
	APB_UART_init();
	Timer_init();
	Touchscreen_init();
	Reset_init();
	IMU_init();
	VGA_init();
}

void Clear_channel(struct channel* channel) {
	int i = 0;
	while(i < (NUM_MEASURES * NOTES_PER_MEASURE)) {
		channel->data[i] = -1;
		++i;
	}
}

void Fill_channel(struct channel* channel) {
	channel->programNumber = 0;
	channel->lastPlayed = 0;
	Clear_channel(channel);
}

void Channel_init(struct channel* channelPtrs[16]) {
	int i = 0;
	while(i < 16) {
		channelPtrs[i]->channelNumber = i;
		Fill_channel(channelPtrs[i]);
		++i;
	}
}

void Update_metronome(struct Loop_Master* loopIn) {
	loopIn->shadow = loopIn->count;
	++loopIn->count;
	if(loopIn->count >= loopIn->metronome_bound) {
		if(loopIn->recordingMode != 0) {
			printf("DONE RECORDING ON CHANNEL %d\n\r", loopIn->selectedChannel);
			loopIn->recordingMode = ~loopIn->recordingMode;
		}
		
		clearCharDisplay();
		loopIn->count = 0;
	}
}

void Cycle_channels(struct channel* channelPtrs[16], struct Loop_Master* loopIn) {
	int i = 0;
	while (i < 16) {
		if(loopIn->channelsPlaying[i]){
			//printf("\tplaying channel %d\n\r", i);
			if(channelPtrs[i]->data[loopIn->count] != 0xFF) {
				noteOff(channelPtrs[i], channelPtrs[i]->lastPlayed, 0);
				noteOn(channelPtrs[i], channelPtrs[i]->data[loopIn->count], 40);
			}
			else {//if(channels[i]->data[Loop.count] == 0){
				noteOff(channelPtrs[i], channelPtrs[i]->lastPlayed, 0);
			}
		}
		else {
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

// RESET GPIO 4
void Reset_init() {
	MSS_GPIO_config( MSS_GPIO_4, MSS_GPIO_OUTPUT_MODE );
}

void reset() {
	MSS_GPIO_set_output(MSS_GPIO_4, MSS_GPIO_DRIVE_LOW);
	MSS_GPIO_set_output(MSS_GPIO_4, MSS_GPIO_DRIVE_HIGH);
}



/***	APB UART DEVICES	***/

void APB_UART_init(){ // character display and keypad
	UART_init(&apb_uart, BASE, 650, (DATA_8_BITS | NO_PARITY));
}

size_t readKeypad(uint8_t* buffer) {
	return UART_get_rx(&apb_uart, buffer, sizeof(buffer));
}

uint8_t instrumentSelect(uint8_t* buffer) {
	uint8_t instrument = 0;
	switch(buffer[0]) {
	case '0':
		instrument = 1;
		break;
	case '1':
		instrument = 18;
		break;
	case '2':
		instrument = 26;
		break;
	case '3':
		instrument = 35;
		break;

	case '4':
		instrument = 41;
		break;
	case '5':
		instrument = 49;
		break;
	case '6':
		instrument = 76;
		break;
	case '7':
		instrument = 81;
		break;

	case '8':
		instrument = 82;
		break;
	case '9':
		instrument = 106;
		break;
	case 'A':
		instrument = 117;
		break;
	case 'B':
		instrument = 119;
		break;

	case 'C':
		instrument = 121;
		break;
	case 'D':
		instrument = 105;
		break;
	case '*':
		instrument = 5;
		break;
	case '#':
		instrument = 128;
		break;

	}
	return instrument - 1; // need to adjust for 1-indexing
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

int checkPress(struct Loop_Master* loopIn) {
	int count =0;
	int i =0;
	while(i<11){
		int x1 = loopIn->touchscreenBuffer[i];
		int x2 = loopIn->touchscreenBuffer[i+2];
		int y1 = loopIn->touchscreenBuffer[i+1];
		int y2 = loopIn->touchscreenBuffer[i+3];
		int deltaX = x1 - x2;
		int deltaY = y1 - y2;
		// was 975		// was 875
		if(((deltaX < 20) && (deltaX > -20))&&((deltaY < 20) && (deltaY > -20)) && (x1>200 && (x1 > 1800 || x1 < 1640)) && (y1>300&&(y1 < 1570 || y1 > 1720)))
			count++;
		i+=2;
	}
	return count>5;
}

void parseTouch(struct Loop_Master* loopIn) {

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
		else{
			y = getY(adc_handler5);
		}
		++i;
		++j;
	}
	int z =0;
	while(z<12){
		loopIn->touchscreenBuffer[z] = loopIn->touchscreenBuffer[z+2];
		z++;
	}
	loopIn->touchscreenBuffer[12] = x;
	loopIn->touchscreenBuffer[13] = y;

}

void readTouch(struct Loop_Master* loopIn) {
	parseTouch(loopIn);
	loopIn->buttonsBuffer[0] = readButtons();
	int x =loopIn->touchscreenBuffer[0];
	int y =loopIn->touchscreenBuffer[1];
	int xSection = -1;
	int ySection = -1;
	if(x > 2600){
		xSection = 0;
	}
	else if(x < 2500 && x > 1900){
		xSection = 1;
	}
	else if(x < 1700 && x > 1100){
		xSection = 2;
	}
	else if(x < 1000){
		xSection = 3;
	}

	if(y > 2450){
		ySection = 0;
	}
	else if(y < 2400 && y > 1750){
		ySection = 1;
	}
	else if(y < 1650 && y > 1100){
		ySection = 2;
	}
	else if(y < 1000){
		ySection = 3;
	}

	//printf("X: %d    Y: %d\n\r", x, y);

	//Read Which Section
	if(checkPress(loopIn) && xSection != -1 && ySection!= -1){
		loopIn->touchscreenButtonPressed = xSection + ySection*4;
	}
	else{
		loopIn->touchscreenButtonPressed = -1;
	}
}


/***	GRAPHICS DISPLAY	***/
void VGA_init() {
	int i = 0;
	while(i < 16) {
		VGA_write(i, VGA_RED);
		++i;
	}
}
void VGA_write(uint8_t button, uint8_t color) {
	uint32_t data = 0x00000000 | (button << 4) | color;

	int i = 0;
	while(i < 10) {
		*((uint32_t*)VGA_DISPLAY_ADDRESS) = data;//0xF0000000 & (button << 4) & color;
		++i;
	}
	return;
}

uint32_t VGA_test() {
	return *((uint32_t*)VGA_DISPLAY_ADDRESS);
}

/***	BUTTONS 	***/

uint32_t readButtons() {
	// PLAY/PAUSE 1
	// CLEAR CHANNEL 2
	// CLEAR ALL 4
	// RECORD 8
	// TOGGLE ALL 16

	return *((uint32_t*)FUNCTION_BUTTONS_ADDRESS);
}

/***	IMU 	***/
void IMU_init() {
	// placeholder
}
uint16_t readIMU() {
	adc_handler2 = ACE_get_channel_handle((const uint8_t *)"ADCDirectInput_2");
	return ACE_get_ppe_sample(adc_handler2);
}
