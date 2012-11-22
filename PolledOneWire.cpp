/*
Polled OneWire Modifications Copyright (c) 2012, Ryan Pierce, rdpierce@pobox.com

The latest version of this library may be found at:
https://github.com/RyanPierce/PolledOneWire

Polled OneWire Version 0.1:
  Initial version, Ryan Pierce
  
Overview: The original OneWire library makes use of blocking calls. E.g. a
reset() includes 1000 microseconds of delay. In very time critical applications
where other things must happen concurrently with OneWire operations (e.g. doing
phase dimming of an AC circuit), blocking for this long is not acceptable. These
modifications are designed to minimize intentional delays. Rather, one makes a call
to a function, and while poll_status != 0, calls the poll() function. The intent is
that other short operations can be interleaved between calls to poll(). Note that
blocking still does occur; it is unavoidable as writing or reading individual bits
requires precise timing, generally accomplished by a call to delayMicros() with
interrupts disabled.

The rule of thumb is that delays of any call to a function or, subsequently, to
poll() must not include substantial delay. So far, the largest delay is 80 us.
It is possible that excessive time between polls could cause problems. If at all
possible, only interleave simple operations (e.g. checking a timer and setting a
pin) between polls. Things like serial operations are best done outside of polling.

Calling OneWire functions (polled or otherwise) other than poll() while 
poll_status != 0 is a recipe for disaster. While building checking for this into this code 
is possible, in the interest of performance and space such checks are deleted.

write_bit() is unchanged. It includes 65 or 70 us of delay.
read_bit() is unchanged. It includes 66 us of delay.

NOTE: These two functions could be optimized in the future. read_bit() only has 13 us of 
critical delay; 50 us could be replaced with polling. write_bit()'s timing depends on the 
bit value. Writing a 1 only requires 10 us critical delay and 55 us time that could
be replaced with polling. Writing a 0 can't be optimized; 65 us delay is
criticual, and the remaining 5 us delay isn't worth optimizing.

The following functions are optimized:

polled_reset() - No delay.
Subsequent poll() - A single poll will have 80 us delay. All other polls will have no
explicit delay. Total polling time: 1000 - 1250 us. The member variable reset_result
will be true if any devices are present on the bus, false otherwise.

polled_write() - 65-70 us delay.
Subsequent poll() - 7 polls each with 65-70 us delay.

polled_read() - 66 us delay.
Subsequent poll() - 7 polls each with 66 us delay. When it completes, the resulting byte
will be stored in the member variable readWriteByte

Note: All multi-byte operations must read or write <= ONEWIRE_MAX_READ_WRITE_BUFFER_LEN
which may be redefined in user code. This should never be less than 9 as that is the number
of bytes needed for a Select.

polled_write_bytes() - Makes N calls to polled_write(). There will be 8N-1 calls to poll()
needed for the operation to complete, each with 65-70 us delay.

polled_read_bytes() - Makes N calls to polled_read(). There will be 8N calls to poll() needed
for the operation to complete, and all but the last will have 66 us delay; the last will have no
delay. When it completes, the resulting bytes will be stored in the member variable 
readWriteBuffer.

polled_select() - A shortcut that does a polled write of 9 bytes.

polled_skip() - A shortcut that does a polled write of 1 byte.

At present, search functions are not optimized. It is anticipated that searching for
OneWire devices will happen on startup before timing-critical things need to happen.
  
Copyright (c) 2007, Jim Studt  (original old version - many contributors since)

The latest version of this library may be found at:
  http://www.pjrc.com/teensy/td_libs_OneWire.html

Version 2.1:
  Arduino 1.0 compatibility, Paul Stoffregen
  Improve temperature example, Paul Stoffregen
  DS250x_PROM example, Guillermo Lovato
  PIC32 (chipKit) compatibility, Jason Dangel, dangel.jason AT gmail.com
  Improvements from Glenn Trewitt:
  - crc16() now works
  - check_crc16() does all of calculation/checking work.
  - Added read_bytes() and write_bytes(), to reduce tedious loops.
  - Added ds2408 example.
  Delete very old, out-of-date readme file (info is here)

Version 2.0: Modifications by Paul Stoffregen, January 2010:
http://www.pjrc.com/teensy/td_libs_OneWire.html
  Search fix from Robin James
    http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1238032295/27#27
  Use direct optimized I/O in all cases
  Disable interrupts during timing critical sections
    (this solves many random communication errors)
  Disable interrupts during read-modify-write I/O
  Reduce RAM consumption by eliminating unnecessary
    variables and trimming many to 8 bits
  Optimize both crc8 - table version moved to flash

Modified to work with larger numbers of devices - avoids loop.
Tested in Arduino 11 alpha with 12 sensors.
26 Sept 2008 -- Robin James
http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1238032295/27#27

Updated to work with arduino-0008 and to include skip() as of
2007/07/06. --RJL20

Modified to calculate the 8-bit CRC directly, avoiding the need for
the 256-byte lookup table to be loaded in RAM.  Tested in arduino-0010
-- Tom Pollard, Jan 23, 2008

Jim Studt's original library was modified by Josh Larios.

Tom Pollard, pollard@alum.mit.edu, contributed around May 20, 2008

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Much of the code was inspired by Derek Yerger's code, though I don't
think much of that remains.  In any event that was..
    (copyleft) 2006 by Derek Yerger - Free to distribute freely.

The CRC code was excerpted and inspired by the Dallas Semiconductor
sample code bearing this copyright.
//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of Dallas Semiconductor
// shall not be used except as stated in the Dallas Semiconductor
// Branding Policy.
//--------------------------------------------------------------------------
*/

