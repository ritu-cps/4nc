/*
write TDU 16nc045
use
 arduino uno
 ina226(strawberry-linux)
  VS:3.3V
  SCL:A5(SCL)
  SDA:A4(SDA)
  AL:unused
  GND:GND
 power relay G2R-1
*/
#include "libraries/ina226.h"
#include <Wire.h>

#define relay1 1  // before power-suply1
#define reley2 2  // after power-suply1
#define relay3 3  // before power-suply2
#define relay4 4  // after power-suply2

const int threshold_power = 200;  // 電源を切り替える際の消費電力の閾値
static boolean powersupply_mode = true;  // 現在稼働している電源の確認用フラグ(true:power-suply1 on, false:power-suply2 on)

const int   INA226_ADDR         = 0x40;                 // INA226 I2C Address (A0=A1=GND)
const word  INA226_CAL_VALUE    = 0x0A00;               // INA226 Calibration Register Value

long  voltage;   // Bus Voltage (mV)
short current;   // Current (mA)
long  power;     // Power (uW)

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


void print_measure_power(){ // serialprint measured power
  char  buf[64];
  snprintf(buf, NELEMS(buf)
    , "V:\t%5ld\tmV, I:\t%5d\tmA, P:\t%5ld\tmW"
    , (voltage + (1000/2)) / 1000
    , current
    , (power + (1000/2)) / 1000
    );
  Serial.println(buf);
}

void measure_power(){
  voltage  = (long)((short)readRegister(INA226_REG_BUS_VOLTAGE)) * 1250L;    // LSB=1.25mV
  current  = (short)readRegister(INA226_REG_CURRENT);
  power    = (long)readRegister(INA226_REG_POWER) * 25000L;                  // LSB=25mW
}

void mode_change(boolean nowMode){
  int change_delay = 500; // 電源が起動しきるまでの待ち時間
  if(nowMode){
    digitalWrite(relay3, HIGH);
    digitalWrite(relay4, HIGH);
    delay(change_delay);
    digitalWrite(relay1, LOW);
    digitalWrite(relay2, LOW);
  }
  else{
    digitalWrite(relay1, HIGH);
    digitalWrite(relay2, HIGH);
    delay(change_delay);
    digitalWrite(relay3, LOW);
    digitalWrite(relay4, LOW);
  }
  powersupply_mode = !nowMode;
}

void MA_power(){  // MA = moving average
  static long power_list[10] = {};
  //static int power_list_num = 0;
  static long power_ave = 0;
  static long power_sum = 0;

  power_sum = power_sum + power - power_list[0];
  for (int i = 0; i < 9; i++) // 9 = リスト数 - 1
    power_list[i] = power_list[i+1];
  power_list[9] = power;
  power_ave = power_sum / 10; // 10 = リスト数
}

void setup(){
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);
  Wire.begin();
  Serial.begin(9600);
  setupRegister();

  // setup power list
  power = 0;
  MA_power();
  for (int i = 0; i < 10; i++){
    measure_power();
    power_list[i] = power;
    power_sum = power_sum + power;
  }
}

void loop(){
  measure_power();
  MA_power();
  if (power > threshold_power){
    mode_change(喜島喜島喜島喜島);モードチェンジについて考える
  }


  delay(500);
}
