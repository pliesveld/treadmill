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
#include <signal.h>
#include <chrono>

#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <iostream>
#include <string>
#include <vector>

using std::vector;
using std::string;
using std::cout;
using std::endl;

using namespace std::chrono;

#include "../include/influxdb.hpp"


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

void sensor_main_loop();
float getSonar();
int pulseIn(int pin, int level, int timeout);
unsigned long long int timestamp_now();

void record_sample(float distance);
vector<string> dns_lookup(const string &host_name, int ipv=4);

static void register_signal_handlers();
static void onterm(int s);
static void onexit(void);

int main()
{
	printf("Program is starting ... \n");
	register_signal_handlers();
	sensor_main_loop();
	return 1;
}

/*
 * main loop triggers the HC-SR04's sonar scan, and measures the distance.
 * HC-SR04 occasionally returns a distance value that is a false positive.
 * Therefore, the main loop checks for 6 readings sequentially before toggling the state.
 */
void sensor_main_loop()
{
	wiringPiSetup();

	pinMode(LED_PIN_NO, OUTPUT); //Set the pin mode of led
	pinMode(HC_SR04_TRIGGER_PIN_NO, OUTPUT); //Set the pin mode of the trigger pin
	pinMode(HC_SR04_ECHO_PIN_NO, INPUT);  //Set the pin mode of the echo pin

	int sampled_level = LOW;
	int current_level = LOW;
	int count = 0;

	int i = 0;

	while (1) {
		float distance = getSonar();
		printf("The distance to: %.2f cm\n", distance);

		if (distance >= 0.0001 && distance <= 160.000) {
			sampled_level = HIGH;
		} else {
			sampled_level = LOW;
		}

		if (current_level != sampled_level) {
			count++;
		} else {
			count = 0;
		}

		if (count == 6) {
			current_level = sampled_level;
			count = 0;
			printf("current_level = %d\n", current_level);
		}

		digitalWrite(LED_PIN_NO, current_level);  //Make GPIO output HIGH level

		if(current_level == HIGH) {
			record_sample(distance);
		}

		delay(1000);
	}
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
			return 0;
		}
	}

	// Calculate length of pulse signal
	if (tn.tv_sec > t1.tv_sec) micros = 1000000L; else micros = 0;
	micros = micros + (tn.tv_usec - t1.tv_usec);
	return micros;
}


influxdb_cpp::server_info si(dns_lookup("bigpi3").at(0), 8086, "test_db", "", "", "ms");


void create_database() {
// CREATE DATABASE test_db
// CREATE RETENTION POLICY "24hours" ON "test_db" DURATION 24h REPLICATION 1 DEFAULT
}

unsigned long long int timestamp_now()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() * 1000;
}


void record_sample(float distance)
{
	int resp = influxdb_cpp::builder()
		.meas("body")
		.tag("position", "center")
		.field("distance", distance)
		.timestamp(timestamp_now())
		.post_http(si);
	printf("resp = %d\n",resp);
}



vector<string> dns_lookup(const string &host_name, int ipv)
{
	vector<string> output;

	struct addrinfo hints, *res, *p;
	int status, ai_family;
	char ip_address[INET6_ADDRSTRLEN];

	ai_family = ipv==6 ? AF_INET6 : AF_INET; //v4 vs v6?
	ai_family = ipv==0 ? AF_UNSPEC : ai_family; // AF_UNSPEC (any), or chosen
	memset(&hints, 0, sizeof hints);
	hints.ai_family = ai_family;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(host_name.c_str(), NULL, &hints, &res)) != 0) {
		//cerr << "getaddrinfo: "<< gai_strerror(status) << endl;
		return output;
	}

	//cout << "DNS Lookup: " << host_name << " ipv:" << ipv << endl;

	for(p = res;p != NULL; p = p->ai_next) {
		void *addr;
		if (p->ai_family == AF_INET) { // IPv4
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
		} else { // IPv6
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			addr = &(ipv6->sin6_addr);
		}

		// convert the IP to a string
		inet_ntop(p->ai_family, addr, ip_address, sizeof ip_address);
		output.push_back(ip_address);
	}

	freeaddrinfo(res); // free the linked list

	return output;
}

static void register_signal_handlers() {
	signal(SIGTERM, onterm);
	signal(SIGABRT, onterm);
	signal(SIGINT, onterm);
	signal(SIGQUIT, onterm);
	atexit(onexit);
}
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
