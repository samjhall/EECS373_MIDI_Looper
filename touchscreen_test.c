#include <stdio.h>
#include "drivers/mss_uart/mss_uart.h"
#include "drivers/mss_gpio/mss_gpio.h"
#include "drivers/mss_ace/mss_ace.h"

//Just for reference I will say GPIO 0 is Yhi, GPIO1 is Ylo, GPIO2 is Xle, GPIO3 is Xri
uint16_t getX(ace_channel_handle_t adc_handler){
	//Set Xri and Xle as 0 and 5 volts and then set other two as highz(read from adc 4 which is connected to Yhi)
	MSS_GPIO_drive_inout( MSS_GPIO_0, MSS_GPIO_HIGH_Z );
	MSS_GPIO_drive_inout( MSS_GPIO_1, MSS_GPIO_HIGH_Z );
	MSS_GPIO_drive_inout( MSS_GPIO_2, MSS_GPIO_DRIVE_HIGH );
	MSS_GPIO_drive_inout( MSS_GPIO_3, MSS_GPIO_DRIVE_LOW );

	//Read Y2 as the output from ADC3
	uint16_t adc_data = ACE_get_ppe_sample(adc_handler);
	return adc_data;
}
uint16_t getY(ace_channel_handle_t adc_handler){
	//Set Ylo and Yhi to be 0 and 5 volts then set other two as highz(read from adc 5 which is connected to Xle).
	MSS_GPIO_drive_inout( MSS_GPIO_2, MSS_GPIO_HIGH_Z );
	MSS_GPIO_drive_inout( MSS_GPIO_3, MSS_GPIO_HIGH_Z );
	MSS_GPIO_drive_inout( MSS_GPIO_0, MSS_GPIO_DRIVE_HIGH );
	MSS_GPIO_drive_inout( MSS_GPIO_1, MSS_GPIO_DRIVE_LOW );
	//Set y1(GPIO3) to high and y2(GPIO1) to low
	//Read from adc at x1 or GPIO0
	uint16_t adc_data = ACE_get_ppe_sample(adc_handler);
	return adc_data;
}

void addNewVal(uint16_t* old, uint16_t newVal){
	int i =0;
	while(i<15){
		old[15-i] = old[15-i-1];
		i++;
	}
	old[0] = newVal;
}

int checkPress(uint16_t* value){
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

int main()
{
	//MSS_GPIO_config( MSS_GPIO_0, MSS_GPIO_OUTPUT_MODE );
	//MSS_GPIO_config( MSS_GPIO_1, MSS_GPIO_OUTPUT_MODE );
	//MSS_GPIO_config( MSS_GPIO_2, MSS_GPIO_OUTPUT_MODE );
	//MSS_GPIO_config( MSS_GPIO_3, MSS_GPIO_OUTPUT_MODE );
	//MSS_GPIO_set_output(MSS_GPIO_0, 1);
	//MSS_GPIO_set_output(MSS_GPIO_1, 0);
	//MSS_GPIO_set_output(MSS_GPIO_2, 1);
	//MSS_GPIO_set_output(MSS_GPIO_3, 0);

	ACE_init();
	MSS_GPIO_config( MSS_GPIO_0, MSS_GPIO_INOUT_MODE );
	MSS_GPIO_config( MSS_GPIO_1, MSS_GPIO_INOUT_MODE );
	MSS_GPIO_config( MSS_GPIO_2, MSS_GPIO_INOUT_MODE );
	MSS_GPIO_config( MSS_GPIO_3, MSS_GPIO_INOUT_MODE );
	ace_channel_handle_t adc_handler4 = ACE_get_channel_handle((const uint8_t *)"ADCDirectInput_4");
	ace_channel_handle_t adc_handler5 = ACE_get_channel_handle((const uint8_t *)"ADCDirectInput_5");
	volatile uint16_t x = 0;
	volatile uint16_t y = 0;

	//Just a complex way to store an array and shift
	uint16_t xarr[] ={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
	uint16_t yarr[] ={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

	uint16_t alternator = 0;
	uint16_t checkPressCount = 0;

	while( 1 )
	{
		//x = getX(adc_handler4);
		//printf("X: %d  \n\r", x);

		//y = getY(adc_handler5);
		//printf("Y: %d  \n\r", y);

		if(alternator >100){
			alternator = 0;
			checkPressCount =0;
		}
		if(alternator>50){
			y = getY(adc_handler5);
			addNewVal(yarr, y);
		}
		else {
			x = getX(adc_handler4);
			addNewVal(xarr, x);
		}
		alternator++;
		if(checkPress(xarr) && checkPress(yarr)){
			checkPressCount ++;
		}
		if(checkPressCount >15){
			printf("X: %d   Y: %d\n\r", x, y);
		}
		else{
			printf("No press\n\r");
		}

	}
}
