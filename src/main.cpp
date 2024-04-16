/*
 * Megaplexer - Seven Segment Multiplexing for the ATmega328P
 * a general purpose component of PrecisionClock-1
 * Copyright (C) 2024-04 Sidpatchy
 *
 * This code has been specifically designed to run on
 * the ATmega328P. It will likely work on other similar MCUs,
 * however, I have not tested this with anything else.
 *
 * Reference https://docs.arduino.cc/retired/hacking/hardware/PinMapping168/
 * for ATmega328P pinouts in relation to Arduino.
 *
 * ///////////////////////////////////////////////////////////////////////////
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <Arduino.h>
#include <Wire.h>
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                    BEGIN CONFIG                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * PWM pins should preferably be used for the common anode/cathode pins
 * of the seven segments for more easy control of brightness. This places
 * a practical limit on how many displays can be multiplexed, beyond just
 *
 * ATmega328P's PWM pins are pre-assigned.
 */

// Whether the displays used are common anode or cathode.
#define IS_COMMON_ANODE true

#define NUM_DIGITS 6

int commonPins[NUM_DIGITS] = {3, 5, 6, 9, 10, 11};
//                     COMMON 0, 1, 2, 3,  4,  5

/*
 * Pins for the individual segments of the displays.
 *
 * For segment name reference, see:
 * https://en.wikipedia.org/wiki/Seven-segment_display_character_representations#/media/File:7_segment_display_labeled.svg
 */
int segmentPins[8] = {0, 1, 2, 4, 7, 8, 12, 13};
//            SEGMENT A, B, C, D, E, F,  G, DP

/*
 * I2C config
 *
 * You MUST modify the I2C_ADDRESS if using more than one multiplexer
 * in order to avoid conflicts.
 */
#define I2C_ADDRESS 0x09

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                     END CONFIG                                                     //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Store which character should be displayed on each digit.
byte digitStates[6] = {0b01000000, 0b01000000, 0b01000000, 0b01000000, 0b01000000, 0b01000000};
                    //  DPgfedcba
/* I2C receive
 *
 * Byte 0: Digit index (0-5)
 * Byte 1: Character data, which segments to light up (i.e. 0-9, Aa-Zz, ",',=,-,_)
 *
 */
void receiveEvent(int numBytes) {
 while (Wire.available() > 1) {
  byte digitIndex = Wire.read();     // Which digit to modify
  byte segmentStates = Wire.read();  // Segment values

  if (digitIndex < 6) {
   digitStates[digitIndex] = segmentStates;
  }
 }
}

// I2C request, no use for this yet
void requestEvent() {
 // Prepare data to send (e.g., read from sensors or other sources)
 byte dataToSend = 42; // Example data
 Wire.write(dataToSend);
}

void updateDisplay(int digit) {
 // Turn off all common pins first to prevent ghosting
 for (int i = 0; i < NUM_DIGITS; i++) {
  digitalWrite(commonPins[i], IS_COMMON_ANODE);
 }

 // Turn on current digit
 digitalWrite(commonPins[digit], !IS_COMMON_ANODE);

 // Set segment pins
 for (int seg = 0; seg < 8; seg++) { // Includes DP
  bool segmentOn = digitStates[digit] & (1 << seg);
  digitalWrite(segmentPins[seg], IS_COMMON_ANODE ? !segmentOn : segmentOn);
 }
}


void setup() {
 // Init common pins
 for (int i = 0; i < NUM_DIGITS; i++) {
  pinMode(commonPins[i], OUTPUT);
  digitalWrite(commonPins[i], !IS_COMMON_ANODE); // Ensure all displays are off
 }

 // Init segment pins
 for (int i = 0; i < 8; i++) {
  pinMode(segmentPins[i], OUTPUT);
  digitalWrite(segmentPins[i], IS_COMMON_ANODE); // Ensure all segments are off
 }

 // Init I2C
 TWAR = I2C_ADDRESS;
 Wire.begin();
 Wire.setClock(400000);
 Wire.onReceive(receiveEvent); // enable receive event
 //Wire.onRequest(requestEvent); // enable request event
}

void loop() {
 static unsigned long lastUpdateTime = 0; // For timing display refresh
 if (millis() - lastUpdateTime > 2) { // Refresh display every 2 milliseconds
  for (int digit = 0; digit < NUM_DIGITS; digit++) {
   updateDisplay(digit); // Update display for each digit
   delayMicroseconds(2); // Delay between digits to reduce flickering
  }
  lastUpdateTime = millis();
 }
}