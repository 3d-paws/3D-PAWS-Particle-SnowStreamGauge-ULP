/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/Users/rjbubon/Documents/Particle/SSG-ULP/src/SSG-ULP.ino"
int seconds_to_next_obs();
bool NetworkConnect();
void NetworkDisconnect();
void firmwareUpdateHandler(system_event_t event, int param);
void setup();
void loop();
#line 1 "/Users/rjbubon/Documents/Particle/SSG-ULP/src/SSG-ULP.ino"
PRODUCT_VERSION(14);
#define COPYRIGHT "Copyright [2024] [University Corporation for Atmospheric Research]"
#define VERSION_INFO "SSG-ULP-20240927"

/*
 *======================================================================================================================
 * SSG_ULP - Snow or Stream Gauge Monitor Ultra Low Power
 *   Board Type : Particle BoronLTE
 *   Description: Snow or Stream Gauge Monitor with Ultra Low Power between between transmission times of 15m
 *   Author: Robert Bubon
 *   Date:  2020-12-11 RJB Initial Development
 *          2021-03-23 RJB 15 minute observations with ULTRA_LOW_POWER and modem off in between
 *          2021-03-31 RJB Bug fix on Check i2c
 *          2021-04-26 RJB Added HTU sensor
 *          2021-06-21 RJB Correct bug in network connect and handle BME returning NAN.
 *          2021-07-21 RJB Updated Libraries after HTU has infinate loop in code
 *          2021-10-07 RJB Production Release to Particle V4
 *          2021-11-15 RJB Version 6 
 *                         Battery Charger Fault Register, Variable name "cfr" Added
 *                         Adafruit BMP388 Support Added
 *                         PMIC was changed to match other code, low battery shutdown conditional changed
 *                         Bug: msgbuf 256 was too small increased to 1024
 *                         Added CloudDisconnectOptions().graceful(true).timeout(5s)
 *          2022-09-19 RJB Version 7
 *                         Will now wake up on the 0,15,30,45 periods during the hour.
 *                         Added support for 8 line oled displays
 *          2023-01-22 RJB Bug HTU had humidity and Temp flipped
 *          2023-08-22 RJB Fixed bug to get RTC updated from network time
 *                         Moved from 3.0.0 Firmware to 4.1
 *                         Update BMX sensor code
 *                         Added Min Max Quality Controls to sensor values
 *                         Added WITH_ACK to Particle Publish
 *          2023-08-29 RJB Adding mcp support
 *          2024-05-08 RJB Version 12
 *                         Added Argon support
 *                         Added 22hr reboot
 *                         Added 3rd party sim support
 *                         Moved SCE Pin from A4 to D8
 *                         Improved Station Monitor output
 *                         Dynamic display support added.
 *                         MCP2, SHT, HIH, LUX, SI1145 sensor support added 
 *                         Updated I2C_Check_Sensors()
 *          2024-06-23 RJB Added Copyright
 *          2024-07-14 RJB Split the code into include files
 *          2024-08-23 RJB Version 13 Fixed distance gauge code
 *          2024-08-24 RJB Version 14 Moved named from SG-ULP to SSG-ULP
 *          2024-09-11 RJB When setting SIM to INTERNAL we now set changed = true to
 *                         report success and reboot message.
 *          2024-09-28 RJB Corrected reporting SHT and HIH Humidity with the temperature value
 *          2024-11-05 RJB Discovered BMP390 first pressure reading is bad. Added read pressure to bmx_initialize()
 *                         Bug fixes for 2nd BMP sensor in bmx_initialize() using first sensor data structure
 *                         Now will only send humidity if bmx sensor supports it.
 *
 * NOTES:
 * When there is a successful transmission of an observation any need to send obersavations will be sent. 
 * On transmit a failure of these need to send observations, processing is stopped and the file is deleted.
 * 
 * Distance Gauge Calibration
 * Adding serial console jumper after boot will cause distance gauge to be read every 1 second and value printed.
 * Removing serial console jumper will resume normal operation
 * 
 * Requires Library
 *  SdFat                   by Bill Greiman
 *  RTCLibrary
 *  Adafruit_SSD1306_RK     I2C ADDRESS 0x3C
 *  Adafruit_BM(PE)280      I2C ADDRESS 0x77  (SD0 to GND = 0x76)
 *  adafruit-htu21df        I2C ADDRESS 0x40
 *  Adafruit_BMP3XX         I2C ADDRESS 0x77 and (SD0 to GND = 0x76)
 * 
 * System.batteryState()
 *  0 = BATTERY_STATE_UNKNOWN
 *  1 = BATTERY_STATE_NOT_CHARGING
 *  2 = BATTERY_STATE_CHARGING
 *  3 = BATTERY_STATE_CHARGED
 *  4 = BATTERY_STATE_DISCHARGING
 *  5 = BATTERY_STATE_FAULT
 *  6 = BATTERY_STATE_DISCONNECTED
 * 
 * Publish to Particle
 *  Event Name: SG
 *  Event Variables:
 *   at     timestamp
 *   sg     OD_Median
 *   bp1    bmx_pressure
 *   bt1    bmx_temp
 *   bh1    bmx_humid
 *   bp2    bmx_pressure
 *   bt2    bmx_temp
 *   bh2    bmx_humid
 *   hh1    htu_humid
 *   ht1    htu_temp
 *   bcs    Battery Charger Status
 *   bpc    Battery Percent Charge
 *   cfr    Charger Fault Register
 *   css    Cell Signal Strength
 *   hth    Health 16bits - See System Status Bits in below define statements
 * 
 * Particle Sleep
 * https://docs.particle.io/firmware/low-power/wake-publish-sleep-cellular/
 * 
 * AN002 Device Powerdown
 * https://support.particle.io/hc/en-us/articles/360044252554?input_string=how+to+handle+low+battery+and+recovery
 * 
 * NOTE: Compile Issues
 * If you have compile issues like multiple definations of functions then you need to clean the compile directory out
 *    ~/.particle/toolchains/deviceOS/2.0.1/build/target/user/...
 * 
 * ========================================================
 * Support for 3rd Party Sim 
 * ========================================================
 *   SEE https://support.particle.io/hc/en-us/articles/360039741113-Using-3rd-party-SIM-cards
 *   SEE https://docs.particle.io/cards/firmware/cellular/setcredentials/
 *   Logic
 *     Output how sim is configured (internal or external)
 *     If console is enabled and SD found and SIM.TXT exists at the top level of SD card
 *       Read 1st line from SIM.TXT. Parse line for one of the below patterns
 *        INTERNAL
 *        AUP epc.tmobile.com username passwd
 *        UP username password
 *        APN epc.tmobile.com
 *      Perform appropriate actions to set sim
 *      Rename file to SIMOLD.TXT, so we don't do this on next boot
 *      Output notification to user to reboot then flash board led forever
 *
 * ========================================================
 * Support for Argon WiFi Boards
 * ========================================================
 * At the top level of the SD card make a file called WIFI.TXT
 * Add one line to the file
 * This line has 3 items that are comma separated Example
 * 
 * AuthType,ssid,password
 * 
 * Where AuthType is one of these keywords (WEP WPA WPA2)
 *
 * ========================================================
 * Distance Sensor Type Setup.
 * ========================================================
 * Create SD Card File 5MDIST.TXT for %m sensor and 1.25 multiplier. No file for 10m Sensor and 2.5 multiplier
 * ======================================================================================================================
 */
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_HTU21DF.h>
#include <Adafruit_MCP9808.h>
#include <Adafruit_SI1145.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_VEML7700.h>
#include <RTClib.h>
#include <SdFat.h>

