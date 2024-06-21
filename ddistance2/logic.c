#include <libpynq.h>
#include <iic.h>
#include "vl53l0x.h"
#include <stdio.h>
#include <pthread.h>
#include <stepper.h>
#include "main.h"
#include <math.h>
#include <string.h>
#define BUFSIZE 128
#define M_PI 3.141592
// extern void *irsensor(void* obstacle);
// extern void *irsensor();
// mosquitto_sub -h mqtt.ics.ele.tue.nl -t /pynqbridge/19/send -u Student37 -P tohfe0aY

char buf[BUFSIZE] = {0};

//Runs c++ cube detection code, returns cube detection code output

struct Position {
    double x;
    double y;
    double direction; // in radians, 0 is North
};

// Initialize the robot's position
struct Position robot = {0, 0, M_PI};

// Function to convert degrees to radians
double degreesToRadians(double degrees) {
    return degrees * M_PI / 180.0;
}

double radiansToDegrees(double radians) {
    return radians * 180.0 / M_PI;
}

// Function to update the position after moving forward
void updatePositionForward(int steps, double direction) {
    robot.x += steps * sin(direction);
    robot.y += steps * cos(direction);
    float dirdeg = radiansToDegrees(robot.direction);

    printf("POSITION XY dir: %f %f %f\n", robot.x, robot.y, dirdeg);
}

// Function to update the direction after turning
void updateDirection(double degrees) {
    robot.direction += degreesToRadians(degrees);
    if (robot.direction >= 2 * M_PI) {
        robot.direction -= 2 * M_PI;
    } else if (robot.direction < 0) {
        robot.direction += 2 * M_PI;
    }
    float dirdeg = radiansToDegrees(robot.direction);
    printf("Updated direction!\n");
    printf("POSITION XY dir: %f %f %f\n", robot.x, robot.y, dirdeg);
}


char* parse_output(void) {
    char *cmd = "./opencv";    
    FILE *fp;
    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Error opening pipe!\n");
        return NULL;
    }
    while (fgets(buf, BUFSIZE, fp) != NULL) {
        printf("OUTPUT: %s", buf);
    }
    if (pclose(fp)) {
        printf("Command not found or exited with error status\n");
        return NULL;
    }
    return buf;
}

void forward(int speed, int steps) {
    printf("going forward\n");
    stepper_set_speed(speed, speed);
    stepper_steps(-steps, -steps);

    // int16_t leftSteps, rightSteps;
    // do {
    //     stepper_get_steps(&leftSteps, &rightSteps);
    // } while (leftSteps != 0 || rightSteps != 0);

    // Update the position
    updatePositionForward(steps/100, robot.direction);
    
}

//Maybe stop should only reset and then seperate function start that enables the stepper again?
void stop(void) {
    printf("stopping\n");
    stepper_reset();
    // sleep_msec(10);
    // stepper_enable();
}

void start(void) {
    printf("starting\n");
    stepper_enable();
}

void backward(int speed, int steps) { //waarom veranderen de coordinaten zo snel?
    printf("going backward\n");
    stepper_set_speed(speed, speed);
    stepper_steps(steps, steps);
    // int16_t leftSteps, rightSteps;
    // do {
    //     stepper_get_steps(&leftSteps, &rightSteps);
    // } while (leftSteps != 0 || rightSteps != 0);

    // Update the position
    updatePositionForward(-steps/100, robot.direction);
}

void right(int speed, int steps) {
    bool whole =false;
    if(steps == 615){ //90 Degrees
        whole = true;
    }
    //if steps = 615 its 90deg, else 45 deg
    printf("turning right\n");
    stepper_set_speed(speed, speed);
    stepper_steps(-steps, steps);

    // int16_t leftSteps, rightSteps;
    // do {
    //     stepper_get_steps(&leftSteps, &rightSteps);
    // } while (leftSteps != 0 || rightSteps != 0);

    // Update the direction

    whole ? updateDirection(90) : updateDirection(45);

}

void left(int speed, int steps) {
    // bool whole =false;
    // if(steps == 615){ //90 Degrees
    //     whole = true;
    // }

    printf("turning left\n");
    stepper_set_speed(speed, speed);
    stepper_steps(steps, -steps);
    
    // int16_t leftSteps, rightSteps;
    // do {
    //     stepper_get_steps(&leftSteps, &rightSteps);
    // } while (leftSteps != 0 || rightSteps != 0);

    // Update the direction
    updateDirection(-90); 
}

