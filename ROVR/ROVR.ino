#include "esp_camera.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <iostream>
#include <sstream>
#include <ESP32Servo.h>

//servos to control cams
#define SERVO1_PIN 14
#define SERVO2_PIN 15

// control speed of the wheels via PWM
#define SPEED_PIN 2


// speed of the motor
int motor_speed = 1000; // TODO: change this initial value???
int speed_resolution = 8; //TODO: need to change this too???


Servo left_right_servo;
Servo up_down_servo;



void setGPIO(){
  //for the camera-control servos
  left_right_servo.attach(SERVO1_PIN);
  up_down_servo.attach(SERVO2_PIN);

  //set up the PWM for the wheels
  ledcsetup(SPEED_PIN, motor_speed, speed_resolution);

  




}