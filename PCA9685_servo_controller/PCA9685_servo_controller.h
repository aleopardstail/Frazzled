#pragma once

#ifndef PCA9685_servo_controller_h
#define PCA9685_servo_controller_h

#include <Arduino.h>
#include <Wire.h>

class PCA9685_servo_controller
{
public:
	// constructor, pass the device I2C address
	PCA9685_servo_controller(uint8_t i2c_address);	
	void begin(void);
	void writeRegister(uint8_t reg, uint8_t value);
	uint8_t readRegister(uint8_t reg);
	void setFrequency(uint16_t frequency);
	void setPosition(uint8_t channel, int8_t angle);			// -90 to +90
	void setPWM(uint8_t channel, int16_t PWM);
	long map(long x, long in_min, long in_max, long out_min, long out_max);
	
private:
	uint8_t _i2c_address;
};

#endif