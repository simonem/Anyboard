/********************************************************
  # NAME: RFduino_token.ino
  # AUTHOR: Matthias Monnier (matthias.monnier@gmail.com), Simone Mora (simonem@ntnu.no)
  # DATE: 16/12/2015
  # LICENSE: Apache V2.0(cf. file license.txt in the main repository)
  #
  # Firmware of the pawn token in the AnyBoard project.
  # V01 - Removed token-token interaction, moved libraries in sketch folder
  #
********************************************************/



#include <Wire.h>
#include <WInterrupts.h>
#include <SimbleeBLE.h>
#include <string.h>

//#include "protocol.h"
#include "BLE_Handler.h"
#include "TokenFeedback.h"
#include "TokenSoloEvent_Handler.h"
#include "TokenConstraintEvent_Handler.h"


// BOARD CONSTANTS
#define ACC_INT1_PIN        4 // Pin where the acceleromter interrupt1 is connected
#define VIBRATING_M_PIN     3 // Pin where the vibrating motor is connected

//Variables for timing
uint_fast16_t volatile number_of_ms = 10;     // ms
bool UpdateInertialCentral = false;

// VARIABLES FOR BLUETOOTH
BLE_Handler BLE;

// Variables for Token Solo Event
TokenSoloEvent_Handler TokenSoloEvent(&BLE);
Accelerometer *TokenAccelerometer = NULL;
InertialCentral_LSM9DS0 *InertialCentral = NULL;

// Variables for Token Constraint Event
TokenConstraintEvent_Handler TokenConstraintEvent(&BLE);
ColorSensor *TokenColorSensor = NULL;

// Initiation of the objects
TokenFeedback tokenFeedback = TokenFeedback(VIBRATING_M_PIN); // Connected on pin 2


void setup(void)
{
  override_uart_limit = true;
  Serial.begin(9600); //SERIAL INTERFACE FOR DEBUGGING PURPOSES
  interrupts(); // Enable interrupts

  Wire.begin();
  Wire.beginTransmission(0x08);
  Wire.write(0x010);
  Wire.endTransmission();
  
  Wire.requestFrom(0x08, 0x1);
  
  while(Wire.available())
    Serial.println(Wire.read(), HEX);
  
  while(1);
  
  //Initialization of the Sensors
  TokenAccelerometer = new Accelerometer(ACC_INT1_PIN);
  InertialCentral = new InertialCentral_LSM9DS0();
  TokenColorSensor = new ColorSensor();
  
  // Initialization of the TokenSoloEvent_Handler
  TokenSoloEvent.setAccelerometer(TokenAccelerometer);
  TokenSoloEvent.setInertialCentral(InertialCentral);
  
  // Initialization of the TokenConstraintEvent_Handler
  TokenConstraintEvent.setColorSensor(TokenColorSensor);


  
  // Configure the RFduino BLE properties
  SimbleeBLE.deviceName = "AnyPawn";
  SimbleeBLE.txPowerLevel = -20;

  // Start the BLE stack
  SimbleeBLE.begin();
  Serial.println("Setup OK!");
  // Config of the LED matrix
  tokenFeedback.matrixConfig();
  tokenFeedback.displayX();
  delay(1000);
  tokenFeedback.matrix.clear();
  tokenFeedback.matrix.writeDisplay();
  
  timer_config();
}

void loop(void)
{
    
    TokenSoloEvent.pollEvent();

    if (!TokenAccelerometer->isActive())
      TokenConstraintEvent.pollEvent();
    
    BLE.ProcessEvents();
    delay(10); // Important delay, do not delete it ! Why ?? I want to delete this one !!
}



#define TRIGGER_INTERVAL 1000      // ms

void timer_config(void)
{
    NRF_TIMER1->TASKS_STOP = 1;                                     // Stop timer
    NRF_TIMER1->MODE = TIMER_MODE_MODE_Timer;                        // sets the timer to TIME mode (doesn't make sense but OK!)
    NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_16Bit;               // with BLE only Timer 1 and Timer 2 and that too only in 16bit mode
    NRF_TIMER1->PRESCALER = 9;                                     // Prescaler 9 produces 31250 Hz timer frequency => t = 1/f =>  32 uS
                                                                     // The figure 31250 Hz is generated by the formula (16M) / (2^n)
                                                                     // where n is the prescaler value
                                                                     // hence (16M)/(2^9)=31250
    NRF_TIMER1->TASKS_CLEAR = 1;                                     // Clear timer
   
    //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //        Conversion to make cycle calculation easy
    //        Since the cycle is 32 uS hence to generate cycles in mS we need 1000 uS
    //        1000/32 = 31.25  Hence we need a multiplication factor of 31.25 to the required cycle time to achive it
    //        e.g to get a delay of 10 mS      we would do
    //        NRF_TIMER2->CC[0] = (10*31)+(10/4);
    //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
   
    NRF_TIMER1->CC[0] = (number_of_ms * 31) + (number_of_ms / 4);                                                                                  //CC[0] register holds interval count value i.e your desired cycle
    NRF_TIMER1->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;                                     // Enable COMAPRE0 Interrupt
    NRF_TIMER1->SHORTS = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);                             // Count then Complete mode enabled
    dynamic_attachInterrupt(TIMER1_IRQn, TIMER1_Interrupt);                                                                            // also used in variant.cpp in the RFduino2.2 folder to configure the RTC1
    NRF_TIMER1->TASKS_START = 1;                                                                                               // Start TIMER
}

void TIMER1_Interrupt(void)
{
    if (NRF_TIMER1->EVENTS_COMPARE[0] != 0)
    {
        UpdateInertialCentral = true;
        
        TokenSoloEvent.HandleTime(number_of_ms);
        TokenConstraintEvent.HandleTime(number_of_ms);
        
        NRF_TIMER1->EVENTS_COMPARE[0] = 0;
    }
}
