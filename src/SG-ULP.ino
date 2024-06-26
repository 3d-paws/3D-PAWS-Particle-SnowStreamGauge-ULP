PRODUCT_VERSION(12);
#define COPYRIGHT "Copyright [2024] [University Corporation for Atmospheric Research]"
#define VERSION_INFO "SG-ULP-20240508"

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
 * ======================================================================================================================
 */
#define W4SC false   // Set true to Wait for Serial Console to be connected

#include <OneWire.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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
 *  Quality Control - Min and Max Sensor Values on Surface of the Earth
 * ======================================================================================================================
 */

// Temperature
#define QC_MIN_T       -40.0     // deg C - Min Recorded -89.2°C (-128.6°F), -40.0 is sensor limit
#define QC_MAX_T       60.0      // deg C - Max Recorded 56.7°C (134°F)
#define QC_ERR_T       -999.9    // deg C Error

// Preasure - We are not adjusting for altitude, record min/max values are adjusted.
#define QC_MIN_P       300.0     // hPa - Min Recorded 652.5 mmHg 869.93hPa
#define QC_MAX_P       1100.0    // hPa - Max Recorded 1083.8mb aka 1083.8hPa
#define QC_ERR_P       -999.9    // hPa Error

// Relative Humidity
#define QC_MIN_RH      0.0       // %
#define QC_MAX_RH      100.0     // %
#define QC_ERR_RH      -999.9    // Relative Humidity Error

// Sensor SI1145
#define QC_MIN_IR      0.0       // unitless
#define QC_MAX_IR      16000.0   // unitless - based on the maximum readings from 43 stations located in tropics
#define QC_ERR_IR      -999.9    // Infrared Light Error
#define QC_MIN_VI      0.0       // unitless
#define QC_MAX_VI      2000.0    // unitless
#define QC_ERR_VI      -999.9    // Visual Light Error
#define QC_MIN_UV      0.0       // unitless
#define QC_MAX_UV      1000.0    // unitless
#define QC_ERR_UV      -999.9    // UV Light Error

// Sensor VEML7700 - Ambient Light Sensor
#define QC_MIN_LX      0         // lx
#define QC_MAX_LX      120000    // lx - based on sensor spec
#define QC_ERR_LX      -999      // Ambient Light Error

// Wind Speed  - world-record surface wind speed measured on Mt. Washington on April 12, 1934 (231 mph or 103 mps)
#define QC_MIN_WS      0.0       // m/s
#define QC_MAX_WS      103.0     // m/s
#define QC_ERR_WS      -999.9    // Relative Humidity Error

// Wind Direction
#define QC_MIN_WD      0         // deg
#define QC_MAX_WD      360       // deg
#define QC_ERR_WD      -999      // deg Error

/*
 * ======================================================================================================================
 *  EEPROM NonVolitileMemory - stores rain totals in persistant memory
 * ======================================================================================================================
 */
typedef struct {
    time32_t ts;         // timestamp of last modification
    unsigned long n2sfp; // sd need 2 send file position
    unsigned long checksum;
} EEPROM_NVM;
EEPROM_NVM eeprom;
int eeprom_address = 0;
bool eeprom_valid = false;

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
 *  Serial Console Enable
 * ======================================================================================================================
 */
int SCE_PIN = D8;
bool SerialConsoleEnabled = false;  // Variable for serial monitor control

/*
 * =======================================================================================================================
 *  Distance Gauge
 * =======================================================================================================================
 */
#define DISTANCEGAUGE   A3
#define OD_BUCKETS      60
char SD_5M_DIST_FILE[] = "5MDIST.TXT";  // If file exists use adjustment of 1.25. No file, then 10m Sensor is 2.5
float od_adjustment = 2.5;       // Default sensor is 10m 
unsigned int od_bucket = 0;            // Observation Distance Buckets
unsigned int od_buckets[OD_BUCKETS];

/*
 * ======================================================================================================================
 *  RTC Setup
 * ======================================================================================================================
 */
RTC_PCF8523 rtc;              // RTC_PCF8523 rtc;
DateTime now;
char timestamp[32];
bool RTC_valid = false;
bool RTC_exists = false;

/*
 * ======================================================================================================================
 *  OLED Display
 * ======================================================================================================================
 */
#define SCREEN_WIDTH        128 // OLED display width, in pixels
#define OLED32_I2C_ADDRESS  0x3C // 128x32 - https://www.adafruit.com/product/4440
#define OLED64_I2C_ADDRESS  0x3D // 128x64 - https://www.adafruit.com/product/326
#define OLED_RESET          -1 // -1 = Not in use
#define OLED32              (oled_type == OLED32_I2C_ADDRESS)
#define OLED64              (oled_type == OLED64_I2C_ADDRESS)

bool DisplayEnabled = true;
int  oled_type = 0;
char oled_lines[8][23];
Adafruit_SSD1306 display32(SCREEN_WIDTH, 32, &Wire, OLED_RESET);
Adafruit_SSD1306 display64(SCREEN_WIDTH, 64, &Wire, OLED_RESET);

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
 *  BMX280 humidity - I2C - Temperature, pressure sensor & altitude - Support 2 of any combination
 * 
 *  https://www.asknumbers.com/PressureConversion.aspx
 *  Pressure is returned in the SI units of Pascals. 100 Pascals = 1 hPa = 1 millibar. 
 *  Often times barometric pressure is reported in millibar or inches-mercury. 
 *  For future reference 1 pascal = 0.000295333727 inches of mercury, or 1 inch Hg = 3386.39 Pascal. 
 *
 *  Looks like you divide by 100 and you get millibars which matches NWS page
 * 
 *  Surface Observations and Station Elevation 
 *  https://forecast.weather.gov/product.php?issuedby=BOU&product=OSO&site=bou 
 * 
 * Forced mode
 * In forced mode, a single measurement is performed according to selected measurement and filter options. 
 * When the measurement is finished, the sensor returns to sleep mode and the measurement results can be obtained 
 * from the data registers. For a next measurement, forced mode needs to be selected again. 
 * ======================================================================================================================
 */
// #define BMX_STATION_ELEVATION 1017.272  // default 1013.25
#define BMX_ADDRESS_1         0x77  // BMP Default Address - Connecting SDO to GND will change BMP to 0x76
#define BMX_ADDRESS_2         0x76  // BME Default Address - Connecting SDO to GND will change BME to 0x77
#define BMP280_CHIP_ID        0x58
#define BME280_BMP390_CHIP_ID 0x60
#define BMP388_CHIP_ID        0x50
#define BMX_TYPE_UNKNOWN      0
#define BMX_TYPE_BMP280       1
#define BMX_TYPE_BME280       2
#define BMX_TYPE_BMP388       3
#define BMX_TYPE_BMP390       4
Adafruit_BMP280 bmp1;
Adafruit_BMP280 bmp2;
Adafruit_BME280 bme1;
Adafruit_BME280 bme2;
Adafruit_BMP3XX bm31;
Adafruit_BMP3XX bm32;
byte BMX_1_chip_id = 0x00;
byte BMX_2_chip_id = 0x00;
bool BMX_1_exists = false;
bool BMX_2_exists = false;
byte BMX_1_type=BMX_TYPE_UNKNOWN;
byte BMX_2_type=BMX_TYPE_UNKNOWN;

/*
 * ======================================================================================================================
 *  HTU21D-F - I2C - Humidity & Temp Sensor
 * ======================================================================================================================
 */
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
bool HTU21DF_exists = false;

/*
 * ======================================================================================================================
 *  MCP9808 - I2C - Temperature sensor
 * 
 * I2C Address is:  0011,A2,A1,A0
 *                  0011000 = 0x18  where A2,1,0 = 0 MCP9808_I2CADDR_DEFAULT  
 *                  0011001 = 0x19  where A0 = 1
 * ======================================================================================================================
 */
#define MCP_ADDRESS_1     0x18
#define MCP_ADDRESS_2     0x19        // A0 set high, VDD
Adafruit_MCP9808 mcp1;
Adafruit_MCP9808 mcp2;
bool MCP_1_exists = false;
bool MCP_2_exists = false;

/*
 * ======================================================================================================================
 *  SHTX - I2C - Temperature & Humidity sensor (SHT31)  - Note the SHT40, SHT45 use same i2c address
 * ======================================================================================================================
 */
#define SHT_ADDRESS_1     0x44
#define SHT_ADDRESS_2     0x45        // ADR pin set high, VDD
Adafruit_SHT31 sht1;
Adafruit_SHT31 sht2;
bool SHT_1_exists = false;
bool SHT_2_exists = false;

/*
 * ======================================================================================================================
 *  HIH8 - I2C - Temperature & Humidity sensor (HIH8000)  - 
 * ======================================================================================================================
 */
#define HIH8000_ADDRESS   0x27
bool HIH8_exists = false;

/*
 * ======================================================================================================================
 *  Si1145 - I2C - UV/IR/Visible Light Sensor
 *  The SI1145 has a fixed I2C address (0x60), you can only connect one sensor per microcontroller!
 * ======================================================================================================================
 */
Adafruit_SI1145 uv = Adafruit_SI1145();
bool SI1145_exists = false;
// When we do a read of all three and we get zeros. If these llast readings are not zero, we will reinitialize the
// chip. When does a reset on it and then read again.
float si_last_vis = 0.0;
float si_last_ir = 0.0;
float si_last_uv = 0.0;

/*
 * ======================================================================================================================
 *  VEML7700 - I2C - Lux Sensor
 * ======================================================================================================================
 */
#define VEML7700_ADDRESS   0x10
Adafruit_VEML7700 veml = Adafruit_VEML7700();
bool VEML7700_exists = false;


/*
 * ======================================================================================================================
 * myswap()
 * ======================================================================================================================
 */
void myswap(unsigned int *p, unsigned int *q) {
  int t;
   
  t=*p; 
  *p=*q; 
  *q=t;
}

/*
 * ======================================================================================================================
 * mysort()
 * ======================================================================================================================
 */
void mysort(unsigned int a[], unsigned int n) { 
  unsigned int i, j;

  for(i = 0;i < n-1;i++) {
    for(j = 0;j < n-i-1;j++) {
      if(a[j] > a[j+1])
        myswap(&a[j],&a[j+1]);
    }
  }
}

/* 
 *=======================================================================================================================
 * I2C_Device_Exist - does i2c device exist
 * 
 *  The i2c_scanner uses the return value of the Write.endTransmisstion to see 
 *  if a device did acknowledge to the address.
 *=======================================================================================================================
 */
bool I2C_Device_Exist(byte address) {
  byte error;

  Wire.begin();                     // Connect to I2C as Master (no addess is passed to signal being a slave)

  Wire.beginTransmission(address);  // Begin a transmission to the I2C slave device with the given address. 
                                    // Subsequently, queue bytes for transmission with the write() function 
                                    // and transmit them by calling endTransmission(). 

  error = Wire.endTransmission();   // Ends a transmission to a slave device that was begun by beginTransmission() 
                                    // and transmits the bytes that were queued by write()
                                    // By default, endTransmission() sends a stop message after transmission, 
                                    // releasing the I2C bus.

  // endTransmission() returns a byte, which indicates the status of the transmission
  //  0:success
  //  1:data too long to fit in transmit buffer
  //  2:received NACK on transmit of address
  //  3:received NACK on transmit of data
  //  4:other error 

  // Partice Library Return values
  // SEE https://docs.particle.io/cards/firmware/wire-i2c/endtransmission/
  // 0: success
  // 1: busy timeout upon entering endTransmission()
  // 2: START bit generation timeout
  // 3: end of address transmission timeout
  // 4: data byte transfer timeout
  // 5: data byte transfer succeeded, busy timeout immediately after
  // 6: timeout waiting for peripheral to clear stop bit

  if (error == 0) {
    return (true);
  }
  else {
    // sprintf (msgbuf, "I2CERR: %d", error);
    // Output (msgbuf);
    return (false);
  }
}

