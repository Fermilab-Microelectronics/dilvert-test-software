# dilvert-test-software
Test Software and Setup Information for DILVERT ASIC

## Concept of Operation 
The DILVERT Cold Board must be supplied with power, START/STOP signals, and digital control/communications.

Digital control and communications is handled by an Arduino. The Arduino must also get a TRIGGER signal when STOP arrives at DILVERT. At each TRIGGER, the Arduino will read out the TDC result from DILVERT, and stream it over serial to the lab PC. DILVERT_HAL_Interface.py will capture this streamed data and write it to a CSV file.


## Hardware: Digital Signal Connections

DILVERT SIGNAL	| LEVEL SHIFTER BOARD	  | SPARKFUN PRO MICRO	| PORTENTA | HD CONN | BREAKOUT
--- | --- | --- | --- | --- | ---
S_DIN	|P0.0	|9	|PC7/D4 (1 from bot left) | J2-63 | PWM2
S_CLK	|P0.1	|10	|PC6/D5 (0 from bot left) | J2-61 |  PWM1
S_PASS	|P0.2	|5	|1 (PK1/D1) (4 from bot left) | J2-60 | PWM5
S_LOAD	|P0.3	|4	|2 (PJ11/D2) (3 from bot left) | J2-67 | PWM4
RESET	|P0.5	|6	|0 (PH15/D0) (5 from bot left) | J2-62 | PWM6
S_DOUT	|P2.5	|8	|PG7/D3 (2 from bot left) | J2-65 | PWM3
TRIGGER	| |15	|21 (PA4/D21) (6 from bot left) | J2-78 | A6 (?)
			

## Hardware: Power Connections

Voltage	|Function	|Reasonable Starting Value (based on sim) | Supply
--- | --- | --- | ---
VDDIO | I/O Supply Voltage | +1.8 V | LS Board
VDD | Core Supply Voltage | +0.8 V | LS Board
VTUNE_N_FAST	|Higher voltage makes NFETs in the fast chain go faster.	|+1.9 V | J2-7
VTUNE_P_FAST	|Lower voltage makes PFETs in the fast chain go faster.	|+0.1 V | J2-5
VTUNE_N_SLOW	|Higher voltage makes NFETs in the slow chain go faster.	|+0.55 V | J2-11
VTUNE_P_SLOW	|Lower voltage makes PFETs in the slow chain go faster.	|+1.45 V | J2-9
VTUNE_N_RO	|Higher voltage makes the NFETs in the ring oscillator go faster.	|+1 V | J2-13
VTUNE_P_RO	|Lower voltage makes the PFETs in the ring oscillator go faster.	|+1 V | J2-3
NMOS_BG_BIAS	|Biases NFETs in the digital domain.	|+1 V | J2-15
PMOS_BG_BIAS	|Biases PFETs in the digital domain.	|-1 V | J2-1



