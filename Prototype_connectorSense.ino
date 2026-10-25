
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// -------- PIN CONFIGURATION --------
#define ONE_WIRE_BUS 4
#define SDA_PIN 21
#define SCL_PIN 22
#define INA226_ADDR 0x44

// -------- TEMPERATURE SENSORS --------
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// -------- DEMO SETTINGS --------
const float BASELINE_R = 0.050f; // Simulated baseline in ohms

// Read a 16-bit INA226 register
uint16_t readINA226(uint8_t reg, bool &ok) {
  Wire.beginTransmission(INA226_ADDR);
  Wire.write(reg);

  if (Wire.endTransmission(false) != 0) {
    ok = false;
    return 0;
  }

  if (Wire.requestFrom(INA226_ADDR, (uint8_t)2) != 2) {
    ok = false;
    return 0;
  }

  uint16_t value = ((uint16_t)Wire.read() << 8);
  value |= Wire.read();

  ok = true;
  return value;
}

void setup() {
  Serial.begin(9600);
  Wire.begin(SDA_PIN, SCL_PIN);
  sensors.begin();

  Serial.println();
  Serial.println("==================================");
  Serial.println("     ConnectorSense EV Monitor");
  Serial.println("==================================");
  Serial.println("Temperature: DS18B20 readings");
  Serial.println("Voltage: INA226 bus register");
  Serial.println("Current: UNVERIFIED");
  Serial.println("Resistance/health: SIMULATED");
  Serial.println("==================================");
}

void loop() {
  // -------- READ TEMPERATURES --------
  sensors.requestTemperatures();

  float connectorTemp = sensors.getTempCByIndex(0);
  float ambientTemp = sensors.getTempCByIndex(1);

  bool connectorOK =
      connectorTemp != DEVICE_DISCONNECTED_C &&
      connectorTemp >= -55 && connectorTemp <= 125;

  bool ambientOK =
      ambientTemp != DEVICE_DISCONNECTED_C &&
      ambientTemp >= -55 && ambientTemp <= 125;

  // -------- READ INA226 --------
  bool busOK = false;
  bool shuntOK = false;

  uint16_t busRaw = readINA226(0x02, busOK);
  uint16_t shuntBits = readINA226(0x01, shuntOK);

  Serial.println();
  Serial.println("--------- LIVE READINGS ---------");

  // Bus voltage: register LSB = 1.25 mV
  if (busOK) {
    float busVoltage = busRaw * 0.00125f;

    Serial.print("Bus voltage: ");
    Serial.print(busVoltage, 3);
    Serial.println(" V");
  } else {
    Serial.println("Bus voltage: SENSOR ERROR");
  }

  // Shunt voltage: signed register, LSB = 2.5 microvolts
  if (shuntOK) {
    int16_t shuntRaw = (int16_t)shuntBits;
    float shunt_mV = shuntRaw * 0.0025f;

    Serial.print("Shunt voltage: ");
    Serial.print(shunt_mV, 4);
    Serial.println(" mV");
  } else {
    Serial.println("Shunt voltage: SENSOR ERROR");
  }

  // Current cannot be reliably derived from this board setup.
  Serial.println("Load current: UNVERIFIED (A)");
  Serial.println("Do not interpret shunt voltage as load current.");

  // -------- TEMPERATURE OUTPUT --------
  if (connectorOK) {
    Serial.print("Connector temperature: ");
    Serial.print(connectorTemp, 2);
    Serial.println(" C");
  } else {
    Serial.println("Connector temperature: SENSOR ERROR");
  }

  if (ambientOK) {
    Serial.print("Ambient temperature: ");
    Serial.print(ambientTemp, 2);
    Serial.println(" C");
  } else {
    Serial.println("Ambient temperature: SENSOR ERROR");
  }

  if (connectorOK && ambientOK) {
    Serial.print("Temperature difference: ");
    Serial.print(connectorTemp - ambientTemp, 2);
    Serial.println(" C");
  } else {
    Serial.println("Temperature difference: UNAVAILABLE");
  }

  // -------- SIMULATED HEALTH DEMO --------
  unsigned long phase = (millis() / 10000UL) % 4;

  float simulatedResistance[] = {
    0.050f, 0.060f, 0.085f, 0.110f
  };

  float resistance = simulatedResistance[phase];
  float ratio = resistance / BASELINE_R;

  float score = 100.0f - (ratio - 1.0f) * 50.0f;
  score = constrain(score, 0.0f, 100.0f);

  const char *status;

  if (ratio < 1.5f) {
    status = "HEALTHY";
  } else if (ratio < 2.0f) {
    status = "WATCH";
  } else {
    status = "ALERT";
  }

  Serial.println("--------- DEMO ANALYSIS ---------");

  Serial.print("SIMULATED resistance: ");
  Serial.print(resistance, 3);
  Serial.println(" ohm");

  Serial.print("SIMULATED resistance ratio: ");
  Serial.print(ratio, 2);
  Serial.println("x baseline");

  Serial.print("DEMO health score: ");
  Serial.print(score, 0);
  Serial.println("/100");

  Serial.print("DEMO status: ");
  Serial.println(status);

  Serial.println("---------------------------------");
  Serial.println("Current and contact resistance");
  Serial.println("are not validated measurements.");
  Serial.println("=================================");

  delay(2000);
}
