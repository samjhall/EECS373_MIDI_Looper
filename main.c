#include "project_helpers.h"


// baud val (written to BASE + 0x008) = (clock / (16 * baud rate)) - 1
// for 100 MHz at 9600 baud, this is 650 in decimal

struct Loop_Master Loop = {
		1,
		0,
		NUM_MEASURES * NOTES_PER_MEASURE,
		0,
		0,
		-1,
		{0xBB, 0xBB},
		//{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0},
		{0xFFFFFFFF},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};


struct channel channel0;
struct channel channel1;
struct channel channel2;
struct channel channel3;

struct channel channel4;
struct channel channel5;
struct channel channel6;
struct channel channel7;

struct channel channel8;
struct channel channel9;
struct channel channel10;
struct channel channel11;

struct channel channel12;
struct channel channel13;
struct channel channel14;
struct channel channel15;

struct channel* channels[16] = {&channel0, &channel1, &channel2, &channel3,
								&channel4, &channel5, &channel6, &channel7,
								&channel8, &channel9, &channel10, &channel11,
								&channel12, &channel13, &channel14, &channel15};

uint8_t GLOBAL_PAUSE_FLAG = 0;

// This will be the main "driver" function as most of the work will be done between interrupts
void Timer1_IRQHandler() {
	// check the four function buttons
	// check for pause
	printf("--------COUNT: %d--------\n\r", Loop.count);
	printf("distance: %d\n\r	imu: %d\n\r", readSensor(), readIMU());

	Loop.buttonsBuffer[0] = readButtons();

	if(Loop.buttonsBuffer[0] & 0x01) {
		printf("PAUSED\n\r");
		//VGA_write(4, 7);
		GLOBAL_PAUSE_FLAG = ~GLOBAL_PAUSE_FLAG;
		if(GLOBAL_PAUSE_FLAG == 0 && Loop.channelsPlaying[10]){
			ACE_enable_sse_irq(PC0_FLAG0);
			playVoice = 1;
		}
		else if(GLOBAL_PAUSE_FLAG != 0 && Loop.channelsPlaying[10]){
			ACE_disable_sse_irq(PC0_FLAG0);
			playVoice = 0;
		}
	}

	if(Loop.buttonsBuffer[0] & 0x02) {
		Clear_channel(channels[Loop.selectedChannel]);
		if(Loop.selectedChannel == 10){
			free_samples(0);
			envm_idx_max = 0;
			voiceRecorded = 0;
			playVoice = 0;
			recordVoice = 0;
			ACE_disable_sse_irq(PC0_FLAG0);
		}
		printf("CHANNEL %d CLEARED\n\r", Loop.selectedChannel);
	}

	if(Loop.buttonsBuffer[0] & 0x04) {
		int i = 0;
		while (i < 16) {
			Clear_channel(channels[i]);
			++i;
		}
		envm_idx_max = 0;
		voiceRecorded = 0;
		playVoice = 0;
		recordVoice = 0;
		ACE_disable_sse_irq(PC0_FLAG0);
		//free_samples(0);
		if(Loop.recordingMode != 0) {
			Loop.recordingMode = ~Loop.recordingMode;
		}
		allNotesOff();
		reset();
		printf("ALL CHANNELS CLEARED\n\r");
	}

	if(Loop.buttonsBuffer[0] & 0x10) {
		 int i = 0;
		 while(i < 16) {
			 Loop.channelsPlaying[i] = 0;
			 ++i;
		 }
		 VGA_init();
		 playVoice = 0;
		 printf("MUTED ALL CHANNELS\n\r");
	}

	if(GLOBAL_PAUSE_FLAG != 0) { // user should be able to clear when paused, but not record
		printf("PAUSED\n\r");
		allNotesOff();
		ACE_disable_sse_irq(PC0_FLAG0);
		MSS_TIM1_clear_irq();
		return;
	}

	//Starting New Loop
	if(Loop.count == 0){
		envm_idx = 0;
		if(Loop.recordingMode != 0 && Loop.selectedChannel == 10){
			free_samples(0);
			ACE_disable_sse_irq(PC0_FLAG0);
			recordVoice = 1;
			playVoice = 0;
		}
		else if(Loop.channelsPlaying[10]){
			ACE_enable_sse_irq(PC0_FLAG0);
			recordVoice = 0;
			playVoice = 1;
		}
	}


	//Start Recording
	if((Loop.buttonsBuffer[0] & 0x08) && (Loop.recordingMode == 0)) { // set recordingMode to "on" and restart the metronome
		printf("RECORDING ON CHANNEL %d\n\r", Loop.selectedChannel);
		Loop.channelsPlaying[Loop.selectedChannel] = 1;
		Loop.recordingMode = ~Loop.recordingMode;
		Loop.count = 0;
		if(Loop.selectedChannel == 10 && (playVoice ||voiceRecorded)){
			free_samples(0);
		}

		MSS_TIM1_clear_irq();
		return;
	}
	if((Loop.buttonsBuffer[0] & 0x08) && (Loop.recordingMode != 0)) {
		printf("DONE RECORDING ON CHANNEL %d\n\r", Loop.selectedChannel);
		Loop.recordingMode = ~Loop.recordingMode;
		if(Loop.selectedChannel == 10){
			recordVoice = 0;
			envm_idx_max = envm_idx;
			voiceRecorded = 1;
		}
		Loop.count = 0;
		MSS_TIM1_clear_irq();
		return;
	}


	if(Loop.selectedChannel == Loop.touchscreenButtonPressed){
		printf("TOGGLING CHANNEL %d\n\r", Loop.selectedChannel);
		if(Loop.selectedChannel == 10 && Loop.channelsPlaying[10] == 0){
			ACE_enable_sse_irq(PC0_FLAG0);
		}
		if(Loop.selectedChannel == 10){
			playVoice = !playVoice;
		}
		else
			Loop.channelsPlaying[Loop.selectedChannel] = !Loop.channelsPlaying[Loop.selectedChannel];
	}

	// check for a touchscreen press
	if(Loop.touchscreenButtonPressed != 255) {
		uint8_t color = 0;
		if(Loop.selectedChannel!=10){
			color = (Loop.channelsPlaying[Loop.selectedChannel]) ? VGA_GREEN : VGA_RED;
		}
		else{
			color = (playVoice) ? VGA_GREEN : VGA_RED;
		}
		VGA_write(Loop.selectedChannel, color);


		Loop.selectedChannel = Loop.touchscreenButtonPressed;

		VGA_write(Loop.selectedChannel, VGA_YELLOW);

		printf("Button Pressed: %d\n\r", Loop.touchscreenButtonPressed);
	}


	// instrument selection from the keypad
	Loop.keypadBuffer[1] = Loop.keypadBuffer[0];
	readKeypad(Loop.keypadBuffer);
	uint8_t program = instrumentSelect(Loop.keypadBuffer);
	if((program != channels[Loop.selectedChannel]->programNumber)
			&& (program != 255)
			&& (Loop.keypadBuffer[0] != Loop.keypadBuffer[1])) {
		printf("CHANGE TO %d\n\r", program);
		programChange(channels[Loop.selectedChannel], program);
		channels[Loop.selectedChannel]->programNumber = program;
	}



	if(Loop.recordingMode != 0) {
		uint32_t data = readSensor();
		uint32_t attack = readIMU();
		if(attack>3500){
			attack = 3500;
		}
		else if(attack<1500){
			attack = 1500;
		}
		if(Loop.selectedChannel == 9) { // special setup for drums
			if((data <= 16)) {
				channels[Loop.selectedChannel]->data[Loop.count] = 35; // bass
			} else if((data > 16) && (data <= 24)) {
				channels[Loop.selectedChannel]->data[Loop.count] = 42; // closed hihat
			} else if((data > 24) && (data <= 32)) {
				channels[Loop.selectedChannel]->data[Loop.count] = 38; // snare
			} else if((data > 32) && (data <= 40)) {
				channels[Loop.selectedChannel]->data[Loop.count] = 49; // crash
			} else if((data > 40) && (data <= 48)) {
				channels[Loop.selectedChannel]->data[Loop.count] = 39; // clap
			} else if((data > 48) && (data <= 56)) {
				channels[Loop.selectedChannel]->data[Loop.count] = 60; // high bongo
			} else if((data > 56) && (data <= 64)) {
				channels[Loop.selectedChannel]->data[Loop.count] = 36; // open hihat
			} else if((data > 64) && (data <= 72)) {
				channels[Loop.selectedChannel]->data[Loop.count] = 56; // cowbell
			}
		}
		else {
			if((data > 72) || (data < 8)) { // total width of 64
				channels[Loop.selectedChannel]->data[Loop.count] = -1;
			}
			else {
				channels[Loop.selectedChannel]->data[Loop.count] = readSensor() + 12;
			}
		}

		channels[Loop.selectedChannel]->attack[Loop.count] = (attack - 1500) / 16;
	}



	Cycle_channels(channels, &Loop);
	uint8_t charBuffer[4] = {32, Loop.keypadBuffer[0], 32, Loop.selectedChannel+48};
	sendCharDisplay(charBuffer, sizeof(charBuffer));

	Update_metronome(&Loop);
	MSS_TIM1_clear_irq();
}

void test_library() {
	Global_init();

	reset();

	clearCharDisplay();

	Timer_set_and_start(25000000); // 1 second = 100 000 000
	//Timer_set_and_start(12500000);

	while(1) {


		readTouch(&Loop);

		//VGA_test();



		/*** MICROPHONE ***/
		/*
		ace_channel_handle_t adc_handler2 = ACE_get_channel_handle((const uint8_t *)"ADCDirectInput_2");
		uint16_t adc_data2 = ACE_get_ppe_sample(adc_handler2);
		printf("Here it is: %d\n\r", (int)adc_data2);
		*/

		if (recordVoice==1){
			process_samples();
		}
		//else{
		//	play_samples(mymode);
		//}

		if (envm_idx>=envm_idx_max){
			ACE_disable_sse_irq(PC0_FLAG0);
			//else {
			//	ACE_enable_sse_irq(PC0_FLAG0);
			//}
		}

	}
}


int main()
{

	//Important stuff
	printf("test\n"); // test UART0
	Channel_init(channels);
	test_library();


	return 0;
}