/*
 * ======================================================================================================================
 *  Timers
 * ======================================================================================================================
 */
#define DELAY_NO_RTC              1000*60    // Loop delay when we have no valided RTC
#define CLOUD_CONNECTION_TIMEOUT  90         // Wait for N seconds to connect to the Cell Network

/*
 * ======================================================================================================================
 * System Status Bits used for report health of systems - 0 = OK
 * 
 * OFF =   SSB &= ~SSB_PWRON
 * ON =    SSB |= SSB_PWROFF
 * 
 * ======================================================================================================================
 */
#define SSB_PWRON            0x1   // Set at power on, but cleared after first observation
#define SSB_SD               0x2   // Set if SD missing at boot or other SD related issues
#define SSB_RTC              0x4   // Set if RTC missing at boot
#define SSB_OLED             0x8   // Set if OLED missing at boot, but cleared after first observation
#define SSB_N2S             0x10   // Set if Need to Send observation file exists
#define SSB_FROM_N2S        0x20   // Set if observation is from the Need to Send file
#define SSB_AS5600          0x40   // Set if wind direction sensor AS5600 has issues - NOT USED with Distance Gauge                   
#define SSB_BMX_1           0x80   // Set if Barometric Pressure & Altitude Sensor missing
#define SSB_BMX_2          0x100   // Set if Barometric Pressure & Altitude Sensor missing
#define SSB_HTU21DF        0x200   // Set if Humidity & Temp Sensor missing
#define SSB_SI1145         0x400   // Set if UV index & IR & Visible Sensor missing
#define SSB_MCP_1          0x800   // Set if Precision I2C Temperature Sensor missing
#define SSB_MCP_2         0x1000   // Set if Precision I2C Temperature Sensor missing
#define SSB_SHT_1         0x4000   // Set if SHTX1 Sensor missing
#define SSB_SHT_2         0x8000   // Set if SHTX2 Sensor missing
#define SSB_HIH8         0x10000   // Set if HIH8000 Sensor missing
#define SSB_LUX          0x20000   // Set if VEML7700 Sensor missing
#define SSB_PM25AQI      0x40000   // Set if PM25AQI Sensor missing

