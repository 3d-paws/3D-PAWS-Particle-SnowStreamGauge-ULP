/*
 * ======================================================================================================================
 *  WRD.h - Wind Rain Distance Functions
 * ======================================================================================================================
 */

/*
 * Distance Sensors
 * The 5-meter sensors (MB7360, MB7369, MB7380, and MB7389) use a scale factor of (Vcc/5120) per 1-mm.
 * Particle 12bit resolution (0-4095),  Sensor has a resolution of 0 - 5119mm,  Each unit of the 0-4095 resolution is 1.25mm
 * Feather has 10bit resolution (0-1023), Sensor has a resolution of 0 - 5119mm, Each unit of the 0-1023 resolution is 5mm
 * 
 * The 10-meter sensors (MB7363, MB7366, MB7383, and MB7386) use a scale factor of (Vcc/10240) per 1-mm.
 * Particle 12bit resolution (0-4095), Sensor has a resolution of 0 - 10239mm, Each unit of the 0-4095 resolution is 2.5mm
 * Feather has 10bit resolution (0-1023), Sensor has a resolution of 0 - 10239mm, Each unit of the 0-1023 resolution is 10mm
 */

/*
 * =======================================================================================================================
 *  Distance Gauge
 * =======================================================================================================================
 */
#define DISTANCEGAUGE   A3
#define DG_BUCKETS      60
char SD_5M_DIST_FILE[] = "5MDIST.TXT";  // If file exists use adjustment of 1.25. No file, then 10m Sensor is 2.5
float dg_adjustment = 2.5;              // Default sensor is 10m 
unsigned int dg_bucket = 0;             //  Distance Buckets
unsigned int dg_buckets[DG_BUCKETS];

/* 
 *=======================================================================================================================
 * distance_gauge_median()
 *=======================================================================================================================
 */
unsigned int distance_gauge_median() {
  int i;

  for (i=0; i<DG_BUCKETS; i++) {
    // delay(500);
    delay(250);
    dg_buckets[i] = (int) analogRead(DISTANCEGAUGE) * dg_adjustment;;
    // sprintf (Buffer32Bytes, "SG[%02d]:%d", i, od_buckets[i]);
    // OutputNS (Buffer32Bytes);
  }
  
  mysort(dg_buckets, DG_BUCKETS);
  i = (DG_BUCKETS+1) / 2 - 1; // -1 as array indexing in C starts from 0
  
  return (dg_buckets[i]);
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