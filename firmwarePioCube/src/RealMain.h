#ifndef _SRC_REALMAIN_H
#define _SRC_REALMAIN_H

#include "RealMain.h"
#include "pins.h"
#include "rosHandler.h"
#include "utils/log.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <Wire.h>
#include "AntakiImu.h"
#include <STM32encoder.h>
#include "config.h"
#include "utils.h"
#include "Encoder.h"
#include "OdometryManager.h"

extern void _Error_Handler(const char *msg, int val);

// Like the Controller in MVC
class RealMain {
public:
  RealMain()
      : mySerial4(UART4),
        i2c1(PB9, PB8),
        imu(55, 0x28, i2c1),
        encLeft(L_ENCODER_PIN1, L_ENCODER_PIN2),
        encRight(R_ENCODER_PIN1, R_ENCODER_PIN2),
        rosHandler(Serial2),
        odomManager(encLeft, encRight)
        {}
  ~RealMain() {}
  friend class rosHandler;
private:
  // ------------------------------ DEVICES ------------------------------
  HardwareSerial mySerial4;
  TwoWire i2c1;
  AntakiImu imu;
  Encoder encLeft;
  Encoder encRight;
  // ------------------------------ END DEVICES ------------------------------

  // ------------------------------ FAKE THREADS ------------------------------
  RosHandler rosHandler;
  OdometryManager odomManager;
  // ------------------------------ END FAKE THREADS ------------------------------
  
public:
  bool initialize() {
    bool success = true;
    delay(2000);

#if NUCLEO_F767ZI_CUSTOM
    serial2.setRx(PD_6);
    serial2.setTx(PD_5);
#endif
#if NUCLEO_F446RE_CUSTOM
    Serial2.end();
    Serial2.setRx(PA_3);
    Serial2.setTx(PA_2);
    Serial2.begin(RosHandler::rosSerialBaud);
    while (!Serial2) {
      yield();
    }
  
    // mySerial4.end();
    // mySerial4.setRx(PA_1);
    // mySerial4.setTx(PA_0);
    // mySerial4.begin(RosHandler::rosSerialBaud);
    // while(!mySerial4){
    //   yield();
    // }
#endif
    LOGEVENT("Setup...");

    #if ENABLE_ROSHANDLER
    success = success && rosHandler.init();
    #endif
    
    #if ENABLE_IMU
    success = success && imu.init();
    #endif
    
    pinMode(LVL_SHIFT_EN_PIN, OUTPUT);
    digitalWrite(LVL_SHIFT_EN_PIN, HIGH);

    // initPwm();
    // setPwm(5000, 50);
    // initAdc();
    pinMode(L_WHEEL_FORW_PIN, OUTPUT);
    pinMode(L_WHEEL_BACK_PIN, OUTPUT);
    pinMode(R_WHEEL_FORW_PIN, OUTPUT);
    pinMode(R_WHEEL_BACK_PIN, OUTPUT);
    

    // l_encoder_tim->setMode(1, TIMER_OUTPUT_COMPARE_PWM1, PA_5);
    // l_encoder_tim->setMode(2, TIMER_OUTPUT_COMPARE_PWM1, PB_3);
    // TIM_TypeDef *Instance = (TIM_TypeDef *)pinmap_peripheral(digitalPinToPinName(PA_5), PinMap_PWM);
    // uint32_t channel = STM_PIN_CHANNEL(pinmap_function(digitalPinToPinName(PA_5), PinMap_PWM));

    // LL_GPIO_SetAFPin_8_15(port, ll_pin, afnum);
    // LL_GPIO_SetAFPin_0_7(port, ll_pin, afnum);

    // pinModeAF(PA_5, GPIO_AF1_TIM2);
    // pinModeAF(PB_3, GPIO_AF1_TIM2);

    // rosHandlerTimer->attachInterrupt([this]() { rosHandler.loop(); });
    // rosHandlerTimer->setOverflow(1000, MICROSEC_FORMAT);  // 1ms

    return success;
  }

  // Main Big Loop
  void loop() {
    while (1) {
      // Blink LED every 1 second
      static uint32_t counter = 0;
      if ((millis() - counter) > 1000) {
        counter = millis();
        LOGEVENT("Looping...");
        Serial2.println("Looping...");
        // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      }

      #if ENABLE_ROSHANDLER
      rosHandler.loop();
      #endif

      #if ENABLE_IMU
      imu.loop();
      #endif

      // Print encoder pos if it changed
      static int32_t lastLeftPos = 0;
      static int32_t lastRightPos = 0;
      int32_t leftPos = encLeft.read();
      int32_t rightPos = encRight.read();
      if (leftPos != lastLeftPos || rightPos != lastRightPos) {
        lastLeftPos = leftPos;
        lastRightPos = rightPos;
        LOGEVENT("Left: %d, Right: %d", leftPos, rightPos);
      }
      // mySerial4.printf("looping\n");

      // analogWrite(L_WHEEL_FORW_PIN, 255);
      // LOGEVENT("ADC vRef Read (mV): %d", readVref());
      // LOGEVENT("ADC Read (mV): %d", readVoltage(readVref(), PA3));
      // HAL_Delay(500);
      // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      // HAL_Delay(10);


    }
  }

  bool deinitialize();

private:
  bool initialized = false;  
};

// The one and only main thread allocated statically.
extern RealMain realMain;
// The UART used for logging.
#define Console Serial2

#endif // _SRC_REALMAIN_H