/* 
 *=======================================================================================================================
 * get_Bosch_ChipID ()  -  Return what Bosch chip is at specified address
 *   Chip ID BMP280 = 0x58 temp, preasure           - I2C ADDRESS 0x77  (SD0 to GND = 0x76)  
 *   Chip ID BME280 = 0x60 temp, preasure, humidity - I2C ADDRESS 0x77  (SD0 to GND = 0x76)  Register 0xE0 = Reset
 *   Chip ID BMP388 = 0x50 temp, preasure           - I2C ADDRESS 0x77  (SD0 to GND = 0x76)
 *   Chip ID BMP390 = 0x60 temp, preasure           - I2C ADDRESS 0x77  (SD0 to GND = 0x76)
 *=======================================================================================================================
 */
byte get_Bosch_ChipID (byte address) {
  byte chip_id = 0;
  byte error;

  Output ("get_Bosch_ChipID()");
  // The i2c_scanner uses the return value of
  // the Write.endTransmisstion to see if
  // a device did acknowledge to the address.

  // Important! Need to check the 0x00 register first. Doing a 0x0D (not chip id loaction) on a bmp388 
  // will return a value that could match one of the IDs 

  // Check Register 0x00
  sprintf (msgbuf, "  I2C:%02X Reg:%02X", address, 0x00);
  Output (msgbuf);
  Wire.begin();
  Wire.beginTransmission(address);
  Wire.write(0x00);  // BM3 CHIPID REGISTER
  error = Wire.endTransmission();
    //  0:success
    //  1:data too long to fit in transmit buffer
    //  2:received NACK on transmit of address
    //  3:received NACK on transmit of data
    //  4:other error 
  if (error) {
    sprintf (msgbuf, "  ERR_ET:%d", error);
    Output (msgbuf);
  }
  else if (Wire.requestFrom(address, 1)) {  // Returns the number of bytes returned from the slave device 
    chip_id = Wire.read();
    if (chip_id == BMP280_CHIP_ID) { // 0x58
      sprintf (msgbuf, "  CHIPID:%02X BMP280", chip_id);
      Output (msgbuf);
      return (chip_id); // Found a Sensor!
    }
    else if (chip_id == BMP388_CHIP_ID) {  // 0x50
      sprintf (msgbuf, "  CHIPID:%02X BMP388", chip_id);
      Output (msgbuf);
      return (chip_id); // Found a Sensor!   
    }
    else if (chip_id == BME280_BMP390_CHIP_ID) {  // 0x60
      sprintf (msgbuf, "  CHIPID:%02X BME/390", chip_id);
      Output (msgbuf);
      return (chip_id); // Found a Sensor!   
    }
    else {
      sprintf (msgbuf, "  CHIPID:%02X InValid", chip_id);
      Output (msgbuf);      
    }
  }
  else {
    sprintf (msgbuf, "  ERR_RF:0");
    Output (msgbuf);
  }

  // Check Register 0xD0
  chip_id = 0;
  sprintf (msgbuf, "  I2C:%02X Reg:%02X", address, 0xD0);
  Output (msgbuf);
  Wire.begin();
  Wire.beginTransmission(address);
  Wire.write(0xD0);  // BM2 CHIPID REGISTER
  error = Wire.endTransmission();
    //  0:success
    //  1:data too long to fit in transmit buffer
    //  2:received NACK on transmit of address
    //  3:received NACK on transmit of data
    //  4:other error 
  if (error) {
    sprintf (msgbuf, "  ERR_ET:%d", error);
    Output (msgbuf);
  }
  else if (Wire.requestFrom(address, 1)) {  // Returns the number of bytes returned from the slave device 
    chip_id = Wire.read(); 
    if (chip_id == BMP280_CHIP_ID) { // 0x58
      sprintf (msgbuf, "  CHIPID:%02X BMP280", chip_id);
      Output (msgbuf);
      return (chip_id); // Found a Sensor!
    }
    else if (chip_id == BMP388_CHIP_ID) {  // 0x50
      sprintf (msgbuf, "  CHIPID:%02X BMP388", chip_id);
      Output (msgbuf);
      return (chip_id); // Found a Sensor!   
    }
    else if (chip_id == BME280_BMP390_CHIP_ID) {  // 0x60
      sprintf (msgbuf, "  CHIPID:%02X BME/390", chip_id);
      Output (msgbuf);
      return (chip_id); // Found a Sensor!   
    }
    else {
      sprintf (msgbuf, "  CHIPID:%02X InValid", chip_id);
      Output (msgbuf);   
    }
  }
  else {
    sprintf (msgbuf, "  ERR_RF:0");
    Output (msgbuf);
  }
  return(0);
}

/* 
 *=======================================================================================================================
 * bmx_initialize() - Bosch sensor initialize
 *=======================================================================================================================
 */
void bmx_initialize() {
  Output("BMX:INIT");
  
  // 1st Bosch Sensor - Need to see which (BMP, BME, BM3) is plugged in
  BMX_1_chip_id = get_Bosch_ChipID(BMX_ADDRESS_1);
  switch (BMX_1_chip_id) {
    case BMP280_CHIP_ID :
      if (!bmp1.begin(BMX_ADDRESS_1)) { 
        msgp = (char *) "BMP1 ERR";
        BMX_1_exists = false;
        SystemStatusBits |= SSB_BMX_1;  // Turn On Bit          
      }
      else {
        BMX_1_exists = true;
        BMX_1_type = BMX_TYPE_BMP280;
        msgp = (char *) "BMP1 OK";
      }
    break;

    case BME280_BMP390_CHIP_ID :
      if (!bme1.begin(BMX_ADDRESS_1)) { 
        if (!bm31.begin_I2C(BMX_ADDRESS_1)) {  // Perhaps it is a BMP390
          msgp = (char *) "BMX1 ERR";
          BMX_1_exists = false;
          SystemStatusBits |= SSB_BMX_1;  // Turn On Bit          
        }
        else {
          BMX_1_exists = true;
          BMX_1_type = BMX_TYPE_BMP390;
          msgp = (char *) "BMP390_1 OK";         
        }      
      }
      else {
        BMX_1_exists = true;
        BMX_1_type = BMX_TYPE_BME280;
        msgp = (char *) "BME280_1 OK";
      }
    break;

    case BMP388_CHIP_ID :
      if (!bm31.begin_I2C(BMX_ADDRESS_1)) { 
        msgp = (char *) "BM31 ERR";
        BMX_1_exists = false;
        SystemStatusBits |= SSB_BMX_1;  // Turn On Bit          
      }
      else {
        BMX_1_exists = true;
        BMX_1_type = BMX_TYPE_BMP388;
        msgp = (char *) "BM31 OK";
      }
    break;

    default:
      msgp = (char *) "BMX_1 NF";
    break;
  }
  Output (msgp);

  // 2nd Bosch Sensor - Need to see which (BMP, BME, BM3) is plugged in
  BMX_2_chip_id = get_Bosch_ChipID(BMX_ADDRESS_2);
  switch (BMX_2_chip_id) {
    case BMP280_CHIP_ID :
      if (!bmp1.begin(BMX_ADDRESS_2)) { 
        msgp = (char *) "BMP2 ERR";
        BMX_2_exists = false;
        SystemStatusBits |= SSB_BMX_2;  // Turn On Bit          
      }
      else {
        BMX_2_exists = true;
        BMX_2_type = BMX_TYPE_BMP280;
        msgp = (char *) "BMP2 OK";
      }
    break;

    case BME280_BMP390_CHIP_ID :
      if (!bme2.begin(BMX_ADDRESS_2)) { 
        if (!bm31.begin_I2C(BMX_ADDRESS_2)) {  // Perhaps it is a BMP390
          msgp = (char *) "BMX2 ERR";
          BMX_2_exists = false;
          SystemStatusBits |= SSB_BMX_2;  // Turn On Bit          
        }
        else {
          BMX_2_exists = true;
          BMX_2_type = BMX_TYPE_BMP390;
          msgp = (char *) "BMP390_2 OK";          
        }
      }
      else {
        BMX_2_exists = true;
        BMX_2_type = BMX_TYPE_BME280;
        msgp = (char *) "BME280_2 OK";
      }
    break;

    case BMP388_CHIP_ID :
      if (!bm31.begin_I2C(BMX_ADDRESS_2)) { 
        msgp = (char *) "BM31 ERR";
        BMX_2_exists = false;
        SystemStatusBits |= SSB_BMX_2;  // Turn On Bit          
      }
      else {
        BMX_2_exists = true;
        BMX_2_type = BMX_TYPE_BMP388;
        msgp = (char *) "BM31 OK";
      }
    break;

    default:
      msgp = (char *) "BMX_2 NF";
    break;
  }
  Output (msgp);
}

/* 
 *=======================================================================================================================
 * htu21d_initialize() - HTU21D sensor initialize
 *=======================================================================================================================
 */
void htu21d_initialize() {
  Output("HTU21D:INIT");
  
  // HTU21DF Humidity & Temp Sensor (I2C ADDRESS = 0x40)
  if (!htu.begin()) {
    msgp = (char *) "HTU NF";
    HTU21DF_exists = false;
    SystemStatusBits |= SSB_HTU21DF;  // Turn On Bit
  }
  else {
    HTU21DF_exists = true;
    msgp = (char *) "HTU OK";
  }
  Output (msgp);
}

/*
 * ======================================================================================================================
 * Blink() - Count, delay between, delay at end
 * ======================================================================================================================
 */
void Blink(int count, int between)
{
  int c;

  for (c=0; c<count; c++) {
    digitalWrite(LED_PIN, HIGH);
    delay(between);
    digitalWrite(LED_PIN, LOW);
    delay(between);
  }
}

/* 
 *=======================================================================================================================
 * stc_timestamp() - Read from System Time Clock and set timestamp string
 *=======================================================================================================================
 */
void stc_timestamp() {

  // ISO_8601 Time Format
  sprintf (timestamp, "%d-%02d-%02dT%02d:%02d:%02d", 
    Time.year(), Time.month(), Time.day(),
    Time.hour(), Time.minute(), Time.second());
}

/* 
 *=======================================================================================================================
 * rtc_timestamp() - Read from RTC and set timestamp string
 *=======================================================================================================================
 */
void rtc_timestamp() {
  now = rtc.now(); //get the current date-time

  // ISO_8601 Time Format
  sprintf (timestamp, "%d-%02d-%02dT%02d:%02d:%02d", 
    now.year(), now.month(), now.day(),
    now.hour(), now.minute(), now.second());
}

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
 * OLED_sleepDisplay()
 * ======================================================================================================================
 */
void OLED_sleepDisplay() {
  if (DisplayEnabled) {
    if (OLED32) {
      display32.ssd1306_command(SSD1306_DISPLAYOFF);
    }
    else {
      display64.ssd1306_command(SSD1306_DISPLAYOFF);
    }
  }
}

/*
 * ======================================================================================================================
 * OLED_wakeDisplay()
 * ======================================================================================================================
 */
void OLED_wakeDisplay() {
  if (DisplayEnabled) {
    if (OLED32) {
      display32.ssd1306_command(SSD1306_DISPLAYON);
    }
    else {
      display64.ssd1306_command(SSD1306_DISPLAYON);
    }
  }
}

/*
 * ======================================================================================================================
 * OLED_spin() 
 * ======================================================================================================================
 */
void OLED_spin() {
  static int spin=0;
    
  if (DisplayEnabled) {
    if (OLED32) {
      display32.setTextColor(WHITE, BLACK); // Draw 'inverse' text
      display32.setCursor(120,24);
      display32.print(" ");
      display32.setCursor(120,24); 
    }
    else {
      display64.setTextColor(WHITE, BLACK); // Draw 'inverse' text
      display64.setCursor(120,24);
      display64.print(" ");
      display64.setCursor(120,24);
      display64.setCursor(120,56);
      display64.print(" ");
      display64.setCursor(120,56);       
    } 
    switch (spin++) {
      case 0 : msgp = (char *) "|"; break;
      case 1 : msgp = (char *) "/"; break;
      case 2 : msgp = (char *) "-"; break;
      case 3 : msgp = (char *) "\\"; break;
    }
    if (OLED32) {
      display32.print(msgp);
      display32.display();
    }
    else {
      display64.print(msgp);
      display64.display();
    }
    spin %= 4;
  }
}


/*
 * ======================================================================================================================
 * OLED_update() -- Output oled in memory map to display
 * ======================================================================================================================
 */
