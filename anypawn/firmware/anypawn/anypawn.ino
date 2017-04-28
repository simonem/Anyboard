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
#include <RFduinoBLE.h>
#include <string.h>

#include "BLE_Handler.h"
#include "TokenSoloEvent_Handler.h"
#include "TokenConstraintEvent_Handler.h"
//#include "TokenFeedback_Handler.h"


// BOARD CONSTANTS
#define ACC_INT1_PIN        4 // Pin where the acceleromter interrupt1 is connected
#define VIBRATING_M_PIN     3 // Pin where the vibrating motor is connected

#define LED_R_PIN           0
#define LED_G_PIN           1
#define LED_B_PIN           2

//Variables for timing
uint_fast16_t volatile number_of_ms = 10;     // ms

// Handlers
BLE_Handler BLE;
TokenSoloEvent_Handler TokenSoloEvent(&BLE);
TokenConstraintEvent_Handler TokenConstraintEvent(&BLE);
TokenFeedback_Handler TokenFeedback;

// Sensors and actuators
Accelerometer *TokenAccelerometer = NULL;
InertialCentral_LSM9DS0 *InertialCentral = NULL;
ColorSensor *TokenColorSensor = NULL;
Haptic *HapticMotor = NULL;
RGB_LED *LED = NULL;
Matrix8x8 *Matrix = NULL;


void setup(void)
{
  override_uart_limit = true;
  //Serial.begin(9600); //SERIAL INTERFACE FOR DEBUGGING PURPOSES Comment if you want to use the LED
  interrupts(); // Enable interrupts

  //Initialization of the Sensors and actuators
  TokenAccelerometer = new Accelerometer(ACC_INT1_PIN);
  TokenColorSensor = new ColorSensor();
  InertialCentral = new InertialCentral_LSM9DS0();
  HapticMotor = new Haptic(VIBRATING_M_PIN);
  LED = new RGB_LED(LED_R_PIN, LED_G_PIN, LED_B_PIN, 2); // Type 1 is common anode, Type 2 is common cathode
  Matrix = new Matrix8x8();
  
  // Initialization of the TokenSoloEvent_Handler
  TokenSoloEvent.setAccelerometer(TokenAccelerometer);
  TokenSoloEvent.setInertialCentral(InertialCentral);
  
  // Initialization of the TokenConstraintEvent_Handler
  TokenConstraintEvent.setColorSensor(TokenColorSensor);

  //Initialization of the TokenFeedback_Handler
  TokenFeedback.setRGB_LED(LED);
  TokenFeedback.setHapticMotor(HapticMotor);
  TokenFeedback.setMatrix8x8(Matrix);
  
  // Configure the RFduino BLE properties
  char DeviceName[8] = {0};
  BLE.AdvertiseName.toCharArray(DeviceName, 8);
  RFduinoBLE.deviceName = DeviceName;
  RFduinoBLE.txPowerLevel = -20;
  RFduinoBLE.begin();
  
  Serial.println("Setup OK!");
  
  timer_config();
}

void loop(void)
{
    BLE.ProcessEvents();
    TokenFeedback.UpdateFeedback();
    
    if(TokenFeedback.isVibrating() == false)
      TokenSoloEvent.pollEvent();

    if (TokenAccelerometer->isActive() == false)
      TokenConstraintEvent.pollEvent();
}


















////////////////////////////  Private system method used to set the time basis throught a Timer /////////////////////////////////////////

#define TRIGGER_INTERVAL 1000      // ms

// This function updates the internal timers of each Handler
void TIMER1_Interrupt(void)
{
    if (NRF_TIMER1->EVENTS_COMPARE[0] != 0)
    {        
        TokenSoloEvent.HandleTime(number_of_ms);
        TokenConstraintEvent.HandleTime(number_of_ms);
        TokenFeedback.HandleTime(number_of_ms);
        
        NRF_TIMER1->EVENTS_COMPARE[0] = 0;
    }
}

// Timer configuration
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
    attachInterrupt(TIMER1_IRQn, TIMER1_Interrupt);                                                                            // also used in variant.cpp in the RFduino2.2 folder to configure the RTC1
    NRF_TIMER1->TASKS_START = 1;                                                                                               // Start TIMER
}