#include "PolledOneWire.h"


PolledOneWire::PolledOneWire(uint8_t pin)
{
	pinMode(pin, INPUT);
	bitmask = PIN_TO_BITMASK(pin);
	baseReg = PIN_TO_BASEREG(pin);
	poll_status = ONEWIRE_POLLSTAT_NONE;
#if ONEWIRE_SEARCH
	reset_search();
#endif
}


// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted
// and we return a 0;
//
// Returns 1 if a device asserted a presence pulse, 0 otherwise.
//
uint8_t PolledOneWire::reset(void)
{
	IO_REG_TYPE mask = bitmask;
	volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;
	uint8_t r;
	uint8_t retries = 125;

	noInterrupts();
	DIRECT_MODE_INPUT(reg, mask);
	interrupts();
	// wait until the wire is high... just in case
	do {
		if (--retries == 0) return 0;
		delayMicroseconds(2);
	} while ( !DIRECT_READ(reg, mask));

	noInterrupts();
	DIRECT_WRITE_LOW(reg, mask);
	DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
	interrupts();
	delayMicroseconds(500);
	noInterrupts();
	DIRECT_MODE_INPUT(reg, mask);	// allow it to float
	delayMicroseconds(80);
	r = !DIRECT_READ(reg, mask);
	interrupts();
	delayMicroseconds(420);
	return r;
}

//
// Write a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
void PolledOneWire::write_bit(uint8_t v)
{
	IO_REG_TYPE mask=bitmask;
	volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;

	if (v & 1) {
		noInterrupts();
		DIRECT_WRITE_LOW(reg, mask);
		DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
		delayMicroseconds(10);
		DIRECT_WRITE_HIGH(reg, mask);	// drive output high
		interrupts();
		delayMicroseconds(55);
	} else {
		noInterrupts();
		DIRECT_WRITE_LOW(reg, mask);
		DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
		delayMicroseconds(65);
		DIRECT_WRITE_HIGH(reg, mask);	// drive output high
		interrupts();
		delayMicroseconds(5);
	}
}

//
// Read a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
uint8_t PolledOneWire::read_bit(void)
{
	IO_REG_TYPE mask=bitmask;
	volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;
	uint8_t r;

	noInterrupts();
	DIRECT_MODE_OUTPUT(reg, mask);
	DIRECT_WRITE_LOW(reg, mask);
	delayMicroseconds(3);
	DIRECT_MODE_INPUT(reg, mask);	// let pin float, pull up will raise
	delayMicroseconds(10);
	r = DIRECT_READ(reg, mask);
	interrupts();
	delayMicroseconds(53);
	return r;
}

