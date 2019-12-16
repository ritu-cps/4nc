#include "libraries/ina226.h"
#include <Wire.h>

const int   INA226_ADDR         = 0x40;                 // INA226 I2C Address (A0=A1=GND)
const word  INA226_CAL_VALUE    = 0x0A00;               // INA226 Calibration Register Value

static void writeRegister(byte reg, word value)
{
  Wire.beginTransmission(INA226_ADDR);
  Wire.write(reg);
  Wire.write((value >> 8) & 0xFF);
  Wire.write(value & 0xFF);
  Wire.endTransmission();
}
static void setupRegister(void)
{
  writeRegister(INA226_REG_CONGIGURATION_REG,
        INA226_CONF_RESET_INACTIVE
      | INA226_CONF_MODE_CONT_SHUNT_AND_BUS
      | INA226_CONF_VSH_1100uS
      | INA226_CONF_VBUS_1100uS
      | INA226_CONF_AVG_64
      );

  writeRegister(INA226_REG_CALIBRATION, INA226_CAL_VALUE);
}

static word readRegister(byte reg){
  word res = 0x0000;

  Wire.beginTransmission(INA226_ADDR);
  Wire.write(reg);

  if(Wire.endTransmission() == 0) {
    if(Wire.requestFrom(INA226_ADDR, 2) >= 2) {
      res = Wire.read() * 256;
      res += Wire.read();
    }
  }

  return res;
}

typedef struct tagREG_INFO {
  byte	reg;
  const char*	name;
}REG_INFO;
const static REG_INFO   st_aRegs[] = {
  { INA226_REG_CONGIGURATION_REG,   "Configuration Register"    },
  { INA226_REG_SHUNT_VOLTAGE,       "Shunt Voltage"             },
  { INA226_REG_BUS_VOLTAGE,         "Bus Voltage"               },
  { INA226_REG_POWER,               "Power"                     },
  { INA226_REG_CURRENT,             "Current"                   },
  { INA226_REG_CALIBRATION,         "Calibration"               },
  { INA226_REG_MASK_ENABLE,         "Mask/Enable"               },
  { INA226_REG_ALERT_LIMIT,         "Alert Limit"               },
  { INA226_REG_DIE_ID,              "Die ID"                    },
};

static void dumpRegisters(void){
  int i;
  const REG_INFO* pInfo;
  static word REGS[NELEMS(st_aRegs)];
  static char  buf[64];

  for(i = 0; i < NELEMS(REGS); i++) {
    pInfo = &st_aRegs[i];
    REGS[i] = readRegister(pInfo->reg);
  }

  Serial.println("---INA226 Registers ---");

  for(i = 0; i < NELEMS(REGS); i++) {
    pInfo = &st_aRegs[i];
    snprintf(buf, NELEMS(buf), "%24s (%02Xh) : %04Xh (%u)", pInfo->name, pInfo->reg, REGS[i], REGS[i]);
    Serial.println(buf);
  }
}


void setup(){
  Wire.begin();
  Serial.begin(9600);
  setupRegister();
}

void loop(){
  char  buf[64];
  long  voltage;   // Bus Voltage (mV)
  short current;   // Current (mA)
  long  power;     // Power (uW)
  voltage  = (long)((short)readRegister(INA226_REG_BUS_VOLTAGE)) * 1250L;    // LSB=1.25mV
  current  = (short)readRegister(INA226_REG_CURRENT);
  power    = (long)readRegister(INA226_REG_POWER) * 25000L;                  // LSB=25mW

  snprintf(buf, NELEMS(buf)
    , "V:\t%5ld\tmV, I:\t%5d\tmA, P:\t%5ld\tmW"
    , (voltage + (1000/2)) / 1000
    , current
    , (power + (1000/2)) / 1000
    );

  Serial.println(buf);

  //dumpRegisters();

  delay(1000);
}
