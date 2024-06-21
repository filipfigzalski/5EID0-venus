#include <libpynq.h>
#include <iic.h>
#include "vl53l0x.h"
#include <stdio.h>
#include "main.h"

void vl53l0x_dual(void *arguments) {

    //Code to turn sensors on/off
    gpio_set_level(IO_A0, GPIO_LEVEL_LOW);
    gpio_set_level(IO_A1, GPIO_LEVEL_LOW);
    sleep_msec(10);
    gpio_set_level(IO_A0, GPIO_LEVEL_HIGH);
    gpio_set_level(IO_A1, GPIO_LEVEL_HIGH);
    sleep_msec(10);
    gpio_set_level(IO_A0, GPIO_LEVEL_LOW);
    gpio_set_level(IO_A1, GPIO_LEVEL_LOW);
    sleep_msec(10);

    //Turn distance sensor A on
    gpio_set_level(IO_A0, GPIO_LEVEL_HIGH);
    sleep_msec(100);

	int i;
	//Setup Sensor A
	printf("Initialising Sensor A:\n");

	//Change the address of the VL53L0X
	uint8_t addrA = 0x69;
	i = tofSetAddress(IIC0, 0x29, addrA);
	printf("---Address Change: ");
	if(i != 0) {
		printf("%d Fail\n", i);
		return;
	}
	printf("Succes\n");
	
	i = tofPing(IIC0, addrA);
	printf("---Sensor Ping: ");
	if(i != 0) {
		printf("Fail\n");
		return;
	}
	printf("Succes\n");

	//Create a sensor struct
	vl53x sensorA;

	//Initialize sensor A
	i = tofInit(&sensorA, IIC0, addrA, 0);
	if (i != 0) {
		printf("---Init: Fail\n");
		return;
	}

	uint8_t model, revision;

	tofGetModel(&sensorA, &model, &revision);
	printf("---Model ID - %d\n", model);
	printf("---Revision ID - %d\n", revision);
	printf("---Init: Succes\n");
	fflush(NULL);

   
	//Setup Sensor B
	printf("Initialising Sensor B:\n");
	gpio_set_level(IO_A1, GPIO_LEVEL_HIGH);

    sleep_msec(100);

	//Use the base addr of 0x29 for sensor B
	//It no longer conflicts with sensor A.
	uint8_t addrB = 0x29;	
	i = tofPing(IIC0, addrB);
	printf("---Sensor Ping: ");
	if(i != 0) {
		printf("Fail\n");
		//exit();
		return;
	}
	printf("Succes\n");

	//Create a sensor struct
	vl53x sensorB;

	//Initialize the sensor
	i = tofInit(&sensorB, IIC0, addrB, 0);
	if (i != 0) {
		printf("---Init: Fail\n");
		return;
	}

	tofGetModel(&sensorB, &model, &revision);
	printf("---Model ID - %d\n", model);
	printf("---Revision ID - %d\n", revision);
	printf("---Init: Succes\n");
	fflush(NULL); //Get some output even if the distance readings hang
	printf("\n");

    //Create struct where measurements are stored
    struct args *argumentss = arguments;

	int j = 0;

	while(1) {
        //Store sensor measurements in struct
		argumentss->disA = tofReadDistance(&sensorA);
		argumentss->disB = tofReadDistance(&sensorB);

        uint64_t interruptPin = gpio_get_interrupt();
        j++;
        if (j == 2) j = 0;
        if (interruptPin == 512 && j == 1) {
            //Store IR sensor measurement in struct
            argumentss->obstacle = 1;
            gpio_set_level(IO_LD0, 1);
        }

        gpio_set_level(IO_LD0, 0);
        gpio_ack_interrupt();
		sleep_msec(210);
		argumentss->obstacle = 0;
	}

	iic_destroy(IIC0);
	pynq_destroy();
	return;
}