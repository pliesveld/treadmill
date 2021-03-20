/**
 * Activity sensor mounted to treadmill to track usage.
 *
 * The software requires the hardware Raspberry Pi, HC-SR04
 * TODO: see README.md for hardware instructions
 *
 * Source code has been modified from: https://github.com/Freenove/Freenove_Ultimate_Starter_Kit_for_Raspberry_Pi
 * Tutorials and kits available at: https://freenove.com/tutorial
 *
 *
 */
/**********************************************************************
* Filename    : UltrasonicRanging.c
* Description : Get distance via UltrasonicRanging sensor
* Author      : www.freenove.com
* modification: 2019/12/27
**********************************************************************/
#include <wiringPi.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include "signal.h"

/*
 * The following pins use the wiriingPi order numbering of the raspberry Pi. See README.md for more details.
 * TODO: replace #define with runtime parameter. perhaps environment variable?
 */
#define LED_PIN_NO  0
#define HC_SR04_TRIGGER_PIN_NO 4
#define HC_SR04_ECHO_PIN_NO 5

#define MAX_DISTANCE 220        // define the maximum measured distance
//#define MAX_TIMEOUT MAX_DISTANCE*60 // calculate timeout according to the maximum measured distance
#define MAX_TIMEOUT 100000L


static void onterm(int s)
{
	printf("onterm\n");
	exit(s != SIGHUP  && s != SIGINT && s != SIGQUIT && s != SIGABRT && s != SIGTERM ? EXIT_SUCCESS : EXIT_FAILURE);
}

static void onexit(void)
{
	//Nicely shut down and application specific things
	printf("onexit\n");
}

static void register_signal_handlers() {
	signal(SIGTERM, onterm);
	signal(SIGABRT, onterm);
	signal(SIGINT, onterm);
	signal(SIGQUIT, onterm);
	atexit(onexit);
}

/*
 * Measure the the time (in us) of the pulse.
 */
int pulseIn(int pin, int level, int timeout)
{
	struct timeval tn, t0, t1;
	long micros = 0;

	// poll pin for start of pulse with a timeout

	gettimeofday(&t0, NULL);
	while (digitalRead(pin) != level) {
		gettimeofday(&tn, NULL);
		if (tn.tv_sec > t0.tv_sec) micros = 1000000L; else micros = 0;
		micros += (tn.tv_usec - t0.tv_usec);
		if (micros > timeout) {
			return 0;
		}
	}

	// signal change on pin observed. Now poll pin for falling pulse with a timeout
	// If the HC-SR04 does not receive an echo then the output never goes low.

	gettimeofday(&t1, NULL);
	while (digitalRead(pin) == level) {
		gettimeofday(&tn, NULL);
		if (tn.tv_sec > t0.tv_sec) micros = 1000000L; else micros = 0;
		micros = micros + (tn.tv_usec - t0.tv_usec);
		if (micros > timeout) {
//			printf("timeout in high\n");
			return 0;
		}
	}

	// Calculate length of pulse signal
	if (tn.tv_sec > t1.tv_sec) micros = 1000000L; else micros = 0;
	micros = micros + (tn.tv_usec - t1.tv_usec);
	return micros;
}

/*
 * Triggers the HC_SR04 hardware to begin the scan, and then calls pulseIn to measure
 * the latency on the echo pin.  Distance is computed using the speed of sound in the forumula.
 *
 */
float getSonar()
{   //get the measurement result of ultrasonic module with unit: cm
	long pingTime;
	float distance;

	digitalWrite(HC_SR04_TRIGGER_PIN_NO, HIGH); //send 10us high level to HC_SR04_TRIGGER_PIN_NO
	delayMicroseconds(10);
	digitalWrite(HC_SR04_TRIGGER_PIN_NO, LOW);

	pingTime = pulseIn(HC_SR04_ECHO_PIN_NO, HIGH, MAX_TIMEOUT);   //read pulse time of HC_SR04_ECHO_PIN_NO

	distance = (float)pingTime * 340.0 / 2.0 / 10000.0; //calculate distance with sound speed 340m/s
	return distance;
}

int main()
{
	printf("Program is starting ... \n");
	register_signal_handlers();
	
	wiringPiSetup();

	pinMode(LED_PIN_NO, OUTPUT); //Set the pin mode of led
	pinMode(HC_SR04_TRIGGER_PIN_NO, OUTPUT); //Set the pin mode of the trigger pin
	pinMode(HC_SR04_ECHO_PIN_NO, INPUT);  //Set the pin mode of the echo pin

	int prev = LOW;
	int desired = LOW;
	int count = 0;


	while (1) {
		float distance = getSonar();
		printf("The distance to my fat ass: %.2f cm\n", distance);

		if (distance >= 0.0001 && distance <= 160.000) {
			prev = HIGH;
		} else {
			prev = LOW;
		}

		if (desired != prev) {
			count++;
		} else {
			count = 0;
		}

		if (count == 6) {
			desired = prev;
			printf("Desired = %d\n", desired);
		}

		digitalWrite(LED_PIN_NO, desired);  //Make GPIO output HIGH level

		delay(1000);
	}
	return 1;
}
