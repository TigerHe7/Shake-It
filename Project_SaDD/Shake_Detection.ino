#include <Wire.h>

void WireRequestArray(int address, uint8_t* buffer, uint8_t amount);
void WireWriteRegister(int address, uint8_t reg, uint8_t value);
void WireWriteByte(int address, uint8_t value);

static float const    SensorMaximumReading = 512.0;
static float const    SensorMaximumAccel  = 9.81 * 4.0;
static uint8_t const  SensorAccelerometer = 0x1D;
static uint32_t const ShakeThresholdeshold = 1000;

static float          ShakeX              = 0.0;
static float          ShakeY              = 0.0;
static float          ShakeZ              = 0.0;

void ShakeInit()
{
  WireWriteRegister(SensorAccelerometer, 0x31, 1);
  WireWriteRegister(SensorAccelerometer, 0x2D, 1 << 3);
}

bool leftTilt()
{
  return ShakeX < -9;
}

bool rightTilt()
{
  return ShakeX > 9;
}

void ShakeTick()
{
  size_t const DataLength = 6;
  uint32_t data[DataLength] = { 0 };

  WireWriteByte(SensorAccelerometer, 0x32);
  WireRequestArray(SensorAccelerometer, data, DataLength);

  uint16_t xi = (data[1] << 8) | data[0];
  uint16_t yi = (data[3] << 8) | data[2];
  uint16_t zi = (data[5] << 8) | data[4];

  ShakeX = *(int16_t*)(&xi) / SensorMaximumReading * SensorMaximumAccel;
  ShakeY = *(int16_t*)(&yi) / SensorMaximumReading * SensorMaximumAccel;
  ShakeZ = *(int16_t*)(&zi) / SensorMaximumReading * SensorMaximumAccel;
//
//  ShakeX = x*x;
//  ShakeY = y*y - 90;
//
//  if (x <= -9) {
//    Serial.print("X SHOOK\n");
//  } else if (x >= 9) {
//    Serial.print("Y SHOOK\n");
//  }
////  
////  Serial.print(x*x);
////  Serial.print(", ");
////  Serial.print(y*y - 90);
////  Serial.print("\n");

}
