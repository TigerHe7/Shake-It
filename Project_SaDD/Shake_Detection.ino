#include <Wire.h>

void WireRequestArray(int address, uint8_t* buffer, uint8_t amount);
void WireWriteRegister(int address, uint8_t reg, uint8_t value);
void WireWriteByte(int address, uint8_t value);

static float const    SensorMaximumReading= 512.0;
static float const    SensorMaximumAccel  = 9.81 * 4.0;
static uint8_t const  SensorAccelerometer = 0x1D;
static uint32_t const ShakeTotalThreshold = 64;
static uint32_t const ShakeThreshold      = ShakeTotalThreshold / 2;

static float          ShakeTotal          = 0;
static float          ShakeXDirection     = 0;
static float          ShakeYDirection     = 0;
static float          ShakeZDirection     = 0;

void ShakeInit()
{
  WireWriteRegister(SensorAccelerometer, 0x31, 1);
  WireWriteRegister(SensorAccelerometer, 0x2D, 1 << 3);
}

bool XShake()
{
  return ShakeXDirection >= ShakeThreshold;
}

bool YShake()
{
  return ShakeYDirection >= ShakeThreshold;
}

bool ZShake()
{
  return ShakeZDirection >= ShakeThreshold;
}

bool RandomShaking()
{
  return ShakeTotal >= ShakeTotalThreshold;
}

int ShakeMag()
{
  return ShakeTotal;
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
  
  float x = *(int16_t*)(&xi) / SensorMaximumReading * SensorMaximumAccel;
  float y = *(int16_t*)(&yi) / SensorMaximumReading * SensorMaximumAccel;
  float z = *(int16_t*)(&zi) / SensorMaximumReading * SensorMaximumAccel;

  ShakeXDirection = x*x;
  ShakeYDirection = y*y;
  ShakeZDirection = z*z;
  ShakeTotal = x*x + y*y + z*z;
}