unsigned int SystemStatusBits = SSB_PWRON; // Set bit 0 for initial value power on. Bit 0 is cleared after first obs
bool JustPoweredOn = true;         // Used to clear SystemStatusBits set during power on device discovery

/*
 * ======================================================================================================================
 *  Globals
 * ======================================================================================================================
 */
#define MAX_MSGBUF_SIZE 1024
char msgbuf[MAX_MSGBUF_SIZE]; // Used to hold messages
char *msgp;                   // Pointer to message text
char Buffer32Bytes[32];       // General storage

int  LED_PIN = D7;            // Built in LED
bool PostedResults;           // How we did in posting Observation and Need to Send Observations

uint64_t lastOBS = 0;         // time of next observation
int countdown = 120;          // Exit station monitor when reaches 0 - protects against burnt out pin or forgotten jumper

uint64_t NetworkReady = 0;
uint64_t LastTimeUpdate = 0;
uint64_t StartedConnecting = 0;
bool ParticleConnecting = false;

int  cf_reboot_countdown_timer = 79200; // There is overhead transmitting data so take off 2 hours from 86400s
                                        // Set to 0 to disable feature
int DailyRebootCountDownTimer;

bool firmwareUpdateInProgress = false;

#if PLATFORM_ID == PLATFORM_BORON
/*
 * ======================================================================================================================
 *  Power Management IC (bq24195)
 * ======================================================================================================================
 */
PMIC pmic;
#endif

/*
 * ======================================================================================================================
 *  SD Card
 * ======================================================================================================================
 */
#define SD_ChipSelect D5                // GPIO 10 is Pin 10 on Feather and D5 on Particle Boron Board
SdFat SD;                               // File system object.
File SD_fp;
char SD_obsdir[] = "/OBS";              // Store our obs in this directory. At Power on, it is created if does not exist
bool SD_exists = false;                     // Set to true if SD card found at boot
char SD_n2s_file[] = "N2SOBS.TXT";          // Need To Send Observation file
uint32_t SD_n2s_max_filesz = 200 * 8 * 24;  // Keep a little over 2 days. When it fills, it is deleted and we start over.
uint32_t SD_N2S_POSITION = 0;               // Position in the file past observations that have been sent.  

char SD_sim_file[] = "SIM.TXT";         // File used to set Ineternal or External sim configuration
char SD_simold_file[] = "SIMOLD.TXT";   // SIM.TXT renamed to this after sim configuration set

char SD_wifi_file[] = "WIFI.TXT";         // File used to set WiFi configuration

/*
 * ======================================================================================================================
 *  Local Code Includes - Do not change the order of the below 
 * ======================================================================================================================
 */
#include "QC.h"                   // Quality Control Min and Max Sensor Values on Surface of the Earth
#include "SF.h"                   // Support Functions
#include "OP.h"                   // OutPut support for OLED and Serial Console
#include "CF.h"                   // Configuration File Variables
#include "TM.h"                   // Time Management
#include "Sensors.h"              // I2C Based Sensors
#include "WRD.h"                  // Wind Rain Distance
#include "EP.h"                   // EEPROM
#include "SDC.h"                  // SD Card
#include "OBS.h"                  // Do Observation Processing
#include "SM.h"                   // Station Monitor
#include "PS.h"                   // Particle Support Functions

