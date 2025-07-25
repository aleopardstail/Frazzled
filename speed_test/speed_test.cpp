// speed_test.cpp

#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <cstring>
#include <chrono>
#include <unistd.h>
#include "cred.h"

int main(int, char **);
void connect_callback(struct mosquitto *mosq, void *obj, int result);
void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);

int currentState=0;			// state machine, 0 = waiting to start, 1 = waiting to finish, 2 = waiting to reset

uint64_t startTime;
uint64_t stopTime;
uint64_t elapsedTime;
const double distance = 92.9;		// measured distance (scale m)

int main(int argc, char **argv)
{
	
	char clientid[24];
	struct mosquitto *mosq;
	int rc = 0;
	
	
	
	printf("\nSpeed Test\nctrl+n to quit\n\n");
	
	mosquitto_lib_init();
	
	memset(clientid, 0, 24);
	snprintf(clientid, 23, "speed_test_%d", getpid());
	printf("Client ID: %s\n", clientid);
	mosq = mosquitto_new(clientid, true, 0);
	if(mosq)
	{
		mosquitto_connect_callback_set(mosq, connect_callback);
		mosquitto_message_callback_set(mosq, message_callback);
		mosquitto_username_pw_set(mosq, MQTTBrokerUserName, MQTTBrokerPassword);
		
		rc = mosquitto_connect(mosq, "192.168.0.39", 1883, 60);
		
		mosquitto_subscribe(mosq, NULL, "track/sensor/GBlock2", 0);			// start sensor
		mosquitto_subscribe(mosq, NULL, "track/sensor/HBlock4", 0);			// stop sensor
		mosquitto_subscribe(mosq, NULL, "track/sensor/FBlock2", 0);			// reset sensor
		
		
	}	
	
	while(1)
	{
		mosquitto_loop(mosq, 0, 1);
	}
	
	return EXIT_SUCCESS;
}

// MQTT stuff
void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	printf("connect callback, rc=%d\n", result);
}

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	switch(message->topic[6])
	{		
		case 's':
		{			
			// start sensor is track/sensor/GBlock2 going ACTIVE
			// stop sensor is track/sensor/HBlock4 going ACTIVE
			
			char Buffer[20] = {0};
			sprintf(Buffer, "%s", (char *) message->payload);
			
			switch(currentState)
			{
				case 0:		// waiting to start
				{
					if (message->topic[13] == 'G' & Buffer[0] == 'A')
					{
						// start the timer
						startTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
						
						printf("Start!\n");
						currentState=1;
					}
					break;
				}
				case 1:		// waiting to end
				{
					if (message->topic[13] == 'H' & Buffer[0] == 'A')
					{
						// stop the timer
						stopTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
						elapsedTime = stopTime - startTime;		// time in ms
						
						// calculate the speed
						double T = (double) elapsedTime / 1000;
						double S = (distance / T) * 2.23;
						
						printf("Stop!   %.2f sec\t %.1f mph\n", T, S);
						currentState=2;
					}
					break;
				}
				case 2:		// waiting to reset
				{
					if (message->topic[13] == 'F' & Buffer[0] == 'A')
					{
						printf("Ready for the next run!\n");
						currentState=0;
					}
					break;
				}
			}			
			break;
		}
	}
}