//
// Write a byte. The writing code uses the active drivers to raise the
// pin high, if you need power after the write (e.g. DS18S20 in
// parasite power mode) then set 'power' to 1, otherwise the pin will
// go tri-state at the end of the write to avoid heating in a short or
// other mishap.
//
void PolledOneWire::write(uint8_t v, uint8_t power /* = 0 */) {
    uint8_t bitMask;

    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
	PolledOneWire::write_bit( (bitMask & v)?1:0);
    }
    if ( !power) {
	noInterrupts();
	DIRECT_MODE_INPUT(baseReg, bitmask);
	DIRECT_WRITE_LOW(baseReg, bitmask);
	interrupts();
    }
}

void PolledOneWire::write_bytes(const uint8_t *buf, uint16_t count, bool power /* = 0 */) {
  for (uint16_t i = 0 ; i < count ; i++)
    write(buf[i]);
  if (!power) {
    noInterrupts();
    DIRECT_MODE_INPUT(baseReg, bitmask);
    DIRECT_WRITE_LOW(baseReg, bitmask);
    interrupts();
  }
}

//
// Read a byte
//
uint8_t PolledOneWire::read() {
    uint8_t bitMask;
    uint8_t r = 0;

    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
	if ( PolledOneWire::read_bit()) r |= bitMask;
    }
    return r;
}

void PolledOneWire::read_bytes(uint8_t *buf, uint16_t count) {
  for (uint16_t i = 0 ; i < count ; i++)
    buf[i] = read();
}

//
// Do a ROM select
//
void PolledOneWire::select( uint8_t rom[8])
{
    int i;

    write(0x55);           // Choose ROM

    for( i = 0; i < 8; i++) write(rom[i]);
}

//
// Do a ROM skip
//
void PolledOneWire::skip()
{
    write(0xCC);           // Skip ROM
}

void PolledOneWire::depower()
{
	noInterrupts();
	DIRECT_MODE_INPUT(baseReg, bitmask);
	interrupts();
}

#if ONEWIRE_SEARCH

//
// You need to use this function to start a search again from the beginning.
// You do not need to do it for the first search, though you could.
//
void PolledOneWire::reset_search()
  {
  // reset the search state
  LastDiscrepancy = 0;
  LastDeviceFlag = FALSE;
  LastFamilyDiscrepancy = 0;
  for(int i = 7; ; i--)
    {
    ROM_NO[i] = 0;
    if ( i == 0) break;
    }
  }

