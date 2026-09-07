#pragma once

// Non-blocking WS2812 light show. Call update() frequently while the show runs.
namespace lights {
void begin();      // init the strip — call once from setup()
void startShow();  // begin the rainbow animation
void update();      // render one frame; call every loop iteration
void off();         // clear the strip and stop
}  // namespace lights
