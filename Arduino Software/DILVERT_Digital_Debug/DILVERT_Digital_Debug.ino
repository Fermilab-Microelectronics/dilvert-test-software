//makeinclude GPIO_user.cpp DO NOT MODIFY THIS LINE
#include "GPIO_user.h"
#include "generic_hal.h"


/* DILVERT Digital Debug HAL 
 * 12/16/2022 - Adam Quinn
 *
 * This HAL performs checks to help debug communications w/ DILVERT Digital.
 *
 * (1) SWEEP CHECK 
 * If SWEEP_CHECK = True, the Arduino will perform a scan chain loopback test
 * with 1,000 test vectors and check for any bit flips / bit errors. 
 * If failures are detected, the program will halt here and idle until reset.
 * This means your communication with the Scan Chain is not working. 
 *
 * (2) Readout Sequence Check
 * Next, it will run the readout sequence once for every time you enter a character over serial. 
 *
 */ 

//CONTROL SWITCHES ***************************************************
#define PORTENTA 1
#define BAUD 230400
#define cycle_time_us 10000
#define INTERACTIVE 1
//#define DEBUG 0
//#define GET_CAPTURE_TIME

//Define which digital pins are used for which signals. **************

#ifdef PORTENTA
//Portenta H7
// (Commented out definitions are unnecessary b/c pins are hardcoded.)
//#define S_CLK PC6/D5
//#define S_DIN PC7/D4
//#define S_DOUT PG7/D3
#define S_LOAD 2    //(PJ11/D2)
#define S_PASS 1    //(PK1/D1)
#define RESET 0     //(PH15/D0)
#define TRIGGER 21  //(PA4/D21)
#else
//Sparkfun Pro Micro
//DON'T USE THESE UNLESS YOU ARE USING THE SPARKFUN PRO MICRO
//#define S_CLK 10
//#define S_DIN 9
//#define S_DOUT 8
//#define S_LOAD 4
//#define S_PASS 5
//#define RESET 6
//#define TRIGGER 15
#endif

//GLOBAL VARIABLES ***********************************************

long capture_time_us;
long last_capture_time_us;

int serialByte = 0;

int single_acquisition = 0;

bool SWEEP_CHECK = true;
long sweep_check_max = 0;
bool sweep_check_pass = true;
int sweep_check_fail_count = 0;

int big_jump_count = 0;

uint16_t qcoarse_buffer[1000];
uint8_t qfine_buffer[1000];

// vvv EDIT INPUT ARRAY HERE vvv
//Arrays to hold input and output data.
char in_data[25] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  //QCOARSE
                     0, 0, 0, 0, 0, 0, 0,           //QFINE
                     0, 1,                          //DOUT1_select (Actually DOUT2_select for Rev1 PCB)
                     0, 1,                          //DOUT2_select (Actually DOUT1_select for Rev1 PCB)
                     1,                             //CAL1 (default:1)
                     0,                             //CAL0 (default:0)
                     1, 1 };                        //DRV

// ^^^ EDIT INPUT ARRAY HERE ^^^

char out_data[25];
char out_data_2[25];

void setup() {

  Serial.begin(BAUD);

  //Initialize out_data to zero.
  for (int i = 0; i < 25; i++) {
    out_data[i] = 0;
    out_data_2[i] = 0;
  }

#ifdef PORTENTA
  //On the Portenta, we use GPIO_Config to configure the pins for the Serial Interface.
  //The lowest five pins in the GPIO table are PG7 (SDOUT), PC7 (SDIN), PC6 (SCLK), PA8, PA10,
  //so if we want to configure SCLK and SDIN as outputs, we should pass 0b01100.
  GPIO_Config(0b01100, 0b0);
#else
  pinMode(S_CLK, OUTPUT);
  pinMode(S_DOUT, INPUT);  //S_DOUT is an OUTPUT of the ASIC, so it's an INPUT of the Arduino.
  pinMode(S_DIN, OUTPUT);
#endif

  pinMode(S_LOAD, OUTPUT);
  pinMode(S_PASS, OUTPUT);
  pinMode(RESET, OUTPUT);

  pinMode(TRIGGER, INPUT);

  Serial.println("DILVERT HAL Initialized");
}

