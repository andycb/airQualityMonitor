#include "Wire.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define iaqI2CAddress 0x5A
#define SEALEVELPRESSURE_HPA (1013.25)

// Holds data from the BME 280 sensor
struct Bme280Data
{
  float tempriture = 0.0F;
  float humidity = 0.0F;
  float pressure = 0.0F;  
};

// Holds data from the IAQ sensor
struct IaqData
{
  uint8_t statusCode = 0u;
  uint16_t co2Ppm = 0u;
  int32_t resistance = 0;
  uint16_t tvocPpm = 0u;
};

// Reads data out of the IAQ sensor
// Solution provided by this thread: https://forum.arduino.cc/index.php?topic=350712.0
IaqData GetIaqData() 
{
  Wire.requestFrom(iaqI2CAddress, 9);

  IaqData result;
  result.co2Ppm = (Wire.read()<< 8 | Wire.read());
  result.statusCode =  Wire.read();
  result.resistance = (Wire.read()& 0x00)| (Wire.read()<<16)| (Wire.read()<<8| Wire.read());
  result.tvocPpm = (Wire.read()<<8 | Wire.read());
  
  return result;
}

// Reads data from the BME 280 sensor
Bme280Data GetBme280Data(Adafruit_BME280 bme)
{
  bme.takeForcedMeasurement();
  
  Bme280Data result;
  result.tempriture = bme.readTemperature();
  result.pressure = bme.readPressure() / 100.0F;
  result.humidity = bme.readHumidity();
  return result;
}

// The current BME 280 instance
Adafruit_BME280 bme;

// Indicates that the BME 280 is in a good state
bool bmeOkay;

void setup() 
{
  Serial.begin(9600);
  Wire.begin();

  // Create BME280 sesor
  bmeOkay = bme.begin();

  if (bmeOkay)
  {
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X16, // temperature
                    Adafruit_BME280::SAMPLING_X16, // pressure
                    Adafruit_BME280::SAMPLING_X16, // humidity
                    Adafruit_BME280::FILTER_OFF);
  }
}

bool CheckIfCanSendData()
{
  // Wait to recive some data before sending a reading
  if (Serial.available() > 0) 
  {
    while (Serial.available() > 0)
    {
      // Look for a line feed to be sent to us (ASCII 10)
      int incomingByte = Serial.read();
      if (incomingByte == 10)
      {
        return true;
      }
    }
  }
  else
  {
    return false;
  }
}

void loop()
{     
  if (!CheckIfCanSendData())
  {
    return;  
  }

  IaqData iaqData;
  Bme280Data bmeData;
  
  iaqData = GetIaqData();

  if (bmeOkay)
  {
    bmeData = GetBme280Data(bme);
  }

  // Build out the JSON object the hard way

    //   obj = {
    //     status: true,
    //     errorText: '',
    //     iaQStatus: 0,
    //     co2: 100, 
    //     tvoc: 200,
    //     temp: 20,
    //     humidity: 40,
    //     pressure: 1000
    // };

  Serial.print("{");
  Serial.print("\"status\":");
  Serial.print(bmeOkay);
  Serial.print(",");  

  Serial.print("\"iaQStatus\":");
  Serial.print(iaqData.statusCode);
  Serial.print(",");  
  
  Serial.print("\"co2\":");
  Serial.print(iaqData.co2Ppm);
  Serial.print(",");  

  Serial.print("\"tvoc\":");
  Serial.print(iaqData.tvocPpm);
  Serial.print(",");  
  
  Serial.print("\"temp\":");
  Serial.print(bmeData.tempriture);
  Serial.print(",");  

  Serial.print("\"humidity\":");
  Serial.print(bmeData.humidity);  
  Serial.print(",");  
  
  Serial.print("\"pressure\":");
  Serial.print(bmeData.pressure); 
  
  Serial.println("}");
}