//
// Perform a search. If this function returns a '1' then it has
// enumerated the next device and you may retrieve the ROM from the
// PolledOneWire::address variable. If there are no devices, no further
// devices, or something horrible happens in the middle of the
// enumeration then a 0 is returned.  If a new device is found then
// its address is copied to newAddr.  Use PolledOneWire::reset_search() to
// start over.
//
// --- Replaced by the one from the Dallas Semiconductor web site ---
//--------------------------------------------------------------------------
// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
uint8_t PolledOneWire::search(uint8_t *newAddr)
{
   uint8_t id_bit_number;
   uint8_t last_zero, rom_byte_number, search_result;
   uint8_t id_bit, cmp_id_bit;

   unsigned char rom_byte_mask, search_direction;

   // initialize for search
   id_bit_number = 1;
   last_zero = 0;
   rom_byte_number = 0;
   rom_byte_mask = 1;
   search_result = 0;

   // if the last call was not the last one
   if (!LastDeviceFlag)
   {
      // 1-Wire reset
      if (!reset())
      {
         // reset the search
         LastDiscrepancy = 0;
         LastDeviceFlag = FALSE;
         LastFamilyDiscrepancy = 0;
         return FALSE;
      }

      // issue the search command
      write(0xF0);

      // loop to do the search
      do
      {
         // read a bit and its complement
         id_bit = read_bit();
         cmp_id_bit = read_bit();

         // check for no devices on 1-wire
         if ((id_bit == 1) && (cmp_id_bit == 1))
            break;
         else
         {
            // all devices coupled have 0 or 1
            if (id_bit != cmp_id_bit)
               search_direction = id_bit;  // bit write value for search
            else
            {
               // if this discrepancy if before the Last Discrepancy
               // on a previous next then pick the same as last time
               if (id_bit_number < LastDiscrepancy)
                  search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
               else
                  // if equal to last pick 1, if not then pick 0
                  search_direction = (id_bit_number == LastDiscrepancy);

               // if 0 was picked then record its position in LastZero
               if (search_direction == 0)
               {
                  last_zero = id_bit_number;

                  // check for Last discrepancy in family
                  if (last_zero < 9)
                     LastFamilyDiscrepancy = last_zero;
               }
            }

            // set or clear the bit in the ROM byte rom_byte_number
            // with mask rom_byte_mask
            if (search_direction == 1)
              ROM_NO[rom_byte_number] |= rom_byte_mask;
            else
              ROM_NO[rom_byte_number] &= ~rom_byte_mask;

            // serial number search direction write bit
            write_bit(search_direction);

            // increment the byte counter id_bit_number
            // and shift the mask rom_byte_mask
            id_bit_number++;
            rom_byte_mask <<= 1;

            // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
            if (rom_byte_mask == 0)
            {
                rom_byte_number++;
                rom_byte_mask = 1;
            }
         }
      }
      while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

      // if the search was successful then
      if (!(id_bit_number < 65))
      {
         // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
         LastDiscrepancy = last_zero;

         // check for last device
         if (LastDiscrepancy == 0)
            LastDeviceFlag = TRUE;

         search_result = TRUE;
      }
   }

   // if no device found then reset counters so next 'search' will be like a first
   if (!search_result || !ROM_NO[0])
   {
      LastDiscrepancy = 0;
      LastDeviceFlag = FALSE;
      LastFamilyDiscrepancy = 0;
      search_result = FALSE;
   }
   for (int i = 0; i < 8; i++) newAddr[i] = ROM_NO[i];
   return search_result;
  }

#endif

#if ONEWIRE_CRC
// The 1-Wire CRC scheme is described in Maxim Application Note 27:
// "Understanding and Using Cyclic Redundancy Checks with Maxim iButton Products"
//

#if ONEWIRE_CRC8_TABLE
// This table comes from Dallas sample code where it is freely reusable,
// though Copyright (C) 2000 Dallas Semiconductor Corporation
static const uint8_t PROGMEM dscrc_table[] = {
      0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
    157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
     35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
    190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
     70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
    219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
    101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
    248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
    140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
     17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
    175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
     50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
    202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
     87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
    233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
    116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

//
// Compute a Dallas Semiconductor 8 bit CRC. These show up in the ROM
// and the registers.  (note: this might better be done without to
// table, it would probably be smaller and certainly fast enough
// compared to all those delayMicrosecond() calls.  But I got
// confused, so I use this table from the examples.)
//
uint8_t PolledOneWire::crc8( uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;

	while (len--) {
		crc = pgm_read_byte(dscrc_table + (crc ^ *addr++));
	}
	return crc;
}
#else
//
// Compute a Dallas Semiconductor 8 bit CRC directly.
// this is much slower, but much smaller, than the lookup table.
//
uint8_t PolledOneWire::crc8( uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;
	
	while (len--) {
		uint8_t inbyte = *addr++;
		for (uint8_t i = 8; i; i--) {
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) crc ^= 0x8C;
			inbyte >>= 1;
		}
	}
	return crc;
}
#endif

#if ONEWIRE_CRC16
bool PolledOneWire::check_crc16(uint8_t* input, uint16_t len, uint8_t* inverted_crc)
{
    uint16_t crc = ~crc16(input, len);
    return (crc & 0xFF) == inverted_crc[0] && (crc >> 8) == inverted_crc[1];
}

