#include <PolledOneWire.h>

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library

PolledOneWire  ds(10);  // on pin 10
byte addr[8];
byte type_s;
  
void setup(void) {
  byte i;
  Serial.begin(9600);
  
  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }
  
  Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }

  if (PolledOneWire::crc8(addr, 7) != addr[7]) {
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
}

void loop(void) {
  byte i;
  byte present = 0;

  float celsius, fahrenheit;
  unsigned long startclock;
  unsigned long returnclock;
  unsigned long stopclock;

  int iNumPolls;
 
  iNumPolls = 0;
  
  startclock = micros();
  ds.polled_reset();
  returnclock = micros();
  while ( ds.poll_status ) {
     ds.poll(); 
     iNumPolls++;
  }
  stopclock = micros();
  Serial.println( "START CONVERSION" );
  Serial.print( "polled_reset() call time: " );
  Serial.println( returnclock - startclock);
  Serial.print( "Number of Reset polls: " );
  Serial.println( iNumPolls );
  Serial.print( "Total time until finished with polling: " );
  Serial.println( stopclock - startclock );
  Serial.print( "Result: " );
  Serial.println( ds.reset_result );
  
  //ds.select(addr);
  
  iNumPolls = 0;
  ds.polled_skip();
  while ( ds.poll_status ) {
     ds.poll(); 
     iNumPolls++;
  }
  Serial.print( "Number of Skip polls: " );
  Serial.println( iNumPolls );
  
  iNumPolls = 0;
  startclock = micros();
  ds.polled_write(0x44,1);   // Convert Temperature
  returnclock = micros();
  while ( ds.poll_status ) {
     ds.poll(); 
     iNumPolls++;
  }
  stopclock = micros();
  Serial.print( "polled_write() call time: " );
  Serial.println( returnclock - startclock);
  Serial.print( "Number of Write polls: " );
  Serial.println( iNumPolls );
  Serial.print( "Total time until finished with polling: " );
  Serial.println( stopclock - startclock );
  Serial.println();  
  
  delay( 1000 );
  
  iNumPolls = 0;
  ds.polled_reset();
  while ( ds.poll_status ) {
     ds.poll(); 
     iNumPolls++;
  }
  Serial.println( "READ" );
  Serial.print( "Number of Reset polls: " );
  Serial.println( iNumPolls );
  Serial.print( "Result: " );
  Serial.println( ds.reset_result );
  
  iNumPolls = 0;
  startclock = micros();
  ds.polled_select(addr);   
  returnclock = micros();
  while ( ds.poll_status ) {
     ds.poll(); 
     iNumPolls++;
  }
  stopclock = micros();
  Serial.print( "polled_select() call time: " );
  Serial.println( returnclock - startclock);
  Serial.print( "Number of Select polls: " );
  Serial.println( iNumPolls );
  Serial.print( "Total time until finished with polling: " );
  Serial.println( stopclock - startclock );
  
  iNumPolls = 0;
  ds.polled_write(0xBE);        // Read Scratchpad
  while ( ds.poll_status ) {
     ds.poll(); 
     iNumPolls++;
  }
  Serial.print( "Number of Write polls: " );
  Serial.println( iNumPolls ); 

  // we need 9 bytes
  iNumPolls = 0;
  startclock = micros();
  ds.polled_read_bytes( 9 );
  returnclock = micros();
  while ( ds.poll_status ) {
    ds.poll(); 
    iNumPolls++;
  }     
  stopclock = micros();
   
  Serial.print( "polled_read_bytes() call time: " );
  Serial.println( returnclock - startclock);
  Serial.print( "Number of ReadBytes polls: " );
  Serial.println( iNumPolls );
  Serial.print( "Total time until finished with polling: " );
  Serial.println( stopclock - startclock );

  Serial.print("  Data = ");  
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    Serial.print(ds.readWriteBuffer[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(PolledOneWire::crc8(ds.readWriteBuffer, 8), HEX);
  Serial.println();

  // convert the data to actual temperature

  unsigned int raw = (ds.readWriteBuffer[1] << 8) | ds.readWriteBuffer[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (ds.readWriteBuffer[7] == 0x10) {
      // count remain gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - ds.readWriteBuffer[6];
    }
  } else {
    byte cfg = (ds.readWriteBuffer[4] & 0x60);
    if (cfg == 0x00) raw = raw << 3;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw << 2; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw << 1; // 11 bit res, 375 ms
    // default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.print(fahrenheit);
  Serial.println(" Fahrenheit");  
  Serial.println();
}