void OLED_update() {  
  if (DisplayEnabled) {
    if (OLED32) {
      display32.clearDisplay();
      display32.setCursor(0,0);             // Start at top-left corner
      display32.print(oled_lines [0]);
      display32.setCursor(0,8);
      display32.print(oled_lines [1]);
      display32.setCursor(0,16);
      display32.print(oled_lines [2]);
      display32.setCursor(0,24);  
      display32.print(oled_lines [3]);
      display32.display();
    }
    else {
      display64.clearDisplay();
      display64.setCursor(0,0);             // Start at top-left corner
      display64.print(oled_lines [0]);
      display64.setCursor(0,8);
      display64.print(oled_lines [1]);
      display64.setCursor(0,16);
      display64.print(oled_lines [2]);
      display64.setCursor(0,24);  
      display64.print(oled_lines [3]);
      display64.setCursor(0,32);  
      display64.print(oled_lines [4]);
      display64.setCursor(0,40);  
      display64.print(oled_lines [5]);
      display64.setCursor(0,48);  
      display64.print(oled_lines [6]);
      display64.setCursor(0,56);  
      display64.print(oled_lines [7]);
      display64.display();
     
    }
  }
}

/*
 * ======================================================================================================================
 * OLED_write() 
 * ======================================================================================================================
 */
void OLED_write(const char *str) {
  int c, len, bottom_line = 3;
  
  if (DisplayEnabled) {
    // move lines up
    for (c=0; c<=21; c++) {
      oled_lines [0][c] = oled_lines [1][c];
      oled_lines [1][c] = oled_lines [2][c];
      oled_lines [2][c] = oled_lines [3][c];
      if (OLED64) {
        oled_lines [3][c] = oled_lines [4][c];
        oled_lines [4][c] = oled_lines [5][c];
        oled_lines [5][c] = oled_lines [6][c];  
        oled_lines [6][c] = oled_lines [7][c];  
        bottom_line = 7;          
      }
    }

    // check length on new output line string
    len = strlen (str);
    if (len>21) {
      len = 21;
    }
    for (c=0; c<=len; c++) {
      oled_lines [bottom_line][c] = *(str+c);
    }

    // Adding Padding
    for (;c<=21; c++) {
      oled_lines [bottom_line][c] = ' ';
    }
    oled_lines [bottom_line][22] = (char) NULL;
    
    OLED_update();
  }
}

/*
 * ======================================================================================================================
 * OLED_write_noscroll() -- keep lines 1-3 and output on line 4
 * ======================================================================================================================
 */
void OLED_write_noscroll(const char *str) {
  int c, len, bottom_line = 3;

  if (OLED64) {
    bottom_line = 7;
  }

  if (DisplayEnabled) {
    len = strlen (str);
    if (len>21) {
      len = 21;
    }
    
    for (c=0; c<=len; c++) {
      oled_lines [bottom_line][c] = *(str+c);
    }

    // Adding Padding
    for (;c<=21; c++) {
      oled_lines [bottom_line][c] = ' ';
    }
    oled_lines [bottom_line][22] = (char) NULL;
    
    OLED_update();
  }
}

/*
 * ======================================================================================================================
 * Serial_write() 
 * ======================================================================================================================
 */
void Serial_write(const char *str) {
  if (SerialConsoleEnabled) {
    Serial.println(str);
  }
}

/*
 * ======================================================================================================================
 * Serial_writeln() 
 * ======================================================================================================================
 */
void Serial_writeln(const char *str) {
  if (SerialConsoleEnabled) {
    Serial.println(str);
    Serial.flush();
  }
}

/*
 * ======================================================================================================================
 * Output()
 * ======================================================================================================================
 */
void Output(const char *str) {
  OLED_write(str);
  Serial_write(str);
}

/*
 * ======================================================================================================================
 * OutputNS() - Output with no scroll on oled
 * ======================================================================================================================
 */
void OutputNS(const char *str) {
  OLED_write_noscroll(str);
  Serial_write(str);
}

/*
 * ======================================================================================================================
 * OLED_initialize() -- Initialize oled if enabled
 * ======================================================================================================================
 */
void OLED_initialize() {
  if (DisplayEnabled) {
    if (I2C_Device_Exist (OLED32_I2C_ADDRESS)) {
      oled_type = OLED32_I2C_ADDRESS;
      display32.begin(SSD1306_SWITCHCAPVCC, OLED32_I2C_ADDRESS);
      display32.clearDisplay();
      display32.setTextSize(1); // Draw 2X-scale text
      display32.setTextColor(WHITE);
      display32.setCursor(0, 0);
      for (int r=0; r<4; r++) {
        oled_lines[r][0]=0;
      }
      OLED_write("OLED32:OK");
    }
    else if (I2C_Device_Exist (OLED64_I2C_ADDRESS)) {
      oled_type = OLED64_I2C_ADDRESS;
      display64.begin(SSD1306_SWITCHCAPVCC, OLED64_I2C_ADDRESS);
      display64.clearDisplay();
      display64.setTextSize(1); // Draw 2X-scale text
      display64.setTextColor(WHITE);
      display64.setCursor(0, 0);
      for (int r=0; r<8; r++) {
        oled_lines[r][0]=0;
      }
      OLED_write("OLED64:OK");
    }
    else {
      DisplayEnabled = false;
      SystemStatusBits |= SSB_OLED; // Turn on Bit
    }
  }
}

/*
 * ======================================================================================================================
 * Serial_Initialize() -
 * ======================================================================================================================
 */
void Serial_Initialize() {
  // serial console enable pin
  pinMode(SCE_PIN, INPUT_PULLUP);   // Internal pullup resistor biases the pin to supply voltage.
                                    // If jumper set to ground, we enable serial console (low = enable)
  if (digitalRead(SCE_PIN) == LOW) {
    SerialConsoleEnabled = true;
  }

  // There are libraries that print to Serial Console so we need to initialize no mater what the jumper is set to.
  Serial.begin(9600);
  delay(1000); // prevents usb driver crash on startup, do not omit this

  if (SerialConsoleEnabled) {
    // Wait for serial port to be available
    if (!Serial.isConnected()) {
      OLED_write("Wait4 Serial Console");
    }
    int countdown=60; // Wait N seconds for serial connection, then move on.
    while (!Serial.isConnected() && countdown) {
      Blink(1, 1000);
      countdown--;
    }

    Serial_writeln(""); // Send carriage return and linefeed
    
    if (DisplayEnabled) {
      Serial_writeln ("OLED:Enabled");
    }
    else {
      Serial_writeln ("OLED:Disabled");
    }
    Output ("SC:Enabled");
  }
}

/*
 * ======================================================================================================================
 * Output_Initialize() -
 * ======================================================================================================================
 */
void Output_Initialize() {
  OLED_initialize();
  Output("SER:Init");
  Serial_Initialize();
  Output("SER:OK");
}

/* 
 *=======================================================================================================================
 * rtc_initialize()
 *=======================================================================================================================
 */
void rtc_initialize() {

  if (!rtc.begin()) { // Always returns true
     Output("ERR:RTC NOT FOUND");
     SystemStatusBits |= SSB_RTC; // Turn on Bit
     return;
  }
  
  if (!I2C_Device_Exist(PCF8523_ADDRESS)) {
    Output("ERR:RTC-I2C NOTFOUND");
    SystemStatusBits |= SSB_RTC; // Turn on Bit
    delay (5000);
    return;
  }

  RTC_exists = true; // We have a clock hardware connected

  // Particle.io library is 1.2.1 (Old). The below are supported in 1.12.4

  //we don't need the 32K Pin, so disable it
  // rtc.disable32K();

  // set alarm 1, 2 flag to false (so alarm 1, 2 didn't happen so far)
  // if not done, this easily leads to problems, as both register aren't reset on reboot/recompile
  // rtc.clearAlarm(1);
  // rtc.clearAlarm(2);

  // stop oscillating signals at SQW Pin
  // otherwise setAlarm1 will fail
  // rtc.writeSqwPinMode(DS3231_OFF);

  // turn off alarm 1, 2 (in case it isn't off already)
  // again, this isn't done at reboot, so a previously set alarm could easily go overlooked
  // rtc.disableAlarm(1);
  // rtc.disableAlarm(2);
  
  // now = rtc.now(); //get the current date-time - done in timestamp().

  rtc_timestamp();
  sprintf (msgbuf, "%s*", timestamp);
  Output (msgbuf);

  // Do a validation check on the year. 
  // Asumption is: If RTC not set, it will not have the current year.
  
  if (now.year() >= 2021) {
    now = rtc.now();
    Time.setTime(now.unixtime()); // If RTC valid, we set STC.
    RTC_valid = true;
  }
  else {
    Output ("NEED GSM TIME->RTC");
    delay (5000); // Give the user some time to see this problem.
  }
}

/* 
 *=======================================================================================================================
 * EEPROM_ChecksumCompute()
 *=======================================================================================================================
 */
unsigned long EEPROM_ChecksumCompute() {
  unsigned long checksum=0;
  checksum += (unsigned long) eeprom.ts;
  checksum += (unsigned long) eeprom.n2sfp;
  return (checksum);
}

/* 
 *=======================================================================================================================
 * EEPROM_ChecksumUpdate()
 *=======================================================================================================================
 */
void EEPROM_ChecksumUpdate() {
  eeprom.checksum = EEPROM_ChecksumCompute();
}

/* 
 *=======================================================================================================================
 * EEPROM_ChecksumValid()
 *=======================================================================================================================
 */
bool EEPROM_ChecksumValid() {
  unsigned long checksum = EEPROM_ChecksumCompute();

  if (checksum == eeprom.checksum) {
    return (true);
  }
  else {
    return (false);
  }
}

/* 
 *=======================================================================================================================
 * EEPROM_Reset() - Reset to default values
 *                  Requires system clock to be valid
 *=======================================================================================================================
 */
void EEPROM_Reset(time32_t current_time) {
  if (Time.isValid()) {
    eeprom.ts = current_time;
    eeprom.n2sfp = 0;
    EEPROM_ChecksumUpdate();
    EEPROM.put(eeprom_address, eeprom);
    Output("EEPROM RESET");
  }
  else {
    Output("EEPROM RESET ERROR");
  }
}

/* 
 *=======================================================================================================================
 * EEPROM_Initialize() - Check status of EEPROM information and determine status
 *                       Requires system clock to be valid
 *=======================================================================================================================
 */
void EEPROM_Initialize() {
  if (Time.isValid()) {
    time32_t current_time = Time.now();

    EEPROM.get(eeprom_address, eeprom);

    if (!EEPROM_ChecksumValid()) {
      EEPROM_Reset(current_time);
    }
    else {
      Output("EEPROM VALID");
    }
    eeprom_valid = true; 
  }
  else {
    Output("EEPROM INIT ERROR");
  }
}

/* 
 *=======================================================================================================================
 * EEPROM_Update() - Check status of EEPROM information and determine status
 *=======================================================================================================================
 */
void EEPROM_Update() {
  if (eeprom_valid) {
    eeprom.ts = Time.now();
    EEPROM_ChecksumUpdate();
    EEPROM.put(eeprom_address, eeprom);
    Output("EEPROM UPDATED");
  }
}

/* 
 *=======================================================================================================================
 * EEPROM_Dump() - 
 *=======================================================================================================================
 */
void EEPROM_Dump() {
  size_t EEPROM_length = EEPROM.length();

  EEPROM.get(eeprom_address, eeprom);

  unsigned long checksum = EEPROM_ChecksumCompute();

  Output("EEPROM DUMP");

  sprintf (msgbuf, " LEN:%d", EEPROM_length);
  Output(msgbuf);

  sprintf (Buffer32Bytes, " TS:%lu", eeprom.ts);
  Output (Buffer32Bytes);

  sprintf (Buffer32Bytes, " N2SFP:%lu", eeprom.n2sfp);
  Output (Buffer32Bytes);

  sprintf (Buffer32Bytes, " CS:%lu", eeprom.checksum);
  Output (Buffer32Bytes);

  sprintf (Buffer32Bytes, " CSC:%lu", checksum);
  Output (Buffer32Bytes);
}

/* 
 *=======================================================================================================================
 * SD_initialize()
 *=======================================================================================================================
 */