void loop() {


  //~ ~ ~ SETUP PHASE ~ ~ ~

  //Make sure these control signals start low. Especially important if we're
  //reseting after a failed handshake.
  digitalWrite(S_PASS, LOW);
  digitalWrite(S_LOAD, LOW);
  digitalWrite(RESET, LOW);

  clock_pulse_portenta_C6(RESET, 100);  //Initial Reset of everything

  delay(10);




  //TEST ONE: Scan Chain Sweep
  if (SWEEP_CHECK) {
    sweep_check_pass = true;
    sweep_check_fail_count = 0;
    for (long i = 0; i < 1000; i++) {
      //in_data contains a bitwise representation of i.
      for (int j = 0; j < 10; j++) {
        in_data[j] = (i >> j) & 1;
      }
      for (int j = 10; j < 20; j++) {
        in_data[j] = (i >> (j - 10)) & 1;
      }
      Serial.print("Writing Data: \t");
      print_data(in_data, 25);
      Serial.println("");

      serial_write_portenta_C6_C7(in_data, 25, 10e3);  //Write in_data at 10kHz.

      clock_pulse_portenta_C6(S_PASS, 100);  //Pass control bits into the TDC internal register.

      serial_read_portenta_C6_G7_max(out_data, 25);  //Read data at operational speed

      Serial.print("Read back data:\t");
      print_data(out_data, 25);
      Serial.println("");


      //Initialize out_data to zero.
      for (int k = 0; k < 25; k++) {
        if (out_data[k] != in_data[k]) {
          Serial.print("Mismatch at bit ");
          Serial.print(k);
          Serial.print("; ");
          sweep_check_pass = false;
          sweep_check_fail_count++;
        }
      }
    }
    Serial.print("Fail Count: ");
    Serial.print(sweep_check_fail_count);
    Serial.println("/1000");

    if (!sweep_check_pass) {
      Serial.println("Sweep Check Failed :(");
      await_reset();
      return;
    } else {
      Serial.println("Sweep Check Passed :)");
      while (Serial.available()) Serial.read();
      while (!Serial.available())
        ;  //Wait for Serial
    }
  } else {
    //Simply write in the control bits with no frills.
    serial_write_portenta_C6_C7(in_data, 25, 10e3);  //Write in_data at 10kHz.

    clock_pulse_portenta_C6(S_PASS, 100);  //Pass control bits into the TDC internal register.
  }


  //TEST TWO: Readout Sequencing

  big_jump_count = 0;
  digitalWrite(S_PASS, HIGH);

  for (long i = 0; i < 1000; i++) {

    wait_on_rising_edge(TRIGGER);

    delayMicroseconds(5);
    digitalWrite(S_PASS, LOW);

    //delayMicroseconds(10);

    clock_pulse_portenta_C6(S_LOAD, 0);

    //delayMicroseconds(10);


    //Read out out_data #1
    serial_read_portenta_C6_G7_max(out_data, 25);

    //delayMicroseconds(10);

    clock_pulse_portenta_C6(S_LOAD, 0);

    //delayMicroseconds(10);


    //4) Read out out_data #2
    serial_read_portenta_C6_G7_max(out_data_2, 25);

    digitalWrite(RESET, HIGH);
    delayMicroseconds(1);
    digitalWrite(RESET, LOW);


    digitalWrite(S_PASS, HIGH);

    //PRINT out_data1 AND out_data2 TO SERIAL CONSOLE:
    Serial.print("out_data1:\t");
    print_data(out_data, 25);
    Serial.println();

    qcoarse_buffer[i] = bin_array_to_dec(&out_data[0], 10);
    qfine_buffer[i] = bin_array_to_dec(&out_data[10], 7);
    Serial.println(String(qcoarse_buffer[i]) + "," + String(qfine_buffer[i]));

    Serial.print("out_data2:\t");
    print_data(out_data_2, 25);
    Serial.println();

    qcoarse_buffer[i] = bin_array_to_dec(&out_data_2[0], 10);
    qfine_buffer[i] = bin_array_to_dec(&out_data_2[10], 7);
    Serial.println(String(qcoarse_buffer[i]) + "," + String(qfine_buffer[i]));
    Serial.println();

    //Don't count the very first entry as a big_jump
    if (i > 1 && ((qcoarse_buffer[i] - qcoarse_buffer[i - 1] > 5) || (qcoarse_buffer[i - 1] - qcoarse_buffer[i] > 5))) {
      big_jump_count++;
    }
  }

  Serial.println("Readout Sequence Check Complete");
  Serial.print("Big Jump Count: ");
  Serial.println(big_jump_count);
  while (Serial.available()) Serial.read();
  while (!Serial.available())
    ;  //Wait for Serial

  /*
#ifdef INTERACTIVE
  //Clear any serial input that comes before the "?"
  while (Serial.available()) Serial.read();
  Serial.println("?");
  while (!Serial.available())
    ;  //Wait for Serial

  serialByte = Serial.read();

  if (serialByte == '1') {
    Serial.println("Single Acquisition!");
    single_acquisition = 1;
  } else {
    Serial.println("Streaming!");
    single_acquisition = 0;
  }

#endif
  
  //Clear Serial buffer
  while (Serial.available()) Serial.read();

  //~ ~ ~ ACQUISITION PHASE ~ ~ ~
  while (true) {

    for (int j = 0; j < 1000; j++) {
      //BEGIN CRITICAL TIMING SECTION

      //1) Wait for the rising edge of START/STOP.
      wait_on_rising_edge(TRIGGER);

#ifdef GET_CAPTURE_TIME
      capture_time_us = micros();
#endif

      //2) Delay 1 us to be 100% sure that the coarse/fine TDC values have settled.
      delayMicroseconds(1);

      //3) Set S_PASS low and grab timing data from the TDC internal register w/ an S_LOAD pulse.
      //digitalWrite(S_PASS, HIGH);
      //delayMicroseconds(20);
      digitalWrite(S_PASS, LOW);
      clock_pulse_portenta_C6(S_LOAD, 0);


      //4) Read out data
      serial_read_portenta_C6_G7_max(out_data, 25); // changed from 17 


      //5) Pulse Reset (~ 2 us)

      digitalWrite(RESET, HIGH);
      delayMicroseconds(1);
      digitalWrite(RESET, LOW);

      //Finally, capture output data in qcoarse/qfine buffers:
      qcoarse_buffer[j] = bin_array_to_dec(&out_data[0], 10);
      qfine_buffer[j] = bin_array_to_dec(&out_data[10], 7);

      //Write S_PASS HIGH to prepare for more data.
      digitalWrite(S_PASS, HIGH);


#ifdef GET_CAPTURE_TIME
      Serial.println("ct:" + String(capture_time_us - last_capture_time_us) + " rt:" + String(micros() - capture_time_us));  //Capture time
      //Serial.print();
      //Serial.print(" rt:"); //Readout time
      //Serial.println((micros() - capture_time_us));
      last_capture_time_us = capture_time_us;
#endif
    }

    //Every 1000 measurements we stop and print everything to console.
    Serial.println("~~~~~~~~~");
    for (int j = 0; j < 1000; j++) {
      Serial.println(String(qcoarse_buffer[j]) + "," + String(qfine_buffer[j]));
    }


    if (single_acquisition || Serial.available()) { break; }
  }

  //Idle.
  delay(1);*/
}