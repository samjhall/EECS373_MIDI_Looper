/*
 * project_helpers.c
 *
 *  Created on: Nov 16, 2018
 *      Author: samjhall
 */
#include "project_helpers.h"

/***	GLOBAL	***/
UART_instance_t apb_uart;

volatile uint8_t pdma_buffer_idx = NB_OF_SAMPLE_BUFFERS;
uint8_t* const envm = (uint8_t *) 0x6002b000;
uint8_t* const dac_byte0 = (uint8_t*)0x40020504;

void Global_init() {
	MIDI_init();
	APB_UART_init();
	Timer_init();
	Touchscreen_init();
	Reset_init();
	IMU_init();
	VGA_init();
	Mic_init();
}

void Clear_channel(struct channel* channel) {
	int i = 0;
	while(i < (NUM_MEASURES * NOTES_PER_MEASURE)) {
		channel->data[i] = -1;
		channel->attack[i] = 0;
		++i;
	}
}

void Fill_channel(struct channel* channel) {
	channel->programNumber = 0;
	channel->button = 0;
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
			if(loopIn->selectedChannel == 10){
				recordVoice = 0;
				voiceRecorded = 1;
				envm_idx_max = envm_idx;
			}
		}
		
		clearCharDisplay();
		loopIn->count = 0;
	}
}