void SD_initialize() {

  if (!SD.begin(SD_ChipSelect)) {
    Output ("SD:NF");
    SystemStatusBits |= SSB_SD;
    delay (5000);
  }
  else {
    if (!SD.exists(SD_obsdir)) {
      if (SD.mkdir(SD_obsdir)) {
        Output ("SD:MKDIR OBS OK");
        Output ("SD:Online");
        SD_exists = true;
      }
      else {
        Output ("SD:MKDIR OBS ERR");
        Output ("SD:Offline");
        SystemStatusBits |= SSB_SD;  // Turn On Bit     
      } 
    }
    else {
      Output ("SD:Online");
      Output ("SD:OBS DIR Exists");
      SD_exists = true;
    }
  }
}

/* 
 *=======================================================================================================================
 * SD_LogObservation()
 *=======================================================================================================================
 */
void SD_LogObservation(char *observations) {
  char SD_logfile[24];
  File fp;

  if (!SD_exists) {
    return;
  }

  if (!Time.isValid()) {
    return;
  }
  
  sprintf (SD_logfile, "%s/%4d%02d%02d.log", SD_obsdir, Time.year(), Time.month(), Time.day());
  
  fp = SD.open(SD_logfile, FILE_WRITE); 
  if (fp) {
    fp.println(observations);
    fp.close();
    SystemStatusBits &= ~SSB_SD;  // Turn Off Bit
    Output ("OBS Logged to SD");
  }
  else {
    SystemStatusBits |= SSB_SD;  // Turn On Bit - Note this will be reported on next observation
    Output ("OBS Open Log Err");
    // At thins point we could set SD_exists to false and/or set a status bit to report it
    // sd_initialize();  // Reports SD NOT Found. Library bug with SD
  }
}

/* 
 *=======================================================================================================================
 * SD_N2S_Delete()
 *=======================================================================================================================
 */
bool SD_N2S_Delete() {
  bool result;

  if (SD_exists && SD.exists(SD_n2s_file)) {
    if (SD.remove (SD_n2s_file)) {
      SystemStatusBits &= ~SSB_N2S; // Turn Off Bit
      Output ("N2S->DEL:OK");
      result = true;
    }
    else {
      Output ("N2S->DEL:ERR");
      SystemStatusBits |= SSB_SD; // Turn On Bit
      result = false;
    }
  }
  else {
    Output ("N2S->DEL:NF");
    result = true;
  }
  eeprom.n2sfp = 0;
  EEPROM_Update();
  return (result);
}

/* 
 *=======================================================================================================================
 * SD_NeedToSend_Add()
 *=======================================================================================================================
 */
void SD_NeedToSend_Add(char *observation) {
  File fp;

  if (!SD_exists) {
    return;
  }
  
  fp = SD.open(SD_n2s_file, FILE_WRITE); // Open the file for reading and writing, starting at the end of the file.
                                         // It will be created if it doesn't already exist.
  if (fp) {  
    if (fp.size() > SD_n2s_max_filesz) {
      fp.close();
      Output ("N2S:Full");
      if (SD_N2S_Delete()) {
        // Only call ourself again if we truely deleted the file. Otherwise infinate loop.
        SD_NeedToSend_Add(observation); // Now go and log the data
      }
    }
    else {
      fp.println(observation); //Print data, followed by a carriage return and newline, to the File
      fp.close();
      SystemStatusBits &= ~SSB_SD;  // Turn Off Bit
      Output ("N2S:OBS Added");
    }
  }
  else {
    SystemStatusBits |= SSB_SD;  // Turn On Bit - Note this will be reported on next observation
    Output ("N2S:Open Error");
    // At thins point we could set SD_exists to false and/or set a status bit to report it
    // sd_initialize();  // Reports SD NOT Found. Library bug with SD
  }
}

/* 
 *=======================================================================================================================
 * SD_N2S_Publish()
 *=======================================================================================================================
 */
void SD_N2S_Publish() {
  File fp;
  char ch;
  int i;
  int sent=0;

  if (SD_exists && SD.exists(SD_n2s_file)) {
    Output ("N2S:Publish");

    fp = SD.open(SD_n2s_file, FILE_READ); // Open the file for reading, starting at the beginning of the file.

    if (fp) {
      // Delete Empty File or too small of file to be valid
      if (fp.size()<=20) {
        fp.close();
        Output ("N2S:Empty");
        SD_N2S_Delete();
      }
      else {
        if (eeprom.n2sfp) {
          if (fp.size()<=eeprom.n2sfp) {
            // Something wrong. Can not have a file position that is larger than the file
            eeprom.n2sfp = 0; 
          }
          else {
            fp.seek(eeprom.n2sfp);  // Seek to where we left off last time. 
          }
        }

        // Loop through each line / obs and transmit
        i = 0;
        while (fp.available() && (i < MAX_MSGBUF_SIZE )) {
          ch = fp.read();

          if (ch == 0x0A) {  // newline

            if (Particle_Publish()) {
              sprintf (Buffer32Bytes, "N2S[%d]->PUB:OK", sent++);
              Output (Buffer32Bytes);
              Serial_write (msgbuf);

              // setup for next line in file
              i = 0;

              // file position is at the start of the next observation or at eof
              eeprom.n2sfp = fp.position();           
            }
            else { // Delay then retry 
              sprintf (Buffer32Bytes, "N2S[%d]->PUB:RETRY", sent);
              Output (Buffer32Bytes);
              Serial_write (msgbuf);

              delay (5000); // Throttle a little while

              if (Particle_Publish()) {
                sprintf (Buffer32Bytes, "N2S[%d]->PUB:OK", sent++);
                Output (Buffer32Bytes);
                // setup for next line in file
                i = 0;

                // file position is at the start of the next observation or at eof
                eeprom.n2sfp = fp.position();
              }
              else {
                sprintf (Buffer32Bytes, "N2S[%d]->PUB:ERR", sent);
                Output (Buffer32Bytes);
                // On transmit failure, stop processing file.
                break;
              }
            } // RETRY
            
            // At this point file pointer's position is at the first character of the next line or at eof
          } // Newline
          else if (ch == 0x0D) { // CR
            msgbuf[i] = 0; // null terminate then wait for newline to be read to process OBS
          }
          else {
            msgbuf[i++] = ch;
          }

          // Check for buffer OverRun
          if (i >= MAX_MSGBUF_SIZE) {
            sprintf (Buffer32Bytes, "N2S[%d]->BOR:ERR", sent);
            Output (Buffer32Bytes);
            fp.close();
            SD_N2S_Delete(); // Bad data in the file so delete the file           
            return;
          }
        } // end while 

        if (fp.available() <= 20) {
          // If at EOF or some invalid amount left then delete the file
          fp.close();
          SD_N2S_Delete();
        }
        else {
          // At this point we sent 0 or more observations but there was a problem.
          // eeprom.n2sfp was maintained in the above read loop. So we will close the
          // file and next time this function is called we will seek to eeprom.n2sfp
          // and start processing from there forward. 
          fp.close();
          EEPROM_Update(); // Update file postion in the eeprom.
        }
      }
    }
    else {
        Output ("N2S->OPEN:ERR");
    }
  }
}

/* 
 *=======================================================================================================================
 * mcp9808_initialize() - MCP9808 sensor initialize
 *=======================================================================================================================
 */
void mcp9808_initialize() {
  Output("MCP9808:INIT");
  
  // 1st MCP9808 Precision I2C Temperature Sensor (I2C ADDRESS = 0x18)
  mcp1 = Adafruit_MCP9808();
  if (!mcp1.begin(MCP_ADDRESS_1)) {
    msgp = (char *) "MCP1 NF";
    MCP_1_exists = false;
    SystemStatusBits |= SSB_MCP_1;  // Turn On Bit
  }
  else {
    MCP_1_exists = true;
    msgp = (char *) "MCP1 OK";
  }
  Output (msgp);

  // 2nd MCP9808 Precision I2C Temperature Sensor (I2C ADDRESS = 0x19)
  mcp2 = Adafruit_MCP9808();
  if (!mcp2.begin(MCP_ADDRESS_2)) {
    msgp = (char *) "MCP2 NF";
    MCP_2_exists = false;
    SystemStatusBits |= SSB_MCP_2;  // Turn On Bit
  }
  else {
    MCP_2_exists = true;
    msgp = (char *) "MCP2 OK";
  }
  Output (msgp);
}

/* 
 *=======================================================================================================================
 * sht_initialize() - SHT31 sensor initialize
 *=======================================================================================================================
 */
void sht_initialize() {
  Output("SHT:INIT");
  
  // 1st SHT31 I2C Temperature/Humidity Sensor (I2C ADDRESS = 0x44)
  sht1 = Adafruit_SHT31();
  if (!sht1.begin(SHT_ADDRESS_1)) {
    msgp = (char *) "SHT1 NF";
    SHT_1_exists = false;
    SystemStatusBits |= SSB_SHT_1;  // Turn On Bit
  }
  else {
    SHT_1_exists = true;
    msgp = (char *) "SHT1 OK";
  }
  Output (msgp);

  // 2nd SHT31 I2C Temperature/Humidity Sensor (I2C ADDRESS = 0x45)
  sht2 = Adafruit_SHT31();
  if (!sht2.begin(SHT_ADDRESS_2)) {
    msgp = (char *) "SHT2 NF";
    SHT_2_exists = false;
    SystemStatusBits |= SSB_SHT_2;  // Turn On Bit
  }
  else {
    SHT_2_exists = true;
    msgp = (char *) "SHT2 OK";
  }
  Output (msgp);
}

/* 
 *=======================================================================================================================
 * hih8_initialize() - HIH8000 sensor initialize
 *=======================================================================================================================
 */
void hih8_initialize() {
  Output("HIH8:INIT");

  if (I2C_Device_Exist(HIH8000_ADDRESS)) {
    HIH8_exists = true;
    msgp = (char *) "HIH8 OK";
  }
  else {
    msgp = (char *) "HIH8 NF";
    HIH8_exists = false;
    SystemStatusBits |= SSB_HIH8;  // Turn On Bit
  }
  Output (msgp);
}

/* 
 *=======================================================================================================================
 * hih8_getTempHumid() - Get Temp and Humidity
 *   Call example:  status = hih8_getTempHumid(&t, &h);
 *=======================================================================================================================
 */
bool hih8_getTempHumid(float *t, float *h) {
  if (HIH8_exists) {
    uint16_t humidityBuffer    = 0;
    uint16_t temperatureBuffer = 0;
  
    Wire.begin();
    Wire.beginTransmission(HIH8000_ADDRESS);

    Wire.write(0x00); // set the register location for read request

    delayMicroseconds(200); // give some time for sensor to process request

    if (Wire.requestFrom(HIH8000_ADDRESS, 4) == 4) {

      // Get raw humidity data
      humidityBuffer = Wire.read();
      humidityBuffer <<= 8;
      humidityBuffer |= Wire.read();
      humidityBuffer &= 0x3FFF;   // 14bit value, get rid of the upper 2 status bits

      // Get raw temperature data
      temperatureBuffer = Wire.read();
      temperatureBuffer <<= 8;
      temperatureBuffer |= Wire.read();
      temperatureBuffer >>= 2;  // Remove the last two "Do Not Care" bits (shift left is same as divide by 4)

      Wire.endTransmission();

      *h = humidityBuffer * 6.10e-3;
      *t = temperatureBuffer * 1.007e-2 - 40.0;

      // QC Check
      *h = (isnan(*h) || (*h < QC_MIN_RH) || (*h >QC_MAX_RH)) ? QC_ERR_RH : *h;
      *t = (isnan(*t) || (*t < QC_MIN_T)  || (*t >QC_MAX_T))  ? QC_ERR_T  : *t;
      return (true);
    }
    else {
      Wire.endTransmission();
      return(false);
    }
  }
  else {
    return (false);
  }
}

/* 
 *=======================================================================================================================
 * si1145_initialize() - SI1145 sensor initialize
 *=======================================================================================================================
 */
void si1145_initialize() {
  Output("SI1145:INIT");
  
  // SSB_SI1145 UV index & IR & Visible Sensor (I2C ADDRESS = 0x60)
  if (! uv.begin(&Wire)) {
    Output ("SI:NF");
    SI1145_exists = false;
    SystemStatusBits |= SSB_SI1145;  // Turn On Bit
  }
  else {
    SI1145_exists = true;
    Output ("SI:OK");
    si_last_vis = uv.readVisible();
    si_last_ir = uv.readIR();
    si_last_uv = uv.readUV()/100.0;

    sprintf (msgbuf, "SI:VI[%d.%02d]", (int)si_last_vis, (int)(si_last_vis*100.0)%100); 
    Output (msgbuf);
    sprintf (msgbuf, "SI:IR[%d.%02d]", (int)si_last_ir, (int)(si_last_ir*100.0)%100); 
    Output (msgbuf);
    sprintf (msgbuf, "SI:UV[%d.%02d]", (int)si_last_uv, (int)(si_last_uv*100.0)%100); 
    Output (msgbuf);
  }
}

