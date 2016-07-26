#include <OneWire.h>

byte i;
byte present = 0;
byte type_s;
byte data[12];
byte addr[8];
float celsius;
float sensor1;
float sensor2;
unsigned long ulc;
int fanSpeed[] = {6, 5, 4};
int on = 3;

void setup(void) {
  Serial.begin(115200);

  pinMode(fanSpeed[0], OUTPUT);
  pinMode(fanSpeed[1], OUTPUT);
  pinMode(fanSpeed[2], OUTPUT);
  pinMode(on, OUTPUT);
}

/*

   The recomended safe HDD temperature range is:
   Minimum: Less than 25*C
   Ideal: 25*C to 40*C
   Acceptable: 41*C to 50*C
   Maximum: 50*C

   Fan speed:
   Off: <25*C
   Very low: 25*C
   Low: 30*C
   Medium: 35*C
   Fast: 40*C

*/

void loop(void) {
  get_temp(7, 1);
  get_temp(8, 2);

  //not that redo
  if (sensor1 <= 35  && sensor2 <= 35) {
    if (sensor1 <= 30  && sensor2 <= 30) {
      if (sensor1 < 25 && sensor2 < 25) {
        on = LOW;
      }
    }
  }
}

void get_temp(int x, int count) {

  OneWire  ds(x);  //a 4.7K resistor is necessary

  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }

  Serial.print("ROM =");
  for ( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return;
  }
  Serial.println();

  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad

  // Serial.print("  Data = ");
  // Serial.print(present, HEX);
  // Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    //  Serial.print(data[i], HEX);
    //  Serial.print(" ");
  }
  // Serial.print(" CRC=");
  // Serial.print(OneWire::crc8(data, 8), HEX);
  // Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.println(" *C");
  Serial.print("RAW Temperature = ");
  Serial.print(raw);
  Serial.println(" *C");

  if (count == 1) {
    sensor1 = celsius;
  } else if (count == 2) {
    sensor2 = celsius;
  }

}