/* 
 *=======================================================================================================================
 * seconds_to_next_obs() - do observations on 0, 15, 30, or 45 minute window
 *=======================================================================================================================
 */
int seconds_to_next_obs() {
  now = rtc.now(); //get the current date-time
  return (900 - (now.unixtime() % 900)); // 900 = 60s * 15m,  The mod operation gives us seconds passed in this 15m window
}

/*
 * ======================================================================================================================
 * NetworkConnect() - Turn on and Connect to Particle if not already   
 * 
 * You must not turn off and on cellular more than every 10 minutes (6 times per hour). Your SIM can be blocked by
 * your mobile carrier for aggressive reconnection if you reconnect to cellular very frequently. If you are manually 
 * managing the cellular connection in case of connection failures, you should wait at least 5 minutes before 
 * stopping the connection attempt.         
 * ======================================================================================================================
 */
bool NetworkConnect() {
 if (!Particle.connected()) {
    if(StartedConnecting == 0) {  // Not already connecting
#if PLATFORM_ID == PLATFORM_BORON
      Cellular.on();
      Output ("Cell Connecting");
      Cellular.connect();
#else
      WiFi.on();
      Output ("WiFi Connecting");
      WiFi.connect();
#endif
      StartedConnecting = System.millis();
    }
#if PLATFORM_ID == PLATFORM_BORON
    else if(Cellular.ready() && !ParticleConnecting) {
#else
    else if(WiFi.ready() && !ParticleConnecting) {
#endif
      // Time is synchronized automatically when the device connects to the Cloud.
      // Use NetworkReady to know when to update RTC
      NetworkReady = System.millis();
      Output ("Particle Connecting");
      Particle.connect();
      ParticleConnecting = true;
    }
    else if((System.millis() - StartedConnecting) >= (CLOUD_CONNECTION_TIMEOUT * 1000) ) { 
      //Already connecting - check for time out 60s
      Output ("Connect Timeout");
      NetworkDisconnect();
      return false;   // return false when timmed out trying to connect
    }
  }
  return true; // return true for being connected or in the process of connecting
}

/*
 * ======================================================================================================================
 * NetworkDisconnect() - Disconnect from Particle and Turn off modem   
 * ======================================================================================================================
 */
void NetworkDisconnect() {
  if (Particle.connected()) {
    Output ("Disconnect:PC");  
  }
  else {
    Output ("Dissconect:PNC");
  }

  // While this function will disconnect from the Cloud, it will keep the connection to the network.
  Particle.disconnect();
  waitFor(Particle.disconnected, 1000);  // Returns true when disconnected from the Cloud.

  // Output ("Cell Disconnect");
  #if PLATFORM_ID == PLATFORM_BORON
  Cellular.disconnect();
  Cellular.off();
  #else
  WiFi.disconnect();
  WiFi.off();
  #endif
  delay(1000); // in case of race conditions

  StartedConnecting = 0;
  ParticleConnecting = false;
}


/*
 * ======================================================================================================================
 * firmwareUpdateHandler() - The firmware update handler sets a global variable when the firmware update starts, 
 *                           so we can defer sleep until it completes (or times out)
 * 
 * SEE https://docs.particle.io/reference/device-os/api/system-events/system-events-reference/#cloud_status-64-parameter-values
 * ======================================================================================================================
 */
void firmwareUpdateHandler(system_event_t event, int param) {
    switch(param) {
        case 0:  // firmware_update_begin:
            firmwareUpdateInProgress = true;
            break;
        case 1:  // firmware_update_complete:
        case -1: // firmware_update_failed:
            firmwareUpdateInProgress = false;
            break;
    }
}

// You must use SEMI_AUTOMATIC or MANUAL mode so the battery is properly reconnected on
// power-up. If you use AUTOMATIC, you may be unable to connect to the cloud, especially
// on a 2G/3G device without the battery.
SYSTEM_MODE(SEMI_AUTOMATIC);

// https://docs.particle.io/cards/firmware/system-thread/system-threading-behavior/
SYSTEM_THREAD(ENABLED);

/*
 * ======================================================================================================================
 * setup() - runs once, when the device is first turned on.
 * ======================================================================================================================
 */
void setup() {
    // The device has booted, reconnect the battery.
#if PLATFORM_ID == PLATFORM_BORON
	pmic.enableBATFET();
#endif

  // Set Default Time Format
  Time.setFormat(TIME_FORMAT_ISO8601_FULL);

  // Put initialization like pinMode and begin functions here.
  pinMode (LED_PIN, OUTPUT);
  Output_Initialize();
  delay(2000); // Prevents usb driver crash on startup, Arduino needed this so keeping for Particle

  // Set up Distance gauge pin for reading 
  pinMode(DISTANCEGAUGE, INPUT);

  Serial_write(COPYRIGHT);
  Output (VERSION_INFO); // Doing it one more time for the OLED
  delay(4000);

  // The System.on() function is used to subscribe to system-level events and configure 
  // how the device should behave when these events occur.
  // Firmware update handler sets a global variable when the firmware update starts.
  // We do not want to go into low power mode during this update
  // System.on(firmware_update, firmwareUpdateHandler);

   // Set Daily Reboot Timer
  DailyRebootCountDownTimer = cf_reboot_countdown_timer;  

  // Initialize SD card if we have one.
  SD_initialize();

  // Report if we have Need to Send Observations
  if (SD_exists && SD.exists(SD_n2s_file)) {
    SystemStatusBits |= SSB_N2S; // Turn on Bit
    Output("N2S:Exists");
  }
  else {
    SystemStatusBits &= ~SSB_N2S; // Turn Off Bit
    Output("N2S:NF");
  }

  // Display EEPROM Information 
  EEPROM_Dump();

  // Check if correct time has been maintained by RTC
  // Uninitialized clock would be 2000-01-00T00:00:00
  stc_timestamp();
  sprintf (msgbuf, "%s+", timestamp);
  Output(msgbuf);

  // Read RTC and set system clock if RTC clock valid
  rtc_initialize();

  if (Time.isValid()) {
    Output("STC: Valid");
  }
  else {
    Output("STC: Not Valid");
  }

  stc_timestamp();
  sprintf (msgbuf, "%s=", timestamp);
  Output(msgbuf);

  // Adafruit i2c Sensors
  bmx_initialize();
  htu21d_initialize();
  mcp9808_initialize();
  sht_initialize();
  hih8_initialize();
  si1145_initialize();
  lux_initialize();

  if (SD.exists(SD_5M_DIST_FILE)) {
    dg_adjustment = 1.25;
    Output ("DIST=5M");
  }
  else {
    dg_adjustment = 2.5;
    Output ("DIST=10M");
  }

#if PLATFORM_ID == PLATFORM_ARGON
  //==================================================
  // Check if we need to program for WiFi change
  //==================================================
  WiFiChangeCheck();
#else
  //==================================================
  // Check if we need to program for Sim change
  //==================================================
  SimChangeCheck();
#endif

  NetworkConnect();
}

/*
 * ======================================================================================================================
 * loop() runs over and over again, as quickly as it can execute.
 * ======================================================================================================================
 */
void loop() {
  bool PowerDown = false;

  // If Serial Console Pin LOW then read Distance Gauge, Print and Sleep
  // Used for calibrating Distance Gauge at Installation
  // Only stay in this mode for countdown seconds, this protects against burnt out pin or forgotten jumper
  if (countdown && digitalRead(SCE_PIN) == LOW) {
    StationMonitor();

    if (!Particle.connected()) {
      NetworkConnect(); // Need to connect to Particle to get board time set
    }
    // StartedConnecting = System.millis();  // Keep set timer so when we pull jumper we finish connecting and not time out
    countdown--;

    // Taking the Distance measurement is delay enough
  }
  else { // Normal Operation - Main Work
    if (Time.isValid()) {
      if (Particle.connected()) {
        Output ("Particle Connected");
        OBS_Do();

        // Shutoff System Status Bits related to initialization after we have logged first observation 
        JPO_ClearBits();
      
        // STC is automatically updated when the device connects to the Cloud
        rtc.adjust(DateTime(Time.year(), Time.month(), Time.day(), Time.hour(), Time.minute(), Time.second() ));
        Output("RTC: Updated");

        PowerDown = true;
      }
      else {
        bool timedOut = NetworkConnect(); // Check on how we are doing connecting to the Cell Network

        if(timedOut == false) { 
          // We failed to connect the Modem is off
          Output ("Connect Failed");
          OBS_Do(); // Particle will not be connected,  this will cause N2S created

          // Shutoff System Status Bits related to initialization after we have logged first observation 
          JPO_ClearBits();

          PowerDown = true;
        }
        else {
          delay (1000); // Waiting on network
        }
      }
    }
    else {
      if (!Particle.connected()) {
        NetworkConnect(); // Need to connect to Particle to get board time set
      }
      stc_timestamp();
      Output(timestamp);
      Output("ERR: No Clock");
      delay (DELAY_NO_RTC);
    }    

    // Before we go into ULP mode for 15m, check if we are not connected to a charging source
    // and our battery is at a low level. If so then power down the display and board. 
    // Wait for power to return.
    //
    // We do this at a high enough battery level to avoid the board from powering
    // itself down out of our control. The goal is to leave enough battery for the sensors to
    // chew on and still be able, when power returns, to charge the battery and transmit 
    // with out a current drop causing the board to reset or power down out of our control.
#if PLATFORM_ID == PLATFORM_BORON
    if (PowerDown) {
      if (firmwareUpdateInProgress) {
        Output ("FW Update In Progress");
        Output ("Delaying until Next OBS");
        delay (seconds_to_next_obs()*1000);
      }
      // We are on battery and have 10% or less percent left then turnoff and wait for power to return
      else if (!pmic.isPowerGood() && (System.batteryCharge() <= 10.0)) {

        Output("Low Power!");

        // Disconnect from the cloud and power down the modem.
        Particle.disconnect();
        Cellular.off();
        delay(5000);

        Output("Powering Down");

        OLED_sleepDisplay();
        delay(5000);

        // Disabling the BATFET disconnects the battery from the PMIC. Since there
        // is no longer external power, this will turn off the device.
        pmic.disableBATFET();

        // This line should not be reached. When power is applied again, the device
        // will cold boot starting with setup().

        // However, there is a potential for power to be re-applied while we were in
        // the process of shutting down so if we're still running, enable the BATFET
        // again and reconnect to the cloud. Wait a bit before doing this so the
        // device has time to actually power off.
        delay(2000);

        OLED_wakeDisplay();   // May need to toggle the Display reset pin.
        delay(2000);
        Output("Power Re-applied");

        pmic.enableBATFET();

        // Start Network
        StartedConnecting = 0; 
        ParticleConnecting = false;
        NetworkConnect();
      }
      else {
        Output ("Going to Sleep");
        // Disconnect from the cloud and power down the modem.
        Particle.disconnect();
        Cellular.off();

        OLED_sleepDisplay();
        delay(2000);        

        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(seconds_to_next_obs()*1000);
        SystemSleepResult result = System.sleep(config);

        // On wake, execution continues after the the System.sleep() command 
        // with all local and global variables intact.
        // System time is still valid.
       
        OLED_wakeDisplay();
        delay(2000);
        Output("Wake Up");

        // Start Network
        StartedConnecting = 0;
        ParticleConnecting = false;
        NetworkConnect();

        // See what sensors we have - any go off line while sleeping?
        I2C_Check_Sensors();
      } // sleep
    } // powerdown
#endif
#if PLATFORM_ID == PLATFORM_ARGON
    if (PowerDown) {
      if (firmwareUpdateInProgress) {
        Output ("FW Update In Progress");
        Output ("Delaying until Next OBS");
        delay (seconds_to_next_obs()*1000);
      }
      else {
        Output ("Going to Sleep");
        // Disconnect from the cloud and power down the modem.
        Particle.disconnect();
        WiFi.off();

        OLED_sleepDisplay();
        delay(2000);        

        SystemSleepConfiguration config;
        config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(seconds_to_next_obs()*1000);
        SystemSleepResult result = System.sleep(config);

        // On wake, execution continues after the the System.sleep() command 
        // with all local and global variables intact.
        // System time is still valid.
       
        OLED_wakeDisplay();
        delay(2000);
        Output("Wake Up");

        // Start Network
        StartedConnecting = 0;
        ParticleConnecting = false;
        NetworkConnect();

        // See what sensors we have - any go off line while sleeping?
        I2C_Check_Sensors(); // Make sure Sensors are online
      } // sleep
    } // powerdown
#endif 
  } // normal operation
} // loop
