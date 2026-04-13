// Include the pitches file which defines note name constants (e.g. NOTE_C4 = 262Hz)
#include "pitches.h"

// ── PIN DEFINITIONS ──
// Piezo buzzer output pin
int buzzerPin = 8;
// Potentiometer analog input pin
int analogPin = A0;
// Ultrasonic sensor trigger pin (output — sends the sound pulse)
int distPin = 9;
// Ultrasonic sensor echo pin (input — listens for the returning pulse)
int echoPin = 10;
// Toggle switch pin (input — selects between the two sound modes)
int switchPin = 11;

// ── MUSICAL SCALE ──
// A two-octave pentatonic scale (C, D, E, G, A across octaves 4 and 5)
int scale[] = {NOTE_C4, NOTE_D4, NOTE_E4, NOTE_G4, NOTE_A4,
               NOTE_C5, NOTE_D5, NOTE_E5, NOTE_G5, NOTE_A5};
int numNotes = 10;

// ── TIMING VARIABLES ──
// unsigned long is required here because millis() counts up very fast.
// A standard int overflows at 32,767 which would be reached in about 32 seconds, breaking the timer. unsigned long holds up to ~4.2 billion.
unsigned long startMillis = 0;

// Counts how many beats have played — used to alternate between kick and snare in the drum mode
int beatStep = 0;

// ── SETUP ──
void setup() {
  // Set buzzer as output so the Arduino can send PWM signals to it
  pinMode(buzzerPin, OUTPUT);
  // Set trigger pin as output so the Arduino can fire the ultrasonic pulse
  pinMode(distPin, OUTPUT);
  // Set echo pin as input so the Arduino can listen for the returning pulse
  pinMode(echoPin, INPUT);
  // INPUT_PULLUP activates the Arduino's internal resistor so no external resistor is needed on the breadboard. 
  // The pin reads HIGH normally and LOW when the switch connects it to ground.
  pinMode(switchPin, INPUT_PULLUP);
}

// ── MAIN LOOP ──
void loop() {

  // long is used for duration and cm because the raw microsecond values returned by pulseIn() can exceed the int limit of 32,767
  long duration, cm;

  // Read the toggle switch — HIGH means switch is open, LOW means closed
  int switchState = digitalRead(switchPin);

  // ── ULTRASONIC SENSOR TRIGGER SEQUENCE ──
  // Pull the trigger pin LOW briefly to ensure a clean starting state
  digitalWrite(distPin, LOW);
  delayMicroseconds(2);
  // Send a 10-microsecond HIGH pulse to fire the ultrasonic burst
  digitalWrite(distPin, HIGH);
  delayMicroseconds(10);
  // Pull LOW again — the sensor is now listening for the echo
  digitalWrite(distPin, LOW);

  // Measure how long the echo takes to return, in microseconds.
  // The 10000 timeout prevents the program from freezing if no object is close enough to reflect the pulse — it returns 0 instead.
  duration = pulseIn(echoPin, HIGH, 10000);

  // Convert the raw time into a distance in centimetres
  cm = microsecondsToCentimeters(duration);

  // ── DISTANCE CLAMPING ──
  // If the sensor timed out (cm = 0) or the object is further than 50cm, treat the distance as 50cm. 
  // This limits the performance zone to a comfortable arm's-length range above the breadboard.
  if (cm == 0 || cm > 50) { cm = 50; }

  // ── MAP DISTANCE TO MUSICAL SCALE ──
  // Map the distance (starting from 2cm — the sensor's reliable minimum) to an index between 0 and 9 to select a note from the scale array.
  // 2cm = lowest note (NOTE_C4), 50cm = highest note (NOTE_A5)
  int noteIndex = map(cm, 2, 50, 0, numNotes - 1);
  // Look up the actual frequency for that index in the scale array
  int currentNote = scale[noteIndex];

  // ── TEMPO FROM POTENTIOMETER ──
  // Read the potentiometer (0–1023) and map it to a beat interval.
  // 150ms = fast beat, 800ms = slow beat
  int analogValue = analogRead(analogPin);
  int tempo = map(analogValue, 0, 1023, 150, 800);

  // ── NON-BLOCKING BEAT TIMER ──
  // Using millis() instead of delay() means the Arduino keeps reading the sensors between beats, making the instrument feel responsive.
  // If delay() were used here, the Arduino would go blind to sensor changes while waiting for the next beat.
  unsigned long currentMillis = millis();

  // Check if enough time has passed since the last beat
  if (currentMillis - startMillis >= tempo) {
    // Reset the timer so the countdown starts again from now
    startMillis = currentMillis;
    // Advance the beat counter — used below to alternate drum sounds
    beatStep++;

    // ── SOUND MODE SELECTION ──
    if (switchState == HIGH) {
      // MODE A — Melodic mode
      // Every beat plays the note determined by the player's hand distance.
      // Moving the hand closer plays a lower note, further plays higher.
      tone(buzzerPin, currentNote, 50);

    } else {
      // MODE B — Drum mode
      // Beats alternate between a deep kick tone and a lighter snare tone.
      // The modulo operator (%) gives the remainder of dividing beatStep by 2.
      // Even beats (remainder = 0) play a fixed deep 100Hz kick drum.
      // Odd beats play the hand-controlled note as a snare.
      if (beatStep % 2 == 0) {
        tone(buzzerPin, 100, 50);
      } else {
        tone(buzzerPin, currentNote, 50);
      }
    }
  }
}

// ── HELPER FUNCTION ──
// Converts the raw echo time (microseconds) into centimetres.
// Sound travels at 340 m/s, or 29 microseconds per centimetre.
// The ping travels out and back, so we divide by 2 to get one-way distance.
// A duration of 10000 microseconds (the timeout value) returns 0cm.
long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2;
}