uint16_t PolledOneWire::crc16(uint8_t* input, uint16_t len)
{
    static const uint8_t oddparity[16] =
        { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };
    uint16_t crc = 0;    // Starting seed is zero.

    for (uint16_t i = 0 ; i < len ; i++) {
      // Even though we're just copying a byte from the input,
      // we'll be doing 16-bit computation with it.
      uint16_t cdata = input[i];
      cdata = (cdata ^ (crc & 0xff)) & 0xff;
      crc >>= 8;

      if (oddparity[cdata & 0x0F] ^ oddparity[cdata >> 4])
          crc ^= 0xC001;

      cdata <<= 6;
      crc ^= cdata;
      cdata <<= 1;
      crc ^= cdata;
    }
    return crc;
}
#endif

// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted
// and we return reset_status = false;
//
// Ultimately returns reset_status = true if a device asserted a presence pulse, false otherwise.
//
void PolledOneWire::polled_reset()
{
	poll_status |= ONEWIRE_POLLSTAT_RESET;
	IO_REG_TYPE mask = bitmask;
	volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;

	noInterrupts();
	DIRECT_MODE_INPUT(reg, mask);
	interrupts();
	
	// Now check if the line is high. If not, wait up to 250 us.
	if ( DIRECT_READ(reg, mask) ) {
		// Success!
		noInterrupts();
		DIRECT_WRITE_LOW(reg, mask);
		DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
		interrupts();
		bitNextTime = micros();
		bitNextTime += 500; // Line should stay low for 500 us
		bit_status = ONEWIRE_BITSTAT_RESET_WAIT_LOW;
		return;
	} else {
		bitNextTime = micros();
		bitNextTime += 250; // Line should go high by 250 ms
		bit_status = ONEWIRE_BITSTAT_RESET_WAIT_LINE_HIGH;
		return;
	}
}

//
// Write a byte. The writing code uses the active drivers to raise the
// pin high, if you need power after the write (e.g. DS18S20 in
// parasite power mode) then set 'power' to 1, otherwise the pin will
// go tri-state at the end of the write to avoid heating in a short or
// other mishap.
//
// We're just going to write 1 bit at a time. Each poll call will write the next bit.
void PolledOneWire::polled_write(uint8_t v, uint8_t power /* = 0 */) {
	readWriteByte = v;
	writePower = power;
	poll_status |= ONEWIRE_POLLSTAT_WRITE;
	
    readWriteBitMask = 0x01;
	PolledOneWire::write_bit( (readWriteBitMask & readWriteByte)?1:0);
	readWriteBitMask <<= 1;
}

//
// Read a byte
//
void PolledOneWire::polled_read() {
    readWriteByte = 0;
	poll_status |= ONEWIRE_POLLSTAT_READ;
	readWriteBitMask = 0x01;
	if ( PolledOneWire::read_bit() ) 
		readWriteByte |= readWriteBitMask;
	readWriteBitMask <<= 1;
}

//
// Do a ROM skip
//
void PolledOneWire::polled_skip()
{
    polled_write(0xCC);           // Skip ROM
}

void PolledOneWire::polled_write_bytes(const uint8_t *buf, uint8_t count, bool power /* = 0 */) {
	byteCount = min(count, ONEWIRE_MAX_READ_WRITE_BUFFER_LEN); // We will truncate if this is exceeded.
	memcpy(readWriteBuffer, buf, byteCount);	
	byteIndex = 0;
	writeBytesPower = power;
	poll_status |= ONEWIRE_POLLSTAT_WRITE_BYTES;
	polled_write(readWriteBuffer[byteIndex]);
	byteIndex++;
}

void PolledOneWire::polled_read_bytes(uint8_t count) {
	byteCount = min(count, ONEWIRE_MAX_READ_WRITE_BUFFER_LEN); // We will truncate if this is exceeded.
	byteIndex = 0;
	poll_status |= ONEWIRE_POLLSTAT_READ_BYTES;
	polled_read();
}

//
// Do a ROM select
//
void PolledOneWire::polled_select( uint8_t rom[8])
{
    uint8_t tmp[9];
	tmp[0] = 0x55; // Choose ROM
	memcpy( tmp+1, rom, 8 );
	polled_write_bytes(tmp, 9);
}

