/**********************************************************************
* Filename    : UltrasonicRanging.c
* Description : Get distance via UltrasonicRanging sensor
* Author      : www.freenove.com
* modification: 2019/12/27
**********************************************************************/
#include <wiringPi.h>
#include <stdio.h>
#include <sys/time.h>

#define  ledPin    0	//define the led pin number
#define trigPin 4       
#define echoPin 5

#define MAX_DISTANCE 220        // define the maximum measured distance
#define timeOut MAX_DISTANCE*60 // calculate timeout according to the maximum measured distance
//function pulseIn: obtain pulse time of a pin

int pulseIn(int pin, int level, int timeout);
float getSonar(){   //get the measurement result of ultrasonic module with unit: cm
    long pingTime;
    float distance;
    digitalWrite(trigPin,HIGH); //send 10us high level to trigPin 
    delayMicroseconds(10);
    digitalWrite(trigPin,LOW);
    pingTime = pulseIn(echoPin,HIGH,timeOut);   //read plus time of echoPin
    distance = (float)pingTime * 340.0 / 2.0 / 10000.0; //calculate distance with sound speed 340m/s
    return distance;
}

int main(){
    printf("Program is starting ... \n");
    
    wiringPiSetup();
    
    float distance = 0;
    pinMode(ledPin, OUTPUT); //Set the pin mode of led
    pinMode(trigPin,OUTPUT); //Set the pin mode of the trigger pin
    pinMode(echoPin,INPUT);  //Set the pin mode of the echo pin


    int prev = LOW;
    int desired = LOW;
    int count = 0;


    while(1){
        distance = getSonar();
        printf("The distance is : %.2f cm\n",distance);

	if(distance >= 0.0001 && distance <= 160.000) {
		prev = HIGH;
	} else {
		prev = LOW;
	}

	if(desired != prev) {
		count++;
	} else {
		count = 0;
	}

	if(count == 6) {
		desired = prev;
		printf("Desired = %d\n", desired);
	}

	digitalWrite(ledPin, desired);  //Make GPIO output HIGH level

        delay(1000);
    }   
    return 1;
}

int pulseIn(int pin, int level, int timeout)
{
   struct timeval tn, t0, t1;
   long micros;
   gettimeofday(&t0, NULL);
   micros = 0;
   while (digitalRead(pin) != level)
   {
      gettimeofday(&tn, NULL);
      if (tn.tv_sec > t0.tv_sec) micros = 1000000L; else micros = 0;
      micros += (tn.tv_usec - t0.tv_usec);
      if (micros > timeout) return 0;
   }
   gettimeofday(&t1, NULL);
   while (digitalRead(pin) == level)
   {
      gettimeofday(&tn, NULL);
      if (tn.tv_sec > t0.tv_sec) micros = 1000000L; else micros = 0;
      micros = micros + (tn.tv_usec - t0.tv_usec);
      if (micros > timeout) return 0;
   }
   if (tn.tv_sec > t1.tv_sec) micros = 1000000L; else micros = 0;
   micros = micros + (tn.tv_usec - t1.tv_usec);
   return micros;
}
