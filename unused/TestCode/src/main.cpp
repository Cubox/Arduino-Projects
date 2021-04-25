#include <Arduino.h>
// Define pin connections & motor's steps per revolution
const int dirPin = 2;
const int stepPin = 3;
const int stepsPerRevolution = 200;

void setup()
{
	// Declare pins as Outputs
	pinMode(stepPin, OUTPUT);
	pinMode(dirPin, OUTPUT);
}
void loop()
{
	// Set motor direction clockwise
	digitalWrite(dirPin, HIGH);

	// Spin motor slowly
	for(int x = 0; x < stepsPerRevolution; x++)
	{
		digitalWrite(stepPin, HIGH);
		delay(5);
		digitalWrite(stepPin, LOW);
		delay(5);
	}
	delay(1000); // Wait a second

  	digitalWrite(dirPin, LOW);

	// Spin motor slowly
	for(int x = 0; x < stepsPerRevolution; x++)
	{
		digitalWrite(stepPin, HIGH);
		delay(5);
		digitalWrite(stepPin, LOW);
		delay(5);
	}
	delay(1000); // Wait a second
	
}