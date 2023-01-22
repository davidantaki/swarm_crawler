#include "RealMain.h"
//
#include <Arduino.h>  // This include order matters for Arduino's INPUT macro not getting overwritten.
//
#include "MantaCommon/src/utils/log.h"
//
#include "pins.h"
#include "common.h"
#include "adc.h"
#include "pwm.h"

// Devices
ePaper view(SPI); // TODO: Make passing SPI into the object work beacuse right now it's not working.
// I just call SPI.something() directly.
RealMain realMain(view);


/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void _Error_Handler(const char *msg, int val)
{
  /* User can add his own implementation to report the HAL error return state */
  LOGERROR("Error: %s (%i)", msg, val);
  while (1) {
  }
}

void setup() {
  // put your setup code here, to run once:
  #if LOGGING
  delay(2000);
  Console.begin(115200);
  while(!Console){
    yield();
  }
  #endif


  initPwm();
  setPwm(5000, 50);
  initAdc();

  LOGEVENT("\n\n\n");
  LOGEVENT("Starting RealMain");
  // realMain.go();

  LOGEVENT("RealMain exited.");
  LOGEVENT("Shutting off device.");
}

void loop() {
  LOGEVENT("ADC vRef Read (mV): %d", readVref());
  LOGEVENT("ADC Read (mV): %d", readVoltage(readVref(), PA3));
  HAL_Delay(10);
}