void PolledOneWire::poll()
{
	IO_REG_TYPE mask = bitmask;
	volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;
	uint8_t r;
	
	// Note that a number of things may be happening at once. Start with lowest level
	// things first. Note also that if one low level thing completes, don't move on to
	// something higher level on this call. This will give the calling program time
	// to do other things.
	
	if ( poll_status & ONEWIRE_POLLSTAT_RESET ) {
		// We're in the middle of a Reset
		if ( bit_status == ONEWIRE_BITSTAT_RESET_WAIT_LINE_HIGH ) {
			if ( ! DIRECT_READ(reg, mask) ) {
				// Line still isn't high.
				if ( (long) ( micros() - bitNextTime ) >= 0 ) {
					// Unrecoverable error
					poll_status &= ~ONEWIRE_POLLSTAT_RESET;
					reset_result = false;
				}
			} else {
				// Line is high, continue
				noInterrupts();
				DIRECT_WRITE_LOW(reg, mask);
				DIRECT_MODE_OUTPUT(reg, mask);	// drive output low
				interrupts();
				bitNextTime = micros();
				bitNextTime += 500; // Line should stay low for 500 us
				bit_status = ONEWIRE_BITSTAT_RESET_WAIT_LOW;
			}
			return;
		}
		if ( bit_status == ONEWIRE_BITSTAT_RESET_WAIT_LOW ) {
			if ( (long) ( micros() - bitNextTime ) < 0 )
				return; // Not time yet
			noInterrupts();
			DIRECT_MODE_INPUT(reg, mask);	// allow it to float
			delayMicroseconds(80);
			r = !DIRECT_READ(reg, mask);
			interrupts();			
			reset_result = r;
			bitNextTime = micros();
			bitNextTime += 420; // Now wait 420 more us
			bit_status = ONEWIRE_BITSTAT_RESET_WAIT_FINISH;
			return;
		}
		if ( bit_status == ONEWIRE_BITSTAT_RESET_WAIT_FINISH ) {
			if ( (long) ( micros() - bitNextTime ) < 0 )
				return; // Not time yet	
			// We're done
			poll_status &= ~ONEWIRE_POLLSTAT_RESET;
		}
		return;
	}
	if ( poll_status & ONEWIRE_POLLSTAT_WRITE ) {
		PolledOneWire::write_bit( (readWriteBitMask & readWriteByte)?1:0);
		readWriteBitMask <<= 1;
		if (readWriteBitMask)
			return;
		// We're done!
		if ( !writePower) {
			noInterrupts();
			DIRECT_MODE_INPUT(baseReg, bitmask);
			DIRECT_WRITE_LOW(baseReg, bitmask);
			interrupts();		
		}
		poll_status &= ~ONEWIRE_POLLSTAT_WRITE;
		return;
	}
	if ( poll_status & ONEWIRE_POLLSTAT_READ ) {
		if ( PolledOneWire::read_bit() ) 
			readWriteByte |= readWriteBitMask;
		readWriteBitMask <<= 1;
		if (readWriteBitMask)
			return;
		// We're done!
		poll_status &= ~ONEWIRE_POLLSTAT_READ;
		return;
	}
	if ( poll_status & ONEWIRE_POLLSTAT_WRITE_BYTES) {
		polled_write(readWriteBuffer[byteIndex]);
		byteIndex++;
		if ( byteIndex < byteCount )
			return;
		// We're done!
		poll_status &= ~ONEWIRE_POLLSTAT_WRITE_BYTES;
		if (!writeBytesPower) {
			noInterrupts();
			DIRECT_MODE_INPUT(baseReg, bitmask);
			DIRECT_WRITE_LOW(baseReg, bitmask);
			interrupts();
		}
		return;
	}
	if ( poll_status & ONEWIRE_POLLSTAT_READ_BYTES ) {
		readWriteBuffer[byteIndex] = readWriteByte;
		byteIndex++;
		if (byteIndex == byteCount)
			// We're done!
			poll_status &= ~ONEWIRE_POLLSTAT_READ_BYTES;
		else
			// Get next byte
			polled_read();
		return;
	}
}


#endif