//Gets the output from cube detection code, transmits it via MQTT
void transmit(const char *output) {
    uint32_t length = strlen(output) - 1;
    printf("%d\n", length);
    uint8_t *bytes = (uint8_t*) &length;
    //Send the length of the message in bytes
    for(uint16_t i = 0; i < 4; i++) {
        uart_send(UART0, bytes[i]);
    }
    //Send the actual message
    for(uint16_t i = 0; i < length; i++) {
        uart_send(UART0, output[i]);
    }
    // uart_reset_fifos(UART0);
} //616

void logic(void *arguments) {
    
    //Sleep to wait until sensors have been initialized
    sleep_msec(2000);
    // stepper_disable();

    //Sensor A on top
    //Sensor B on bottom

    while(1) {
        //Gets data from sensors
        struct args *distances = arguments;

        printf("LOGIC %d %d %d\n", distances->disA, distances->disB, distances->obstacle);
        double degreesrobot = radiansToDegrees(robot.direction);
        printf("POSITION XY dir: %f %f %f\n", robot.x, robot.y, degreesrobot);

        //Stops and turns right if IR sensor detects black line.
        if(distances->obstacle == 1 && distances->disB > 80) {
            stop();
            printf("BLACK LINE1!\n");
            char holestring[200] = {};
            sprintf(holestring, "{\"x\": %lf, \"y\": %lf, \"type\": \"%s\", \"color\": \"%s\", \"size\": \"%s\"} ", robot.x, robot.y, "hole", "", "");
            printf("%s\n", holestring);
            transmit(holestring);
            sleep_msec(50);
            start();
            if(stepper_steps_done() == 1) {
                backward(40000, 500);
                sleep_msec(1000);
                right(40000, 315);
                sleep_msec(1000);
            }
        }

        //If both sensors sense a distance of more than 250 mm and no object
        if(distances->disA >= 150 && distances->disB >= 150) {
            distances = arguments;
            if(distances->obstacle == 1 && distances->disB > 80) {
                stop();
                printf("BLACK LINE2!\n");
                char holestring[200] = {};
                sprintf(holestring, "{\"x\": %lf, \"y\": %lf, \"type\": \"%s\", \"color\": \"%s\", \"size\": \"%s\"} ", robot.x, robot.y, "hole", "", "");
                printf("%s\n", holestring);
                transmit(holestring);
                sleep_msec(50);
                start();
                if(stepper_steps_done() == 1) {
                    backward(40000, 500);
                    sleep_msec(1000);
                    right(40000, 315);
                    sleep_msec(1000);
                }
            }
            // printf("LOGIC %d %d %d\n", distances->disA, distances->disB, distances->obstacle);
            //If the robot is not currently running, go forward.
            if(stepper_steps_done() == 1) {
                forward(40000, 2000);
                // right(20000, 300);
                // left(20000, 600);
            }
            // if(stepper_steps_done() == 1) {
            // }
            // if(stepper_steps_done() == 1) {
            // }
            
            // sleep_msec(10);
        }
        //Bottom sensor detects block
        if(distances->disB < 150 && distances->disA > distances->disB) {
            if(stepper_steps_done() == 1) {
                forward(40000, 500);
            }
            if(distances->obstacle == 1 && distances->disB > 80) {
                stop();
                printf("BLACK LINE3!\n");
                char holestring[200] = {};
                sprintf(holestring, "{\"x\": %lf, \"y\": %lf, \"type\": \"%s\", \"color\": \"%s\", \"size\": \"%s\"} ", robot.x, robot.y, "hole", "", "");
                printf("%s\n", holestring);
                transmit(holestring);
                sleep_msec(50);
                start();
                if(stepper_steps_done() == 1) {
                    backward(40000, 500);
                        sleep_msec(1000);
                        right(40000, 315);
                        sleep_msec(1000);
                }
            }
        }

        //Distance top sensor bigger than bottom sensor, and bottom sensor
        //less than or equal 50 mm
        //block
        if(distances->disB <= 50 && distances->disB > 10 && distances->disA > 120) {
            //If distance sensed on bottom camera is less than 50mm,
            //robot stops, takes a picture, sends the picture data
            //and backs up until the distance to the block is 100 mm.

            //// Subtracting remaining distance from added distance
            int16_t leftRemaining, rightRemaining;
             stepper_get_steps(&leftRemaining, &rightRemaining); //Check how many steps were remaining so you can subtract this from the forward direction coordinate
             //They should be ~ equal
             updatePositionForward(-leftRemaining/100, robot.direction);

            ////
            stop();
            sleep_msec(10);
            start();
            printf("TAKING PICTURE\n");
            char *output = parse_output();
            output[strcspn(output, "\n")] = '\0';
            char size[50];
            char color[50];
            char *token = strtok(output, " +");
            if (token != NULL) {
                strncpy(size, token, sizeof(size) - 1);
                size[sizeof(size) - 1] = '\0';
            }

            token = strtok(NULL, " +");
            if (token != NULL) {
                strncpy(color, token, sizeof(color) - 1);
                color[sizeof(color) - 1] = '\0';
            }

            if(strcmp(output, "unknown") != 0) {
                char string[200] = {};
                sprintf(string, "{\"x\": %lf, \"y\": %lf, \"type\": \"%s\", \"color\": \"%s\", \"size\": \"%s\"} ", robot.x, robot.y, "cube", color, size);
                printf("%s\n", string);
                transmit(string);
            }

            sleep_msec(1000);

            //While robot is close to block, back up until distance to block
            //is more than 100mm
            if(stepper_steps_done() == 1) {
                backward(40000, 500);
            }

            while(distances->disB < 100) {
                distances = arguments;
                // printf("LOGIC %d %d %d\n", distances->disA, distances->disB, distances->obstacle);
                if(stepper_steps_done() == 1) {
                    backward(40000, 1000);
                } //COMMENT THIS IF BACKWARDS GOES UP TOO QUICKLY
            }

            if(distances->obstacle == 1) {
                stop();
                printf("BLACK LINE4!\n");
                char holestring[200] = {};
                sprintf(holestring, "{\"x\": %lf, \"y\": %lf, \"type\": \"%s\", \"color\": \"%s\", \"size\": \"%s\"} ", robot.x, robot.y, "hole", "", "");
                printf("%s\n", holestring);
                transmit(holestring);
                sleep_msec(50);
                start();
                if(stepper_steps_done() == 1) {
                    backward(40000, 500);
                        sleep_msec(1000);
                        right(40000, 315);
                        sleep_msec(1000);
                }
            }
            stop();
            sleep_msec(100);
            start();
            //Turn right away from block
            right(40000, 615);
        }

        //If both sensors sense distance less than 150mm
        //Mountain
        if(distances->disA < 150 && distances->disB < 150) {
            distances = arguments;
            if(distances->obstacle == 1 && distances->disB > 80) {
                stop();
                printf("BLACK LINE5!\n");
                char holestring[200] = {};
                sprintf(holestring, "{\"x\": %lf, \"y\": %lf, \"type\": \"%s\", \"color\": \"%s\", \"size\": \"%s\"} ", robot.x, robot.y, "hole", "", "");
                printf("%s\n", holestring);
                transmit(holestring);
                sleep_msec(50);
                start();
                if(stepper_steps_done() == 1) {
                    backward(40000, 500);
                        sleep_msec(1000);
                        right(40000, 315);
                        sleep_msec(1000);
                }
            }
            // printf("LOGIC %d %d %d\n", distances->disA, distances->disB, distances->obstacle);
            //Go forward slowly until distance to mountain is less than 50mm
            if(stepper_steps_done() == 1) {
                forward(40000, 100);
            }
            if(distances->disA <= 80 && distances->disB <= 60) {
                // printf("LOGIC %d %d %d\n", distances->disA, distances->disB, distances->obstacle);
                if(distances->obstacle == 1 && distances->disB > 80) {
                    stop();
                    printf("BLACK LINE6!\n");
                    char holestring[200] = {};
                    sprintf(holestring, "{\"x\": %lf, \"y\": %lf, \"type\": \"%s\", \"color\": \"%s\", \"size\": \"%s\"} ", robot.x, robot.y, "hole", "", "");
                    printf("%s\n", holestring);
                    transmit(holestring);
                    sleep_msec(50);
                    start();
                    if(stepper_steps_done() == 1) {
                        backward(40000, 500);
                        sleep_msec(1000);
                        right(40000, 315);
                        sleep_msec(1000);
                    }
                }
                stop();
                printf("Mountain");
                char mountainstring[200] = {};
                sprintf(mountainstring, "{\"x\": %lf, \"y\": %lf, \"type\": \"%s\", \"color\": \"%s\", \"size\": \"%s\"} ", robot.x, robot.y, "mountain", "", "");
                printf("%s\n", mountainstring);
                transmit(mountainstring);
                sleep_msec(100);
                start();
                if(stepper_steps_done() == 1) {
                    backward(40000, 500);
                    sleep_msec(1000);
                    right(40000, 615);
                    sleep_msec(1000);
                }
            }
        }
        sleep_msec(10);
    }
}