// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//


// Convert the duration into a readable string
void secondsToEnglishString(char *str, uint32_t seconds)
{
  boolean hasHours = false;
  // Take care of hours
  if (seconds >= 3600) {
    sprintf(str, "%ld hour%c", seconds/3600, seconds>=7200? 's':0);
    seconds = seconds % 3600;
    hasHours = true;
    str += strlen(str);
  }

  // Take care of minutes
  seconds = seconds / 60;
  if (seconds) {
    if (hasHours)
      strcpy(str++, " ");
    sprintf(str, "%ld minutes", seconds);
  }
}


// Display seconds in the format hhh:mm:ss
char *secondsInClockFormat(char *str, uint32_t seconds)
{
  uint16_t hours = seconds/3600;
  uint16_t minutes = (seconds % 3600) / 60;
  seconds = seconds % 60;
  if (hours)
    sprintf(str, "%d:%02d:%02ld", hours, minutes, seconds);
  else
    sprintf(str, "%d:%02ld", minutes,seconds);
  return str;
}


// Animate the heating element and fan icons, based on their current state
// This function is called 50 times per second
void animateIcons(uint16_t x)
{
  static uint16_t lastBitmap[NUMBER_OF_OUTPUTS];
  static uint8_t animateFan = 0;
  static uint8_t animationPhase = 0;
  static uint32_t lastUpdate = 0;
  uint16_t bitmap;

  // If it has been a while since the last update then redraw all the icons
  // This can happen if the user finishes a bake, then starts another one.
  uint32_t now = millis();
  if (now - lastUpdate > 100) {
    for (int8_t i=0; i < NUMBER_OF_OUTPUTS; i++)
      lastBitmap[i] = 0;
    lastUpdate = now;
  }
  
  for (int8_t i=0; i < NUMBER_OF_OUTPUTS; i++) {
    // Don't display anything if the output is unused
    if (prefs.outputType[i] == TYPE_UNUSED)
      continue;
    // Handle heating elements first
    if (isHeatingElement(prefs.outputType[i])) {
      // Is the element on?
      if (getOutput(i) == HIGH)
        bitmap = BITMAP_ELEMENT_ON;
      else
        bitmap = BITMAP_ELEMENT_OFF;
    }
    else {
      // Output must be a fan
      if (prefs.outputType[i] == TYPE_CONVECTION_FAN)
        bitmap = BITMAP_CONVECTION_FAN1;
      else
        bitmap = BITMAP_COOLING_FAN1;

      // Should the fan be animated?
      if (getOutput(i) == HIGH)
        bitmap += animationPhase;
    }

    // Draw the bitmap
    if (bitmap != lastBitmap[i]) {
      renderBitmap(bitmap, x, 4);
      lastBitmap[i] = bitmap;
    }
    x +=40;
  }
  // Go to the next animation image the next time this function is called
  if (++animateFan == 3) {
    animationPhase = (animationPhase + 1) % 3;
    animateFan = 0;
  }
}

