#pragma once

// Non-blocking WS2812 light show. Call update() frequently while a show runs.
namespace lights {
void begin();      // init the strip — call once from setup()
void startShow();  // begin the rainbow animation
void startEyes();  // flash the orange eyes + white section (the celebration look)
void startIdleGlow();  // pulsing yellow on the FLASH section (while the idle chime plays)
void update();      // render one frame; call every loop iteration
void off();         // clear the strip and stop
}  // namespace lights
