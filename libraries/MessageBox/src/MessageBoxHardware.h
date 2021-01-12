#pragma once

#include <GxEPD.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#if defined(HARDWARE_LILYGO_T_BLOCK)

// What I know about the hardware so far:
// 
// * There are two buttons, one of them is directly connected to the power unit the other
//   is [ttgo->button]
// * PCF8563: RTC (Real Time Clock)
// * AXP202: Battery management hardware [ttgo->power]
// * MPU6050: Triple Axis Gyroscope & Accelerometer [ttgo->mpu]
// * eINK is an uses GxEPD

// A TTP223 touch thingy, on GPIO 32.
//
// There is a LED inside, no idea how to control it.

#define USER_BUTTON         36
// #define USER_LED            2  // There is a LED but it doesn't appear it is controllable.

// #define TOUCH_INT           38
// #define RTC_INT_PIN         37
// #define AXP202_INT          35
// #define BMA423_INT1         39
// #define BMA423_INT2         0

#define EINK_BUSY           34
#define EINK_RESET          27
#define EINK_DC             19
#define EINK_SS             5
#define EINK_SPI_MOSI       23
#define EINK_SPI_MISO       -1 //elink no use
#define EINK_SPI_CLK        18

// Depending on the version of the hardware, the controller might be slighly different.
//#include <GxGDE0213B1/GxGDE0213B1.h>      // 2.13" b/w
//#include <GxGDEH0213B72/GxGDEH0213B72.h>  // 2.13" b/w new panel
#include <GxGDEP015OC1/GxGDEP015OC1.h>  // 2.13" b/w newer panel

#elif defined(HARDWARE_LILYGO_T_H239)

#define USER_BUTTON         39
#define USER_LED            19 

#define EINK_BUSY           4
#define EINK_RESET          16
#define EINK_DC             17
#define EINK_SS             5
#define EINK_SPI_MOSI       23
#define EINK_SPI_MISO       -1 //elink no use
#define EINK_SPI_CLK        18

#include <GxGDEH0213B73/GxGDEH0213B73.h>

#elif
#error Please select hardware
#endif