/* 
 *=======================================================================================================================
 * lux_initialize() - VEML7700 sensor initialize
 * 
 * SEE https://learn.microsoft.com/en-us/windows/win32/sensorsapi/understanding-and-interpreting-lux-values
 * 
 * This data set is for illustration and may not be completely accurate for all users or situations.
 * 
 * Lighting condition     From (lux)     To (lux)     Mean value (lux)     Lighting step
 * Pitch Black            0              10           5                    1
 * Very Dark              11             50           30                   2
 * Dark Indoors           51             200          125                  3
 * Dim Indoors            201            400          300                  4
 * Normal Indoors         401            1000         700                  5
 * Bright Indoors         1001           5000         3000                 6
 * Dim Outdoors           5001           10,000       7500                 7
 * Cloudy Outdoors        10,001         30,000       20,000               8
 * Direct Sunlight        30,001         100,000      65,000               9
 * 
 * From www.vishay.com - Designing the VEML7700 Into an Application
 * 1    lx Full moon overhead at tropical latitudes
 * 3.4  lx Dark limit of civil twilight under a clear sky
 * 50   lx Family living room
 * 80   lx Hallway / bathroom
 * 100  lx Very dark overcast day
 * 320  lx to 500 lx Office lighting
 * 400  lx Sunrise or sunset on a clear day
 * 1000 lx Overcast day; typical TV studio lighting
 * 
 *=======================================================================================================================
 */
void lux_initialize() {
  Output("LUX:INIT");

  if (veml.begin()) {
    VEML7700_exists = true;
    msgp = (char *) "LUX OK";
  }
  else {
    msgp = (char *) "LUX NF";
    VEML7700_exists = false;
    SystemStatusBits |= SSB_LUX;  // Turn On Bit
  }
  Output (msgp);
}

/* 
 *=======================================================================================================================
 * distance_gauge_median()
 *=======================================================================================================================
 */
unsigned int distance_gauge_median() {
  int i;

  for (i=0; i<OD_BUCKETS; i++) {
    // delay(500);
    delay(250);
    od_buckets[i] = (int) analogRead(DISTANCEGAUGE);
    // sprintf (Buffer32Bytes, "SG[%02d]:%d", i, od_buckets[i]);
    // OutputNS (Buffer32Bytes);
  }
  
  mysort(od_buckets, OD_BUCKETS);
  i = (OD_BUCKETS+1) / 2 - 1; // -1 as array indexing in C starts from 0
  
  return (od_buckets[i]/4); // Pins are 12bit resolution (0-4095) Sensor values are 0-1023
}

/*
 * ======================================================================================================================
 * OBS_Do() - Collect Observations, Build message, Send to logging site
 * ======================================================================================================================
 */
void OBS_Do() {
  float bmx1_pressure = 0.0;
  float bmx1_temp = 0.0;
  float bmx1_humid = 0.0;
  float bmx2_pressure = 0.0;
  float bmx2_temp = 0.0;
  float bmx2_humid = 0.0;
  float htu1_temp = 0.0;
  float htu1_humid = 0.0;
  float mcp1_temp = 0.0;
  float mcp2_temp = 0.0;
  float st1 = 0.0;
  float sh1 = 0.0;
  float st2 = 0.0;
  float sh2 = 0.0;
  float ht2 = 0.0; // HIH8
  float hh2 = 0.0; // HIH8
  float lux = 0.0;
  float si_vis = 0.0;
  float si_ir = 0.0;
  float si_uv = 0.0;

  // Safty Check for Vaild Time
  if (!Time.isValid()) {
    Output ("OBS_Do: Time NV");
    return;
  }

  // Take multiple readings and return the median
  int OD_Median = distance_gauge_median();
  int BatteryState = 0;
  float BatteryPoC = 0.0;                 // Battery Percent of Charge
  byte cfr = 0;

  // Adafruit I2C Sensors
  if (BMX_1_exists) {
    float p = 0.0;
    float t = 0.0;
    float h = 0.0;

    if (BMX_1_chip_id == BMP280_CHIP_ID) {
      p = bmp1.readPressure()/100.0F;       // bp1 hPa
      t = bmp1.readTemperature();           // bt1
    }
    else if (BMX_1_chip_id == BME280_BMP390_CHIP_ID) {
      if (BMX_1_type == BMX_TYPE_BME280) {
        p = bme1.readPressure()/100.0F;     // bp1 hPa
        t = bme1.readTemperature();         // bt1
        h = bme1.readHumidity();            // bh1 
      }
      if (BMX_1_type == BMX_TYPE_BMP390) {
        p = bm31.readPressure()/100.0F;     // bp1 hPa
        t = bm31.readTemperature();         // bt1 
      }    
    }
    else { // BMP388
      p = bm31.readPressure()/100.0F;       // bp1 hPa
      t = bm31.readTemperature();           // bt1
    }
    bmx1_pressure = (isnan(p) || (p < QC_MIN_P)  || (p > QC_MAX_P))  ? QC_ERR_P  : p;
    bmx1_temp     = (isnan(t) || (t < QC_MIN_T)  || (t > QC_MAX_T))  ? QC_ERR_T  : t;
    bmx1_humid    = (isnan(h) || (h < QC_MIN_RH) || (h > QC_MAX_RH)) ? QC_ERR_RH : h;
  }

  if (BMX_2_exists) {
    float p = 0.0;
    float t = 0.0;
    float h = 0.0;

    if (BMX_2_chip_id == BMP280_CHIP_ID) {
      p = bmp2.readPressure()/100.0F;       // bp2 hPa
      t = bmp2.readTemperature();           // bt2
    }
    else if (BMX_2_chip_id == BME280_BMP390_CHIP_ID) {
      if (BMX_2_type == BMX_TYPE_BME280) {
        p = bme2.readPressure()/100.0F;     // bp2 hPa
        t = bme2.readTemperature();         // bt2
        h = bme2.readHumidity();            // bh2 
      }
      if (BMX_2_type == BMX_TYPE_BMP390) {
        p = bm32.readPressure()/100.0F;     // bp2 hPa
        t = bm32.readTemperature();         // bt2       
      }
    }
    else { // BMP388
      p = bm32.readPressure()/100.0F;       // bp2 hPa
      t = bm32.readTemperature();           // bt2
    }
    bmx2_pressure = (isnan(p) || (p < QC_MIN_P)  || (p > QC_MAX_P))  ? QC_ERR_P  : p;
    bmx2_temp     = (isnan(t) || (t < QC_MIN_T)  || (t > QC_MAX_T))  ? QC_ERR_T  : t;
    bmx2_humid    = (isnan(h) || (h < QC_MIN_RH) || (h > QC_MAX_RH)) ? QC_ERR_RH : h;
  }

  if (HTU21DF_exists) {
    htu1_humid = htu.readHumidity();
    htu1_humid = (isnan(htu1_humid) || (htu1_humid < QC_MIN_RH) || (htu1_humid > QC_MAX_RH)) ? QC_ERR_RH : htu1_humid;

    htu1_temp = htu.readTemperature();
    htu1_temp = (isnan(htu1_temp) || (htu1_temp < QC_MIN_T)  || (htu1_temp > QC_MAX_T))  ? QC_ERR_T  : htu1_temp;
  }

  if (SHT_1_exists) {
    st1 = sht1.readTemperature();
    st1 = (isnan(st1) || (st1 < QC_MIN_T)  || (st1 > QC_MAX_T))  ? QC_ERR_T  : st1;
    sh1 = sht1.readHumidity();
    sh1 = (isnan(sh1) || (sh1 < QC_MIN_RH) || (sh1 > QC_MAX_RH)) ? QC_ERR_RH : sh1;
  }

  if (SHT_2_exists) {
    st2 = sht2.readTemperature();
    st2 = (isnan(st2) || (st2 < QC_MIN_T)  || (st2 > QC_MAX_T))  ? QC_ERR_T  : st2;
    sh2 = sht2.readHumidity();
    sh2 = (isnan(sh2) || (sh2 < QC_MIN_RH) || (sh2 > QC_MAX_RH)) ? QC_ERR_RH : sh2;
  }

  if (HIH8_exists) {
    bool status = hih8_getTempHumid(&ht2, &hh2);
    if (!status) {
      ht2 = -999.99;
      hh2 = 0.0;
    }
    ht2 = (isnan(ht2) || (ht2 < QC_MIN_T)  || (ht2 > QC_MAX_T))  ? QC_ERR_T  : ht2;
    hh2 = (isnan(hh2) || (hh2 < QC_MIN_RH) || (hh2 > QC_MAX_RH)) ? QC_ERR_RH : hh2;
  }

  if (SI1145_exists) {
    si_vis = uv.readVisible();
    si_ir = uv.readIR();
    si_uv = uv.readUV()/100.0;

    // Additional code to force sensor online if we are getting 0.0s back.
    if ( ((si_vis+si_ir+si_uv) == 0.0) && ((si_last_vis+si_last_ir+si_last_uv) != 0.0) ) {
      // Let Reset The SI1145 and try again
      Output ("SI RESET");
      if (uv.begin()) {
        SI1145_exists = true;
        Output ("SI ONLINE");
        SystemStatusBits &= ~SSB_SI1145; // Turn Off Bit

        si_vis = uv.readVisible();
        si_ir = uv.readIR();
        si_uv = uv.readUV()/100.0;
      }
      else {
        SI1145_exists = false;
        Output ("SI OFFLINE");
        SystemStatusBits |= SSB_SI1145;  // Turn On Bit    
      }
    }

    // Save current readings for next loop around compare
    si_last_vis = si_vis;
    si_last_ir = si_ir;
    si_last_uv = si_uv;

    // QC Checks
    si_vis = (isnan(si_vis) || (si_vis < QC_MIN_VI)  || (si_vis > QC_MAX_VI)) ? QC_ERR_VI  : si_vis;
    si_ir  = (isnan(si_ir)  || (si_ir  < QC_MIN_IR)  || (si_ir  > QC_MAX_IR)) ? QC_ERR_IR  : si_ir;
    si_uv  = (isnan(si_uv)  || (si_uv  < QC_MIN_UV)  || (si_uv  > QC_MAX_UV)) ? QC_ERR_UV  : si_uv;
  }

  if (MCP_1_exists) {
    mcp1_temp = mcp1.readTempC();
    mcp1_temp = (isnan(mcp1_temp) || (mcp1_temp < QC_MIN_T)  || (mcp1_temp > QC_MAX_T))  ? QC_ERR_T  : mcp1_temp;
  }
  if (MCP_2_exists) {
    mcp2_temp = mcp2.readTempC();
    mcp2_temp = (isnan(mcp2_temp) || (mcp2_temp < QC_MIN_T)  || (mcp2_temp > QC_MAX_T))  ? QC_ERR_T  : mcp2_temp;
  }

  if (VEML7700_exists) {
    float lux = veml.readLux(VEML_LUX_AUTO);
    lux = (isnan(lux) || (lux < QC_MIN_LX)  || (lux > QC_MAX_LX))  ? QC_ERR_LX  : lux;
  }

  float SignalStrength = 0;

#if PLATFORM_ID == PLATFORM_ARGON
  WiFiSignal sig = WiFi.RSSI();
  SignalStrength = sig.getStrength();
#else
  if (Cellular.ready()) {
    CellularSignal sig = Cellular.RSSI();
    SignalStrength = sig.getStrength();
  }
  // Get Battery Charger Failt Register
  cfr = pmic.getFault();

  BatteryState = System.batteryState();
  // Read battery charge information only if battery is connected. 
  if (BatteryState>0 && BatteryState<6) {
    BatteryPoC = System.batteryCharge();
  }
#endif

  stc_timestamp();
  Output(timestamp);

  // Report if we have Need to Send Observations
  if (SD_exists && SD.exists(SD_n2s_file)) {
    SystemStatusBits |= SSB_N2S; // Turn on Bit
  }
  else {
    SystemStatusBits &= ~SSB_N2S; // Turn Off Bit
  }

  memset(msgbuf, 0, sizeof(msgbuf));
  JSONBufferWriter writer(msgbuf, sizeof(msgbuf)-1);
  writer.beginObject();
    writer.name("at").value(timestamp);
    writer.name("sg").value(OD_Median);

    if (BMX_1_exists) {
      writer.name("bp1").value(bmx1_pressure, 4);
      writer.name("bt1").value(bmx1_temp, 2);
      writer.name("bh1").value(bmx1_humid, 2);
    }
    if (BMX_2_exists) {
      writer.name("bp2").value(bmx2_pressure, 4);
      writer.name("bt2").value(bmx2_temp, 2);
      writer.name("bh2").value(bmx2_humid, 2);
    }
    if (HTU21DF_exists) {
      writer.name("ht1").value(htu1_temp, 2);
      writer.name("hh1").value(htu1_humid, 2);
    }
    if (MCP_1_exists) {
      writer.name("mt1").value(mcp1_temp, 2);
    }
    if (MCP_2_exists) {
      writer.name("mt2").value(mcp2_temp, 2);
    }
    if (SHT_1_exists) {
      writer.name("st1").value(st1, 2);
      writer.name("sh1").value(st1, 2);
    }
    if (SHT_2_exists) {
      writer.name("st2").value(st2, 2);
      writer.name("sh2").value(st2, 2);
    }
    if (HIH8_exists) {
      writer.name("ht2").value(ht2, 2);
      writer.name("hh2").value(ht2, 2);
    }
    if (SI1145_exists) {
      writer.name("sv1").value(si_vis, 2);
      writer.name("si1").value(si_ir, 2);
      writer.name("su1").value(si_uv, 2);
    }
    if (VEML7700_exists) {
      writer.name("lx").value(lux, 2);
    }

    writer.name("bcs").value(BatteryState);
    writer.name("bpc").value(BatteryPoC, 4);
    writer.name("cfr").value(cfr);
    writer.name("css").value(SignalStrength, 4);
    writer.name("hth").value(SystemStatusBits);
  writer.endObject();

  // Log Observation to SD Card
  SD_LogObservation(msgbuf);
  Serial_write (msgbuf);

  lastOBS = System.millis();

  Output ("Publish(SG)");
  if (Particle_Publish()) {
    PostedResults = true;

    if (SD_exists) {
      Output ("Publish(OK)");
    }
    else {
      Output ("Publish(OK)-NO SD!!!");
    }

    // If we Published, Lets try send N2S observations
    SD_N2S_Publish();
  }
  else {
    PostedResults = false;

    Output ("Publish(FAILED)");
    
    // Set the bit so when we finally transmit the observation,
    // we know it cam from the N2S file.
    SystemStatusBits |= SSB_FROM_N2S; // Turn On Bit

    memset(msgbuf, 0, sizeof(msgbuf));
    // SEE https://docs.particle.io/reference/device-os/firmware/argon/#jsonwriter
    JSONBufferWriter writer(msgbuf, sizeof(msgbuf)-1);
    writer.beginObject();
      writer.name("at").value(timestamp);
      writer.name("sg").value(OD_Median);

      if (BMX_1_exists) {
        // sprintf (Buffer32Bytes, "%d.%02d", (int)bmx1_pressure, (int)(bmx1_pressure*100)%100);
        // writer.name("bp1").value(Buffer32Bytes);
        writer.name("bp1").value(bmx1_pressure, 4);
        writer.name("bt1").value(bmx1_temp, 4);
        writer.name("bh1").value(bmx1_humid, 4);
      }
      if (BMX_2_exists) {
        // sprintf (Buffer32Bytes, "%d.%02d", (int)bmx2_pressure, (int)(bmx2_pressure*100)%100);
        // writer.name("bp2").value(Buffer32Bytes);
        writer.name("bp2").value(bmx2_pressure, 4);
        writer.name("bt2").value(bmx2_temp, 4);
        writer.name("bh2").value(bmx2_humid, 4);
      }
      if (HTU21DF_exists) {
        writer.name("ht1").value(htu1_temp, 2);
        writer.name("hh1").value(htu1_humid, 2);
      }
      if (MCP_1_exists) {
        writer.name("mt1").value(mcp1_temp, 2);
      }
      if (MCP_2_exists) {
        writer.name("mt2").value(mcp2_temp, 2);
      }
      if (SHT_1_exists) {
        writer.name("st1").value(st1, 2);
        writer.name("sh1").value(st1, 2);
      }
      if (SHT_2_exists) {
        writer.name("st2").value(st2, 2);
        writer.name("sh2").value(st2, 2);
      }
      if (HIH8_exists) {
        writer.name("ht2").value(ht2, 2);
        writer.name("hh2").value(ht2, 2);
      }
      if (SI1145_exists) {
        writer.name("sv1").value(si_vis, 2);
        writer.name("si1").value(si_ir, 2);
        writer.name("su1").value(si_uv, 2);
      }
      if (VEML7700_exists) {
        writer.name("lx").value(lux, 2);
      } 
      writer.name("bcs").value(BatteryState);
      writer.name("bpc").value(BatteryPoC, 4);
      writer.name("cfr").value(cfr);
      writer.name("css").value(SignalStrength, 4);
      writer.name("hth").value(SystemStatusBits);
    writer.endObject();
    SystemStatusBits &= ~SSB_FROM_N2S; // Turn Off Bit

    SD_NeedToSend_Add(msgbuf);
  }

  Output(timestamp);

  sprintf (msgbuf, "%d %d.%02d %d.%02d", OD_Median,
    (int)bmx1_pressure, (int)(bmx1_pressure*100)%100,
    (int)bmx2_pressure, (int)(bmx2_pressure*100)%100);
  Output(msgbuf);

  sprintf (msgbuf, "C%d.%02d B%d:%d.%02d %04X", 
    (int)SignalStrength, (int)(SignalStrength*100)%100,
    BatteryState, 
    (int)BatteryPoC, (int)(BatteryPoC*100)%100,
    SystemStatusBits);
  Output(msgbuf);
}

