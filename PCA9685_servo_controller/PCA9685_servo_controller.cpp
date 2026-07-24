#include "PCA9685_servo_controller.h"

PCA9685_servo_controller::PCA9685_servo_controller( uint8_t i2c_address)
{
	_i2c_address = i2c_address;
	
	return;
}

void PCA9685_servo_controller::begin(void)
{	
	// configure the PCA9685 for driving servos
	writeRegister(0x00, 0b10100000);
	writeRegister(0x01, 0b00000100);
	setFrequency(50);
	
	// centre all Servos initially
	for (uint8_t C = 0; C < 16; C++)
	{
		setPosition(C, 0);
	}
	return;
}

void PCA9685_servo_controller::writeRegister(uint8_t reg, uint8_t value)
{
	Wire.beginTransmission(_i2c_address);
	Wire.write(reg);
	Wire.write(value);
	Wire.endTransmission();
	
	return;
}

uint8_t PCA9685_servo_controller::readRegister(uint8_t reg)
{
	Wire.beginTransmission(_i2c_address);
	Wire.write(reg);
	Wire.endTransmission();
	
	Wire.requestFrom(_i2c_address, (uint8_t)1);
	uint8_t D = Wire.read();
		
	return D;
}

void PCA9685_servo_controller::setFrequency(uint16_t Frequency)
{
	int preScalerVal = (25000000 / (4096 * Frequency)) - 1;
    if (preScalerVal > 255) preScalerVal = 255;
    if (preScalerVal < 3) preScalerVal = 3;

	//need to be in sleep mode to set the pre-scaler
	uint8_t M1 = readRegister(0x00);
	writeRegister(0x00, ((M1 & ~0b10000000) | 0b00010000));
	writeRegister(0xFE, (uint8_t)preScalerVal);

	// restart
	writeRegister(0x00, ((M1 & ~0b00010000) | 0b10000000));
	delay(1);
	return;
}

// set a position based on a supplied angle
void PCA9685_servo_controller::setPosition(uint8_t channel, int8_t angle)
{
	// PWM range is 0-4096 total, however to drive a servo we use a fraction
	// of this range, the pulse length for a servo at -90 should be 1ms
	// for centred 1.5ms and for +90 2ms
	
	// we have a frequency of 50Hz, so a pulse frequency of one pulse every
	// 20ms
	
	// our minimum value should be:
	// 
	
	// note we apply an offset to stagger the pulse within the total window, basically
	// so we only adjust a limited number at one to lower the current draw
	
	// map the angle to a PWM value (-90 to +90) to the servo
	// PWM_min & PWM_max values
	uint16_t PWM_min = 40; //104;		// 148;
	uint16_t PWM_max = 250;	// 508;
	
	uint16_t PWM = (uint16_t) this->map(angle, -90, 90, PWM_min, PWM_max);
	uint16_t ChannelOffset = channel * 10;		// adds 0-160 to the counter values
	
	
	uint8_t D[5];
	
	uint16_t ChannelOn = 0 + ChannelOffset;
	uint16_t ChannelOff = PWM + ChannelOffset;
	
	D[0] = 0x06 + (4 * channel);
	D[1] = (0x00FF & ChannelOn);
	D[2] = (0xFF00 & ChannelOn) >> 8;
	D[3] = (0x00FF & ChannelOff);
	D[4] = (0xFF00 & ChannelOff) >> 8;
	
	Wire.beginTransmission(_i2c_address);
	Wire.write(D[0]);
	Wire.write(D[1]);
	Wire.write(D[2]);
	Wire.write(D[3]);
	Wire.write(D[4]);
	Wire.endTransmission();
	
	
	return;
}

// set a position based on a supplied PWM value (assumes "on" is at zero, offset applied here)
void PCA9685_servo_controller::setPWM(uint8_t channel, int16_t PWM)
{
	uint8_t D[5];
	
	uint16_t ChannelOffset = channel * 10;		// adds 0-160 to the counter values

	uint16_t ChannelOn = 0 + ChannelOffset;
	uint16_t ChannelOff = PWM + ChannelOffset;
	
	D[0] = 0x06 + (4 * channel);
	D[1] = (0x00FF & ChannelOn);
	D[2] = (0xFF00 & ChannelOn) >> 8;
	D[3] = (0x00FF & ChannelOff);
	D[4] = (0xFF00 & ChannelOff) >> 8;
	
	Wire.beginTransmission(_i2c_address);
	Wire.write(D[0]);
	Wire.write(D[1]);
	Wire.write(D[2]);
	Wire.write(D[3]);
	Wire.write(D[4]);
	Wire.endTransmission();
	
	return;
}

// useful utility function
long PCA9685_servo_controller::map(long x, long in_min, long in_max, long out_min, long out_max)
{
	return (x - in_min) * (out_max - out_min)/(in_max - in_min) + out_min;
}