void Cycle_channels(struct channel* channelPtrs[16], struct Loop_Master* loopIn) {
	int i = 0;
	while (i < 16) {
		if(i != 10){
			if(loopIn->channelsPlaying[i]){
				//printf("\tplaying channel %d\n\r", i);
				if(channelPtrs[i]->data[loopIn->count] != 0xFF) {
					noteOff(channelPtrs[i], channelPtrs[i]->lastPlayed, 0);
					noteOn(channelPtrs[i], channelPtrs[i]->data[loopIn->count], channelPtrs[i]->attack[loopIn->count]);
				}
				else {//if(channels[i]->data[Loop.count] == 0){
					noteOff(channelPtrs[i], channelPtrs[i]->lastPlayed, 0);
				}
			}
			else {
				noteOff(channelPtrs[i], channelPtrs[i]->lastPlayed, 0);
			}
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
	uint8_t instrument = 1;
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

void charDisplayData(struct Loop_Master* loopIn, struct channel* channelPtrs[NUM_MEASURES * 8], uint32_t distance) {
	uint8_t hex[16] = {'0', '1', '2', '3',
						'4', '5', '6', '7',
						'8', '9', 'A', 'B',
						'C', 'D', 'E', 'F'};

	uint8_t display[4][20];

	int a = 0;
	while(a < 4){
		int b = 0;
		while(b < 20) {
			display[a][b] = ' ';
			++b;
		}
		++a;
	}

	// need to prepare this then send it

	//clearCharDisplay();

	// reset the cursor
	//uint8_t resetCursor[2] = {0xFE, 0x80};
	//sendCharDisplay(resetCursor, 2);

	// set channelsPlaying
	int row = 0;

	display[0][1] = loopIn->channelsRecorded[0] ? '0' : 'X';
	display[0][3] = loopIn->channelsRecorded[1] ? '1' : 'X';
	display[0][5] = loopIn->channelsRecorded[2] ? '2' : 'X';
	display[0][7] = loopIn->channelsRecorded[3] ? '3' : 'X';

	display[1][1] = loopIn->channelsRecorded[4] ? '4' : 'X';
	display[1][3] = loopIn->channelsRecorded[5] ? '5' : 'X';
	display[1][5] = loopIn->channelsRecorded[6] ? '6' : 'X';
	display[1][7] = loopIn->channelsRecorded[7] ? '7' : 'X';

	display[2][1] = loopIn->channelsRecorded[8] ? '8' : 'X';
	display[2][3] = loopIn->channelsRecorded[9] ? '9' : 'X';
	display[2][5] = loopIn->channelsRecorded[10] ? 'A' : 'X';
	display[2][7] = loopIn->channelsRecorded[11] ? 'B' : 'X';

	display[3][1] = loopIn->channelsRecorded[12] ? 'C' : 'X';
	display[3][3] = loopIn->channelsRecorded[13] ? 'D' : 'X';
	display[3][5] = loopIn->channelsRecorded[14] ? 'E' : 'X';
	display[3][7] = loopIn->channelsRecorded[15] ? 'F' : 'X';



	// set the divider between channelsPlaying and status info
	row = 0;
	while(row < 4) {
		display[row][8] = ']';
		++row;
	}

	// DEBUGGING
/*	row = 0;
	while(row < 4) {
		printf("%s\n\r", display[row]);
		sendCharDisplay(display[row], sizeof(display[row]));
		++row;
	}*/

	// set channel line
	display[0][9] = 'C';
	display[0][10] = 'h';
	display[0][11] = 'a';
	display[0][12] = 'n';
	display[0][13] = 'n';
	display[0][14] = 'e';
	display[0][15] = 'l';
	display[0][16] = ' ';
	display[0][17] = ' ';
	display[0][18] = ' ';
	display[0][19] = hex[loopIn->selectedChannel];

	// set parse line
	display[1][9] = 'P';
	display[1][10] = 'a';
	display[1][11] = 't';
	display[1][12] = 'c';
	display[1][13] = 'h';
	display[1][14] = ' ';
	display[1][15] = ' ';
	display[1][16] = ' ';
	display[1][17] = ' ';
	display[1][18] = ' ';
	display[1][19] = channelPtrs[loopIn->selectedChannel]->button;
	if(display[1][19] == 0) {
		display[1][19] = '0';
	}


	// set distance line
	if(distance > 80) {
		distance = 80;
	}
	int lsb = distance % 10;
	int msb = (distance - lsb) / 10;

	display[2][9] = 'D';
	display[2][10] = 'i';
	display[2][11] = 's';
	display[2][12] = 't';
	display[2][13] = 'a';
	display[2][14] = 'n';
	display[2][15] = 'c';
	display[2][16] = 'e';
	display[2][17] = ' ';
	display[2][18] = msb + 48;
	display[2][19] = lsb + 48;

	/*
	char pause[11] = 'PAUSED     ';
	char muted[11] = 'MUTED      '; // set by looking at channelsPlaying
	char mutedAll[11] = 'MUTED ALL  ';
	char clear[11] = 'CLEARED    ';
	char clearAll[11] = 'CLEARED ALL';
	char recording[11] = 'RECORDING  ';
	*/
	uint8_t statusNumber = 6;
	char status[7][11] = {"PAUSED     ", "MUTED      ",
							"MUTED ALL  ", "CLEARED    ",
							"CLEARED ALL", "RECORDING  ",
							"PLAYING    "};

	if(loopIn->paused != 0) { // paused
		statusNumber = 0;
	} else if(loopIn->buttonsBuffer[0] & 0x02) { // clear channel
		statusNumber = 3;
	} else if(loopIn->buttonsBuffer[0] & 0x04) { // clear all
		statusNumber = 4;
	} else if(loopIn->buttonsBuffer[0] & 0x08 || loopIn->recordingMode) { // recording
		statusNumber = 5;
	} else if(!loopIn->channelsPlaying[loopIn->selectedChannel]) { // muted
		statusNumber = 1;
	} else {
		if(loopIn->buttonsBuffer[0] & 0x10) { // muted all
			statusNumber = 2;
		}
		else {
			statusNumber = 6;
		}
	}

	uint8_t index = 9;
	while(index < 20) {
		display[3][index] = status[statusNumber][index-9];
		++index;
	}

	//printf("done\n\r");

	row = 0;
	while(row < 4) {
		//printf("%s\n\r", display[row]);
		sendCharDisplay(display[row], sizeof(display[row]));
		++row;
	}
	//printf("done\n\r");

	return;
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
		if(((deltaX < 20) && (deltaX > -20))&&((deltaY < 20) && (deltaY > -20)) && (x1>200 && (x1 > 1600 || x1 < 1450)) && (y1>300&&(y1 < 1450 || y1 > 1600)))
		//if(((deltaX < 20) && (deltaX > -20))&&((deltaY < 20) && (deltaY > -20)) && (x1>200 && (x1 > 1800 || x1 < 1640)) && (y1>300&&(y1 < 1570 || y1 > 1720)))
			count++;
		i+=2;
	}
	return count>1;

	/*
	uint16_t x = getX();
	uint16_t y = getY();
	uint8_t buttonX = -1;
	uint8_t buttonY = -1;


	if(x >= 2650) {
		buttonX = 0;
	} else if(x >= 1850 && x <= 2550) {
		buttonX = 1;
	} else if(x >= 940 && x <= 1600) {
		buttonX = 2;
	} else if(x <= 800) {
		buttonX = 3;
	}

	if(y >= 2540) {
		buttonY = 0;
	} else if(y >= 1800 && y <= 2400) {
		buttonY = 1;
	} else if(y >= 1000 && y <= 1600) {
		buttonY = 2;
	} else if(y <= 850) {
		buttonY = 3;
	}

	uint8_t buttonPressed = (buttonY * 4) + buttonX;
	if(buttonX == -1 || buttonY == -1) {
		return
	}*/
}

void parseTouch(struct Loop_Master* loopIn) {

	volatile uint16_t x = 0;
	volatile uint16_t y = 0;

	int j = 0;
	int i =0;
			// was 1000
			// was 500
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
	int x =loopIn->touchscreenBuffer[0];
	int y =loopIn->touchscreenBuffer[1];
	int xSection = -1;
	int ySection = -1;
	if(x > 2650){
		xSection = 0;
	}
	else if(x < 2550 && x > 1850){
		xSection = 1;
	}
						// was 1100
	else if(x < 1600 && x > 940){
		xSection = 2;
	}
	else if(x < 800){
		xSection = 3;
	}

	if(y > 2540){
		ySection = 0;
	}
	else if(y < 2400 && y > 1800){
		ySection = 1;
	}
	else if(y < 1600 && y > 1000){
		ySection = 2;
	}
	else if(y < 850){
		ySection = 3;
	}

	printf("X: %d    Y: %d\n\r", x, y);

	//Read Which Section
	if((checkPress(loopIn) && xSection != -1 && ySection!= -1)){
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
	uint32_t data = (button << 4) | color;

	int i = 0;
	while(i < 10) {
		*((uint32_t*)VGA_DISPLAY_ADDRESS) = data;
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

/*** Microphone ***/
void Mic_init(){
	mymode = NORMAL;
	recordVoice = 0;
	playVoice = 0;
	voiceRecorded = 0;
	envm_idx = 0;
	envm_idx_max = 1000000000;
	dac_irq = 0;
	NVIC_EnableIRQ(ACE_PC0_Flag0_IRQn);

	ACE_init();
	ACE_configure_sdd(
			SDD1_OUT,
			SDD_8_BITS,
			SDD_VOLTAGE_MODE | SDD_RETURN_TO_ZERO,
			INDIVIDUAL_UPDATE
	);
	ACE_enable_sdd(SDD1_OUT);
	ACE_disable_sse_irq(PC0_FLAG0);



	PDMA_init();
	PDMA_configure
	(
			PDMA_CHANNEL_0,
			PDMA_FROM_ACE,
			PDMA_LOW_PRIORITY | PDMA_WORD_TRANSFER | PDMA_INC_DEST_FOUR_BYTES,
			PDMA_DEFAULT_WRITE_ADJ
	);
	PDMA_set_irq_handler( PDMA_CHANNEL_0, ace_pdma_rx_handler );

	NVIC_EnableIRQ( DMA_IRQn );

	free_samples(0);
}


void free_samples(uint8_t full_flag){
	uint32_t i;
	for ( i = 0; i < NB_OF_SAMPLE_BUFFERS; ++i ){
		buffer_status[i] = FREE_BUFFER;
	}
	pdma_buffer_idx = 0;
	buffer_status[pdma_buffer_idx] = UNDER_DMA_CONTROL;
	if (full_flag){
	}
	else{
		PDMA_enable_irq( PDMA_CHANNEL_0 );
		PDMA_start
		(
				PDMA_CHANNEL_0,
				(uint32_t) PDMA_ACE_PPE_DATAOUT,
				(uint32_t) samples_buffer[pdma_buffer_idx],
				SAMPLES_BUFFER_SIZE
		);
	}

}

void ace_pdma_rx_handler()
{
	PDMA_clear_irq( PDMA_CHANNEL_0 );
	buffer_status[pdma_buffer_idx] = DATA_READY;
	PDMA_disable_irq( PDMA_CHANNEL_0 );
}

void process_samples(){
	uint8_t i;
	if ( buffer_status[pdma_buffer_idx] == DATA_READY){
		uint32_t proc_buffer_idx = pdma_buffer_idx;
		if (pdma_buffer_idx==(NB_OF_SAMPLE_BUFFERS-1))
			pdma_buffer_idx = 0;
		else
			pdma_buffer_idx++;

		if (envm_idx<TOTAL_SAMPLE_SIZE){
			PDMA_start (
					PDMA_CHANNEL_0,
					/* Read PPE_PDMA_DOUT */
					(uint32_t) PDMA_ACE_PPE_DATAOUT,
					/* This is in MSS ESRAM */
					(uint32_t)samples_buffer[pdma_buffer_idx],
					SAMPLES_BUFFER_SIZE
			);
			PDMA_enable_irq( PDMA_CHANNEL_0 );
		}

		buffer_status[pdma_buffer_idx] = UNDER_DMA_CONTROL;
		uint8_t data_processed[SAMPLES_BUFFER_SIZE];
		for ( i = 0; i < SAMPLES_BUFFER_SIZE; ++i ){
			volatile uint16_t raw_value;
			raw_value =  ACE_translate_pdma_value(samples_buffer[proc_buffer_idx][i], 0);
			data_processed[i] = raw_value>>4;
		}
		NVM_write((uint32_t)(envm+envm_idx), data_processed, SAMPLES_BUFFER_SIZE);
		envm_idx += SAMPLES_BUFFER_SIZE;
	}
}

void ACE_PC0_Flag0_IRQHandler(){
	uint32_t data_out = 0;

	data_out = envm[envm_idx];
	envm_idx += 1;

	if(playVoice && voiceRecorded && envm_idx<=envm_idx_max){
		ACE_set_sdd_value(SDD1_OUT, data_out);
	}
	ACE_clear_sse_irq(PC0_FLAG0);
}