/*
 * ======================================================================================================================
 * Output_CellBatteryInfo() - On OLED display station information
 * ======================================================================================================================
 */
void Output_CellBatteryInfo() {
#if PLATFORM_ID == PLATFORM_ARGON
  WiFiSignal sig = WiFi.RSSI();
  float SignalStrength = sig.getStrength();

  sprintf (Buffer32Bytes, "CS:%d.%02d", 
    (int)SignalStrength, (int)(SignalStrength*100)%100);
  Output(Buffer32Bytes);
#else
  CellularSignal sig = Cellular.RSSI();
  float SignalStrength = sig.getStrength();
  
  int BatteryState = System.batteryState();
  float BatteryPoC = 0.0;                 // Battery Percent of Charge

  // Read battery charge information only if battery is connected. 
  if (BatteryState>0 && BatteryState<6) {
    BatteryPoC = System.batteryCharge();
  }
  
  sprintf (Buffer32Bytes, "CS:%d.%02d B:%d,%d.%02d", 
    (int)SignalStrength, (int)(SignalStrength*100)%100,
    BatteryState, (int)BatteryPoC, (int)(BatteryPoC*100)%100);
  Output(Buffer32Bytes);
#endif
}

/*
 * ======================================================================================================================
 * I2C_Check_Sensors() - See if each I2C sensor responds on the bus and take action accordingly             
 * ======================================================================================================================
 */
void I2C_Check_Sensors() {

  // BMX_1 Barometric Pressure 
  if (I2C_Device_Exist (BMX_ADDRESS_1)) {
    // Sensor online but our state had it offline
    if (BMX_1_exists == false) {
      if (BMX_1_chip_id == BMP280_CHIP_ID) {
        if (bmp1.begin(BMX_ADDRESS_1)) { 
          BMX_1_exists = true;
          Output ("BMP1 ONLINE");
          SystemStatusBits &= ~SSB_BMX_1; // Turn Off Bit
        } 
      }
      else if (BMX_1_chip_id == BME280_BMP390_CHIP_ID) {
        if (BMX_1_type == BMX_TYPE_BME280) {
          if (bme1.begin(BMX_ADDRESS_1)) { 
            BMX_1_exists = true;
            Output ("BME1 ONLINE");
            SystemStatusBits &= ~SSB_BMX_1; // Turn Off Bit
          } 
        }
        if (BMX_1_type == BMX_TYPE_BMP390) {
          if (bm31.begin_I2C(BMX_ADDRESS_1)) {
            BMX_1_exists = true;
            Output ("BMP390_1 ONLINE");
            SystemStatusBits &= ~SSB_BMX_1; // Turn Off Bit
          }
        }        
      }
      else {
        if (bm31.begin_I2C(BMX_ADDRESS_1)) { 
          BMX_1_exists = true;
          Output ("BM31 ONLINE");
          SystemStatusBits &= ~SSB_BMX_1; // Turn Off Bit
        }                  
      }      
    }
  }
  else {
    // Sensor offline but our state has it online
    if (BMX_1_exists == true) {
      BMX_1_exists = false;
      Output ("BMX1 OFFLINE");
      SystemStatusBits |= SSB_BMX_1;  // Turn On Bit 
    }    
  }

  // BMX_2 Barometric Pressure 
  if (I2C_Device_Exist (BMX_ADDRESS_2)) {
    // Sensor online but our state had it offline
    if (BMX_2_exists == false) {
      if (BMX_2_chip_id == BMP280_CHIP_ID) {
        if (bmp2.begin(BMX_ADDRESS_2)) { 
          BMX_2_exists = true;
          Output ("BMP2 ONLINE");
          SystemStatusBits &= ~SSB_BMX_2; // Turn Off Bit
        } 
      }
      else if (BMX_2_chip_id == BME280_BMP390_CHIP_ID) {
        if (BMX_2_type == BMX_TYPE_BME280) {
          if (bme1.begin(BMX_ADDRESS_2)) { 
            BMX_2_exists = true;
            Output ("BME2 ONLINE");
            SystemStatusBits &= ~SSB_BMX_2; // Turn Off Bit
          } 
        }
        if (BMX_2_type == BMX_TYPE_BMP390) {
          if (bm31.begin_I2C(BMX_ADDRESS_2)) {
            BMX_1_exists = true;
            Output ("BMP390_1 ONLINE");
            SystemStatusBits &= ~SSB_BMX_2; // Turn Off Bit
          }
        }        
      }
      else {
         if (bm32.begin_I2C(BMX_ADDRESS_2)) { 
          BMX_2_exists = true;
          Output ("BM32 ONLINE");
          SystemStatusBits &= ~SSB_BMX_2; // Turn Off Bit
        }                         
      }     
    }
  }
  else {
    // Sensor offline but we our state has it online
    if (BMX_2_exists == true) {
      BMX_2_exists = false;
      Output ("BMX2 OFFLINE");
      SystemStatusBits |= SSB_BMX_2;  // Turn On Bit 
    }    
  }

  // HTU21DF Humidity & Temp Sensor
  if (I2C_Device_Exist (HTU21DF_I2CADDR)) {
    // Sensor online but our state had it offline
    if (HTU21DF_exists == false) {
      // See if we can bring sensor online
      if (htu.begin()) {
        HTU21DF_exists = true;
        Output ("HTU ONLINE");
        SystemStatusBits &= ~SSB_HTU21DF; // Turn Off Bit
      }
    }
  }
  else {
    // Sensor offline but we our state has it online
    if (HTU21DF_exists == true) {
      HTU21DF_exists = false;
      Output ("HTU OFFLINE");
      SystemStatusBits |= SSB_HTU21DF;  // Turn On Bit
    }   
  }

  // SI1145 UV index & IR & Visible Sensor
  if (I2C_Device_Exist (SI1145_ADDR)) {
    // Sensor online but our state had it offline
    if (SI1145_exists == false) {
      // See if we can bring sensore online
      if (uv.begin()) {
        SI1145_exists = true;
        Output ("SI ONLINE");
        SystemStatusBits &= ~SSB_SI1145; // Turn Off Bit
      }
    }
  }
  else {
    // Sensor offline but we our state has it online
    if (SI1145_exists == true) {
      SI1145_exists = false;
      Output ("SI OFFLINE");
      SystemStatusBits |= SSB_SI1145;  // Turn On Bit
    }   
  }

  // VEML7700 Lux 
  if (I2C_Device_Exist (VEML7700_ADDRESS)) {
    // Sensor online but our state had it offline
    if (VEML7700_exists == false) {
      // See if we can bring sensor online
      if (veml.begin()) {
        VEML7700_exists = true;
        Output ("LUX ONLINE");
        SystemStatusBits &= ~SSB_LUX; // Turn Off Bit
      }
    }
  }
  else {
    // Sensor offline but we our state has it online
    if (VEML7700_exists == true) {
      VEML7700_exists = false;
      Output ("LUX OFFLINE");
      SystemStatusBits |= SSB_LUX;  // Turn On Bit
    }   
  }
}

