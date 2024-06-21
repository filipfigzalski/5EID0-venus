#include <libpynq.h>
#include <iic.h>
#include "vl53l0x.h"
#include <stdio.h>
#include <pthread.h>
#include <stepper.h>
#include "main.h"

extern void *vl53l0x_dual(void *arguments);
extern void *logic(void *arguments);

int main(void) {
    //Init PYNQ
  	pynq_init();

    //Create two thread variables
    pthread_t thread1, thread2;

    //Distance sensors init
        //Switchbox
    switchbox_init();
	switchbox_set_pin(IO_AR_SCL, SWB_IIC0_SCL);
	switchbox_set_pin(IO_AR_SDA, SWB_IIC0_SDA);
        //IIC
	iic_init(IIC0);
        //GPIO
    gpio_init();
    gpio_set_direction(IO_A0, GPIO_DIR_OUTPUT);
    gpio_set_direction(IO_A1, GPIO_DIR_OUTPUT);
    gpio_set_direction(IO_AR2, GPIO_DIR_INPUT);
    gpio_set_direction(IO_AR3, GPIO_DIR_INPUT);

    //Communication init
        //Switchbox
    switchbox_set_pin(IO_AR0, SWB_UART0_RX);
    switchbox_set_pin(IO_AR1, SWB_UART0_TX);
        //UART
    uart_init(UART0);
    uart_reset_fifos(UART0);
    // printf("Reset fifos!\n");

    //Stepper init
	stepper_init();
    stepper_enable();
    stepper_set_speed(5000,5000);

    //IR init
        //GPIO
    gpio_interrupt_init();
    gpio_set_direction(IO_LD0, GPIO_DIR_OUTPUT);
    gpio_enable_interrupt(IO_AR9);
    // gpio_disable_all_interrupts();
    // gpio_ack_interrupt();

    struct args arguments;
    arguments.disA = 0;
    arguments.disB = 0;
    arguments.obstacle = 0;

    //Creates thread which runs the ToF sensor + IR sensor code
    pthread_create(&thread1, NULL, vl53l0x_dual, (void*) &arguments);
    //Creates thread which runs logic code
    pthread_create(&thread2, NULL, logic, (void*) &arguments);

    //Terminate threads when code is done
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

	iic_destroy(IIC0);
	pynq_destroy();
	return EXIT_SUCCESS;
}