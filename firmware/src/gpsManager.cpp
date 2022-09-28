/**
 * gpsManager.cpp
 *
 * Author: David Antaki
 * Date: 9/24/2022
 */

#include "gpsManager.h"
LOG_MODULE_REGISTER(gps, LOG_LEVEL_DBG);

K_THREAD_STACK_DEFINE(gps_manager_stack, gpsManager::kStackSize);

gpsManager::gpsManager(uartBase _uart) : gps(_uart) {}

gpsManager::~gpsManager() {}

// The entry point and/or "main" function of this thread
void gpsManager::loopHook() {
  int64_t lastPrintMs = k_uptime_get();
  while (1) {
    gps.readIn();
    k_sleep(K_MSEC(loopTimeMs));
    // Print GPS data:
    if(k_uptime_get() - lastPrintMs > 3000){
      lastPrintMs = k_uptime_get();
      // TODO: Change to use LOGGING instead
      printk("Sats HDOP  Latitude   Longitude   Fix  Date       Time     Date  Alt    Course Speed Card  Chars Sentences Checksum\r\n");
      printk("           (deg)      (deg)       Age                      Age   (m)    --- from GPS ----   RX    RX        Fail\r\n");
      printk("----------------------------------------------------------------------------------------------------------------------------------------\r\n");
      // printk("#Sats: ");
      printInt(gps.getSatellites().value(), gps.getSatellites().isValid(), 5);
      // printk("HDOP: ");
      printFloat(gps.getHdop().hdop(), gps.getHdop().isValid(), 6, 1);
      // printk("Lat(deg): ");
      printFloat(gps.getLocation().lat(), gps.getLocation().isValid(), 11, 6);
      // printk("Lng(deg): ");
      printFloat(gps.getLocation().lng(), gps.getLocation().isValid(), 12, 6);      
      // printk("FixAge: ");
      printInt(gps.getLocation().age(), gps.getLocation().isValid(), 5);
      // printk("DateTime: ");
      printDateTime(gps.getDate(), gps.getTime());
      // printk("Alt(m): ");
      printFloat(gps.getAltitude().meters(), gps.getAltitude().isValid(), 7, 2);
      // printk("Course(deg): ");
      printFloat(gps.getCourse().deg(), gps.getCourse().isValid(), 7, 2);
      // printk("Speed: ");
      printFloat(gps.getSpeed().kmph(), gps.getSpeed().isValid(), 6, 2);
      // printk("Cardinal: ");
      printStr(gps.getCourse().isValid() ? TinyGPSPlus::cardinal(gps.getCourse().deg()) : "*** ", 6);
      printInt(gps.charsProcessed(), true, 6);
      printInt(gps.sentencesWithFix(), true, 10);
      printInt(gps.failedChecksum(), true, 9);
      printk("\r\n\r\n");
    }
  }
}

void gpsManager::entryPoint(void *thisThread, void *, void *) {
  gpsManager *pThis = (gpsManager *)thisThread;
  pThis->loopHook();
}

k_tid_t gpsManager::kThreadId;
struct k_thread gpsManager::gps_manager_thread_data;

// Creates this thread
void gpsManager::create() {
  // We pass a pointer to "this" class instance as a parameter because
  // entryPoint() must be static. Then we can call the passed class instance's
  // loopHook() from within entryPoint().
  kThreadId = k_thread_create(&gps_manager_thread_data, gps_manager_stack,
                              K_THREAD_STACK_SIZEOF(gps_manager_stack),
                              gpsManager::entryPoint, (gpsManager *)this, NULL,
                              NULL, kThreadPriority, 0, K_FOREVER);
}

// Starts this thread
void gpsManager::start() { k_thread_start(kThreadId); }