/*
 * ======================================================================================================================
 * Particle_Publish() - Publish to Particle what is in msgbuf
 * ======================================================================================================================
 */
bool Particle_Publish() {
  // Calling Particle.publish() when the cloud connection has been turned off will not publish an event. 
  // This is indicated by the return success code of false. If the cloud connection is turned on and 
  // trying to connect to the cloud unsuccessfully, Particle.publish() may block for up to 20 seconds 
  // (normal conditions) to 10 minutes (unusual conditions). Checking Particle.connected() 
  // before calling Particle.publish() can help prevent this.
  // if (Cellular.ready() && Particle.connected()) {
  if (Particle.connected()) {
    if (Particle.publish("SG", msgbuf,  WITH_ACK)) {  // PRIVATE flag is always used even when not specified
      // Currently, a device can publish at rate of about 1 event/sec, with bursts of up to 4 allowed in 1 second. 
      delay (1000);
      return(true);
    }
  }
  else {
    Output ("Particle:NotReady");
  }
  return(false);
}

/*
 * ======================================================================================================================
 * JPO_ClearBits() - Clear System Status Bits related to initialization
 * ======================================================================================================================
 */
void JPO_ClearBits() {
  if (JustPoweredOn) {
    JustPoweredOn = false;
    SystemStatusBits &= ~SSB_PWRON;   // Turn Off Power On Bit
    SystemStatusBits &= ~SSB_OLED;    // Turn Off OLED Missing Bit
    SystemStatusBits &= ~SSB_BMX_1;   // Turn Off BMX280_1 Not Found Bit
    SystemStatusBits &= ~SSB_BMX_2;   // Turn Off BMX280_1 Not Found Bit
    SystemStatusBits &= ~SSB_HTU21DF; // Turn Off HTU Not Found Bit
    SystemStatusBits &= ~SSB_MCP_1;   // Turn Off MCP_1 Not Found Bit
    SystemStatusBits &= ~SSB_MCP_2;   // Turn Off MCP_2 Not Found Bit
    SystemStatusBits &= ~SSB_SHT_1;   // Turn Off SHT_1 Not Found Bit
    SystemStatusBits &= ~SSB_SHT_2;   // Turn Off SHT_1 Not Found Bit
    SystemStatusBits &= ~SSB_HIH8;    // Turn Off HIH Not Found Bit
    SystemStatusBits &= ~SSB_LUX;     // Turn Off VEML7700 Not Found Bit
    SystemStatusBits &= ~SSB_SI1145;  // Turn Off UV,IR, VIS Not Found Bit
  }
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
 * RTC_UpdateCheck() - Check if we need to Set or Update the RTC clock from the Cell Network   
 * ======================================================================================================================
 */
void RTC_UpdateCheck() {
  if (RTC_exists && NetworkReady) { 
    // We have a RTC and We have connected to the Cell network at some point
    if (!RTC_valid) {
      // Set Uninitialized RTC from STC. Which has been set from the Cloud
      rtc.adjust(DateTime(Time.year(), Time.month(), Time.day(), Time.hour(), Time.minute(), Time.second() ));
      Output("RTC: Set");
      RTC_valid = true;
    }
    else if (LastTimeUpdate == 0){
      // We have never updated RTC from Cell network time 
      rtc.adjust(DateTime(Time.year(), Time.month(), Time.day(), Time.hour(), Time.minute(), Time.second() ));

      Output("RTC: 1ST SYNC");
      rtc_timestamp();
      sprintf (msgbuf, "%s*", timestamp);
      Output (msgbuf);
      LastTimeUpdate = System.millis();
    }
    else if ((System.millis() - LastTimeUpdate) >= 2*3600*1000) {  // It's been 2 hours since last RTC update
      rtc.adjust(DateTime(Time.year(), Time.month(), Time.day(), Time.hour(), Time.minute(), Time.second() ));

      Output("RTC: 2HR SYNC");
      rtc_timestamp();
      sprintf (msgbuf, "%s*", timestamp);
      Output (msgbuf);
      LastTimeUpdate = System.millis();
    }
  }
}

#if PLATFORM_ID == PLATFORM_ARGON
/*
 * ======================================================================================================================
 * WiFiChangeCheck() - Check for WIFI.TXT file and set Wireless SSID, Password            
 * ======================================================================================================================
 */
void WiFiChangeCheck() {
  File fp;
  int i=0;
  char *p, *auth, *ssid, *pw;
  char ch, buf[128];

  if (SD_exists) {
    // Test for file WIFI.TXT
    if (SD.exists(SD_wifi_file)) {
      fp = SD.open(SD_wifi_file, FILE_READ); // Open the file for reading, starting at the beginning of the file.

      if (fp) {
        // Deal with too small or too big of file
        if (fp.size()<=7 || fp.size()>127) {
          fp.close();
          Output ("WIFI:Invalid SZ");
        }
        else {
          Output ("WIFI:Open");
          // Read one line from file
          while (fp.available() && (i < 127 )) {
            ch = fp.read();

            // sprintf (msgbuf, "%02X : %c", ch, ch);
            // Output (msgbuf);

            if ((ch == 0x0A) || (ch == 0x0D) ) {  // newline or linefeed
              break;
            }
            else {
              buf[i++] = ch;
            }
          }
          fp.close();
          Output ("WIFI:Close");

          // At this point we have encountered EOF, CR, or LF
          // Now we need to terminate array with a null to make it a string
          buf[i] = (char) NULL;

          // Parse string for the following
          //   WIFI ssid password
          p = &buf[0];
          auth = strtok_r(p, ",", &p);

          if (auth == NULL) {
            Output("WIFI:ID=Null Err");
          }
          else if ( (strcmp (auth, "WEP") != 0)  &&
                    (strcmp (auth, "WPA") != 0)  &&
                    (strcmp (auth, "WPA2") != 0) ){
            sprintf (msgbuf, "WIFI:ATYPE[%s] Err", auth);          
            Output(msgbuf);
          }
          else {
            ssid = strtok_r(p, ",", &p);
            pw  = strtok_r(p, ",", &p);

            if (ssid == NULL) {
              Output("WIFI:SSID=Null Err");
            }
            else if (pw == NULL) {
              Output("WIFI:PW=Null Err");
            }
            else {
              sprintf (msgbuf, "WIFI:ATYPE[%s]", auth);          
              Output(msgbuf);
              sprintf (msgbuf, "WIFI:SSID[%s]", ssid);
              Output(msgbuf);
              sprintf (msgbuf, "WIFI:PW[%s]", pw);
              Output(msgbuf);

              // Connects to a network secured with WPA2 credentials.
              // https://docs.particle.io/reference/device-os/api/wifi/securitytype-enum/
              if (strcmp (auth, "UNSEC") == 0) {
                Output("WIFI:Credentials Cleared");
                WiFi.clearCredentials();
                Output("WIFI:Credentials Set UNSEC");
                WiFi.setCredentials(ssid, pw, UNSEC);
              }
              else if (strcmp (auth, "WEP") == 0) {
                Output("WIFI:Credentials Cleared");
                WiFi.clearCredentials();
                Output("WIFI:Credentials Set WEP");
                WiFi.setCredentials(ssid, pw, WEP);
              }
              else if (strcmp (auth, "WPA") == 0) {
                Output("WIFI:Credentials Cleared");
                WiFi.clearCredentials();
                Output("WIFI:Credentials Set WPA");
                WiFi.setCredentials(ssid, pw, WPA);
              }
              else if (strcmp (auth, "WPA2") == 0) {
                Output("WIFI:Credentials Cleared");
                WiFi.clearCredentials();
                Output("WIFI:Credentials Set WPA2");
                WiFi.setCredentials(ssid, pw, WPA2);
              }
              else if (strcmp (auth, "WPA_ENTERPRISE") == 0) {
                Output("WIFI:Credentials Cleared");
                WiFi.clearCredentials();
                Output("WIFI:Credentials Set WPAE");
                WiFi.setCredentials(ssid, pw, WPA_ENTERPRISE);
              }
              else if (strcmp (auth, "WPA2_ENTERPRISE") == 0) {
                Output("WIFI:Credentials Cleared");
                WiFi.clearCredentials();
                Output("WIFI:Credentials Set WPAE2");
                WiFi.setCredentials(ssid, pw, WPA2_ENTERPRISE);
              }
              else { 
                Output("WIFI:Credentials NOT Set");
                Output("WIFI:USING NVAUTH");
              }
            }
          }
        }
      }
      else {
        sprintf (msgbuf, "WIFI:Open[%s] Err", SD_wifi_file);          
        Output(msgbuf);
        Output ("WIFI:USING NVAUTH");
      }
    } 
    else {
      Output ("WIFI:NOFILE USING NVAUTH");
    }
  } // SD enabled
  else {
    Output ("WIFI:NOSD USING NVAUTH");
  }
}
#else
/*
 * ======================================================================================================================
 * SimChangeCheck() - Check for SIM.TXT file and set sim based on contents, after rename file to SIMOLD.TXT            
 * ======================================================================================================================
 */
void SimChangeCheck() {
  File fp;
  int i=0;
  char *p, *id, *apn, *un, *pw;
  char ch, buf[128];
  bool changed = false;

  SimType simType = Cellular.getActiveSim();

  if (simType == 1) {
    Output ("SIM:Internal");
  } else if (simType == 2) {
    Output ("SIM:External");
  }
  else {
    sprintf (msgbuf, "SIM:Unknown[%d]", simType);
    Output (msgbuf);
  }

  if (SerialConsoleEnabled && SD_exists) {
    // Test for file SIM.TXT
    if (SD.exists(SD_sim_file)) {
      fp = SD.open(SD_sim_file, FILE_READ); // Open the file for reading, starting at the beginning of the file.

      if (fp) {
        // Deal with too small or too big of file
        if (fp.size()<=7 || fp.size()>127) {
          fp.close();
          Output ("SIMF:Invalid SZ");
          if (SD.remove (SD_sim_file)) {
            Output ("SIMF->Del:OK");
          }
          else {
            Output ("SIMF->Del:Err");
          }
        }
        else {
          Output ("SIMF:Open");
          while (fp.available() && (i < 127 )) {
            ch = fp.read();

            if ((ch == 0x0A) || (ch == 0x0D) ) {  // newline or linefeed
              break;
            }
            else {
              buf[i++] = ch;
            }
          }

          // At this point we have encountered EOF, CR, or LF
          // Now we need to terminate array with a null to make it a string
          buf[i] = NULL;

          // Parse string for the following
          //   INTERNAL
          //   AUP epc.tmobile.com username passwd
          //   UP username password
          //   APN epc.tmobile.com
          p = &buf[0];
          id = strtok_r(p, " ", &p);

          if (id != NULL) {
            sprintf (msgbuf, "SIMF:ID[%s]", id);
            Output(msgbuf);
          }

          if (strcmp (id,"INTERNAL") == 0) {
            Cellular.setActiveSim(INTERNAL_SIM);
            Cellular.clearCredentials();
          }
          else if (strcmp (id,"APN") == 0) {
            apn = strtok_r(p, " ", &p);

            if (apn == NULL) {
              Output("SIMF:APN=Null Err");
            }
            else {
              Cellular.setActiveSim(EXTERNAL_SIM);
              Output("SIM:Set External-APN");

              // Connects to a cellular network by APN only
              Cellular.setCredentials(apn);
              Output("SIM:Set Credentials");
              sprintf (msgbuf, " APN[%s]", apn);
              Output(msgbuf);
              changed = true;
            }
          }
          else if (strcmp (id," UP") == 0) {
            un  = strtok_r(p, " ", &p);
            pw  = strtok_r(p, " ", &p);

            if (un == NULL) {
              Output("SIMF:Username=Null Err");
            }
            else if (pw == NULL) {
              Output("SIMF:Passwd=Null Err");
            }
            else {
              Cellular.setActiveSim(EXTERNAL_SIM);
              Output("SIM:Set External-UP");

              // Connects to a cellular network with USERNAME and PASSWORD only
              Cellular.setCredentials(un,pw);
              Output("SIM:Set Credentials");
              sprintf (msgbuf, " UN[%s]", un);
              Output(msgbuf);
              sprintf (msgbuf, " PW[%s]", pw);
              Output(msgbuf);
              changed = true;
            }
          }
          else if (strcmp (id,"AUP") == 0) {
            apn = strtok_r(p, " ", &p);
            un  = strtok_r(p, " ", &p);
            pw  = strtok_r(p, " ", &p);

            if (apn == NULL) {
              Output("SIMF:APN=Null Err");
            }
            else if (un == NULL) {
              Output("SIMF:Username=Null Err");
            }
            else if (pw == NULL) {
              Output("SIMF:Passwd=Null Err");
            }
            else {
              Cellular.setActiveSim(EXTERNAL_SIM);
              Output("SIM:Set External-AUP");

              // Connects to a cellular network with a specified APN, USERNAME and PASSWORD
              Cellular.setCredentials(apn,un,pw);
              Output("SIM:Set Credentials");
              sprintf (msgbuf, " APN[%s]", apn);
              Output(msgbuf);
              sprintf (msgbuf, "  UN[%s]", un);
              Output(msgbuf);
              sprintf (msgbuf, "  PW[%s]", pw);
              Output(msgbuf);
              changed = true;
            }
          }
          else {  // Pasrse Error
            sprintf (msgbuf, "SIMF:ID[%s] Err", id);
            Output(msgbuf);
          }
        }

        // No matter what happened with the above, rename file so we don't process again at boot
        // if SIMOLD.TXT exists then remove it before we rename SIM.TXT
        if (SD.exists(SD_simold_file)) {
          if (SD.remove (SD_simold_file)) {
            Output ("SIMF:DEL SIMOLD");
          }
        }

        if (!fp.rename(SD_simold_file)) {
          Output ("SIMF:RENAME ERROR");
        }
        else {
          Output ("SIMF:RENAME OK");
        }
        fp.close();

        // Notify user to reboot blink led forever
        if (changed) {
          Output ("==============");
          Output ("!!! REBOOT !!!");
          Output ("==============");

        }
        else {
          Output ("=====================");
          Output ("!!! SET SIM ERROR !!!");
          Output ("=====================");
        }
        
        while(true) { // wait for Host to open serial port
          Blink(1, 750);
        }
      }
      else {
        Output ("SIMF:OPEN ERROR");
      }
    } 
    else {
      Output ("SIM:NO UPDATE FILE");
    }
  } // Console and SD enabled
}
#endif

/*
 * ======================================================================================================================
 * StationMonitor() - On OLED display station information
 * ======================================================================================================================
 */
void StationMonitor() {
  static int cycle = 0;
  int r, c, len;

  // Clear display with spaces
  for (r=0; r<4; r++) {
    for (c=0; c<22; c++) {
      oled_lines [r][c] = ' ';
    }
    oled_lines [r][c] = (char) NULL;
  }
  
  // =================================================================
  // Line 0 of OLED
  // =================================================================
  stc_timestamp();
  len = (strlen (timestamp) > 21) ? 21 : strlen (timestamp);
  for (c=0; c<=len; c++) oled_lines [0][c] = *(timestamp+c);
  Serial_writeln (timestamp);

  // =================================================================
  // Line 1 of OLED Distance Median & Distance Raw
  // =================================================================
  memset(msgbuf, 0, sizeof(msgbuf));
  sprintf (msgbuf+strlen(msgbuf), "DIST M:%d R:%d %s", 
    distance_gauge_median(), analogRead(DISTANCEGAUGE),
    (od_adjustment == 1.25) ? "5M" : "10M"
    );

  len = (strlen (msgbuf) > 21) ? 21 : strlen (msgbuf);
  for (c=0; c<=len; c++) oled_lines [1][c] = *(msgbuf+c);
  Serial_writeln (msgbuf);

  // =================================================================
  // Line 2 of OLED Wind Direction and Wind Speed
  // =================================================================
  memset(msgbuf, 0, sizeof(msgbuf));

#if PLATFORM_ID == PLATFORM_ARGON
    WiFiSignal sig = WiFi.RSSI();
    float SignalStrength = sig.getStrength();
    int BatteryState = 0;
#else
    CellularSignal sig = Cellular.RSSI();
    float SignalStrength = sig.getStrength();
#endif

    sprintf (msgbuf+strlen(msgbuf), "%c S:%d.%02d H:%X",
      (Particle.connected()) ? '+' : '-',
      (int)SignalStrength, (int)(SignalStrength*100)%100,
      (int)SystemStatusBits); 
  
  len = (strlen (msgbuf) > 21) ? 21 : strlen (msgbuf);
  for (c=0; c<=len; c++) oled_lines [2][c] = *(msgbuf+c);
  Serial_writeln (msgbuf);
  
  // =================================================================
  // Line 3 of OLED Cycle between multiple sensors
  // =================================================================
  if (cycle == 0) {
    if (BMX_1_exists) {
      float bmx_pressure = 0.0;
      float bmx_temp = 0.0;
      float bmx_humid;
      
      switch (BMX_1_chip_id) {
        case BMP280_CHIP_ID :
          bmx_pressure = bmp1.readPressure()/100.0F;           // bmxp1
          bmx_temp = bmp1.readTemperature();                   // bmxt1
          break;
        
        case BME280_BMP390_CHIP_ID :
          if (BMX_1_chip_id == BME280_BMP390_CHIP_ID) {
            bmx_pressure = bme1.readPressure()/100.0F;           // bmxp1
            bmx_temp = bme1.readTemperature();                   // bmxt1
            bmx_humid = bme1.readHumidity();                     // bmxh1 
          }
          else { // BMP390
            bmx_pressure = bm31.readPressure()/100.0F;
            bmx_temp = bm31.readTemperature();
          }
          break;
          
        case BMP388_CHIP_ID :
          bmx_pressure = bm31.readPressure()/100.0F;
          bmx_temp = bm31.readTemperature();
          break;
        
        default: // WTF
          break;
      }
      sprintf (msgbuf, "B1 %d.%02d %d.%02d %d.%02d", 
        (int)bmx_pressure, (int)(bmx_pressure*100)%100,
        (int)bmx_temp, (int)(bmx_temp*100)%100,
        (int)bmx_humid, (int)(bmx_humid*100)%100);
    }
    else {
      sprintf (msgbuf, "B1 NF");
    }
  }
  
  if (cycle == 1) {
    if (BMX_2_exists) {
      float bmx_pressure = 0.0;
      float bmx_temp = 0.0;
      float bmx_humid;
      
      switch (BMX_2_chip_id) {
        case BMP280_CHIP_ID :
          bmx_pressure = bmp2.readPressure()/100.0F;           // bmxp1
          bmx_temp = bmp1.readTemperature();                   // bmxt1
          break;
        
        case BME280_BMP390_CHIP_ID :
          if (BMX_2_chip_id == BME280_BMP390_CHIP_ID) {
            bmx_pressure = bme2.readPressure()/100.0F;           // bmxp1
            bmx_temp = bme1.readTemperature();                   // bmxt1
            bmx_humid = bme1.readHumidity();                     // bmxh1 
          }
          else { // BMP390
            bmx_pressure = bm32.readPressure()/100.0F;
            bmx_temp = bm31.readTemperature();
          }
          break;
          
        case BMP388_CHIP_ID :
          bmx_pressure = bm32.readPressure()/100.0F;
          bmx_temp = bm31.readTemperature();
          break;
        
        default: // WTF
          break;
      }
      sprintf (msgbuf, "B2 %d.%02d %d.%02d %d.%02d", 
        (int)bmx_pressure, (int)(bmx_pressure*100)%100,
        (int)bmx_temp, (int)(bmx_temp*100)%100,
        (int)bmx_humid, (int)(bmx_humid*100)%100);
    }
    else {
      sprintf (msgbuf, "B2 NF");
    }
  }

  if (cycle == 2) {
    memset(msgbuf, 0, sizeof(msgbuf));
      
    if (MCP_1_exists) {
      float mcp_temp = mcp1.readTempC();   
      sprintf (msgbuf, "MCP1 T%d.%02d", (int)mcp_temp, (int)(mcp_temp*100)%100);
    }
    else {
      sprintf (msgbuf, "MCP1 NF");
    }
  }

  if (cycle == 3) {
    if (MCP_2_exists) {
      float mcp_temp = mcp2.readTempC();   
      sprintf (msgbuf, "MCP2 T%d.%02d", (int)mcp_temp, (int)(mcp_temp*100)%100);
    }
    else {
      sprintf (msgbuf, "MCP2 NF");
    }
  }

  if (cycle == 4) {   
    if (HTU21DF_exists) {
      float htu_humid = htu.readHumidity();
      float htu_temp = htu.readTemperature();

      sprintf (msgbuf, "HTU H:%02d.%02d T:%02d.%02d", 
        (int)htu_humid, (int)(htu_humid*100)%100, 
        (int)htu_temp, (int)(htu_temp*100)%100);
    }
    else {
      sprintf (msgbuf, "HTU NF"); 
    } 
  }

  if (cycle == 5) {   
    if (VEML7700_exists) {
      float lux = veml.readLux(VEML_LUX_AUTO);
      lux = (isnan(lux)) ? 0.0 : lux;
        sprintf (msgbuf, "LX L%02d.%1d", (int)lux, (int)(lux*10)%10);
    }
    else {
      sprintf (msgbuf, "LX NF");
    }
  }

  if (cycle == 6) {
    if (SHT_1_exists) {
      float t = sht1.readTemperature();
      float h = sht1.readHumidity();
      sprintf (msgbuf, "SHT1 T:%d.%02d H:%d.%02d", 
         (int)t, (int)(t*100)%100,
         (int)h, (int)(h*100)%100);
    }
    else {
      sprintf (msgbuf, "SHT1 NF");
    }
  }

  if (cycle == 7) {
    if (SHT_2_exists) {
      float t = sht2.readTemperature();
      float h = sht2.readHumidity();
      sprintf (msgbuf, "SHT2 T:%d.%02d H:%d.%02d", 
         (int)t, (int)(t*100)%100,
         (int)h, (int)(h*100)%100);
    }
    else {
      sprintf (msgbuf, "SHT2 NF");
    }      
  }

  if (cycle == 8) {   
    if (HIH8_exists) {
      float t = 0.0;
      float h = 0.0;
      bool status = hih8_getTempHumid(&t, &h);
      if (!status) {
        t = -999.99;
        h = 0.0;
      }
      sprintf (msgbuf, "HIH8 T%d.%02d H%d.%02d", 
         (int)t, (int)(t*100)%100,
         (int)h, (int)(h*100)%100);
    }
    else {
      sprintf (msgbuf, "HIH8 NF");
    }
  }

  if (cycle == 9) {
    if (SI1145_exists) {
      float si_vis = uv.readVisible();
      float si_ir = uv.readIR();
      float si_uv = uv.readUV()/100.0;
      sprintf (msgbuf, "SI V%d.%02d I%d.%02d U%d.%02d", 
        (int)si_vis, (int)(si_vis*100)%100,
        (int)si_ir, (int)(si_ir*100)%100,
        (int)si_uv, (int)(si_uv*100)%100);
    }
    else {
      sprintf (msgbuf, "SI NF");
    }
  }

  len = (strlen (msgbuf) > 21) ? 21 : strlen (msgbuf);
  for (c=0; c<=len; c++) oled_lines [3][c] = *(msgbuf+c);
  Serial_writeln (msgbuf);

  cycle = ++cycle % 10;
  
  OLED_update();
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
    od_adjustment = 1.25;
    Output ("DIST=5M");
  }
  else {
    od_adjustment = 2.5;
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
