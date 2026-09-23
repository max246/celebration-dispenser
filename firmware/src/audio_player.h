#pragma once

// Plays audio from on-board flash (LittleFS) to the MAX98357A over I2S
// (ESP32-audioI2S). Two one-shot clips: an idle chime (the caller replays it on
// an interval) and a celebration sound that takes over and then hands back.
// Call update() every loop iteration.
namespace audioplayer {
void begin();                 // mount LittleFS + set up I2S — call once from setup()
void playIdle();              // play the idle chime once (interrupts celebration)
void playCelebration();       // play the celebration sound once (interrupts idle)
void update();                 // service the decoder + loop the idle file
bool isCelebrationPlaying();   // true only while the celebration sound is playing
}  // namespace audioplayer
