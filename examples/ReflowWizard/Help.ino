// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//
// Shows context-sensitive help for the various screens

static uint16_t nextHelpLineY, nextHelpLineX;

#define INTER_LINE_SPACING  30
#define BOTTOM_OF_HELP_BOX  310
#define HELP_BOX_HEIGHT(lines)  (18 + (INTER_LINE_SPACING * lines))

void showHelp(uint8_t screen)
{
  switch(screen) {
    case SCREEN_SETTINGS:
      drawHelpBorder(435, HELP_BOX_HEIGHT(6));
      displayHelpLine((char *) "Test - Test outputs (relays)");
      displayHelpLine((char *) "Learning - Learning info");
      displayHelpLine((char *) "Reset - Touch and factory");
      displayHelpLine((char *) "Setup - Configure oven");
      displayHelpLine((char *) "Stats - A bunch of numbers");
      displayHelpLine((char *) "About - Version information");
      getTap(SHOW_TEMPERATURE_IN_HEADER);
      // Clear the area used by Help.  The screen will need to be redrawn
      eraseHelpScreen(435, HELP_BOX_HEIGHT(6));
      break;
      
    case SCREEN_TEST:
      drawHelpBorder(440, HELP_BOX_HEIGHT(7));
      displayHelpLine((char *) "There are 6 outputs that can");
      displayHelpLine((char *) "be used to control relays that");
      displayHelpLine((char *) "are connected to fans or heating");
      displayHelpLine((char *) "elements.  Here you can turn"); 
      displayHelpLine((char *) "outputs on and off individually to");
      displayHelpLine((char *) "ensure they have been wired");
      displayHelpLine((char *) "correctly and are working.");
      getTap(SHOW_TEMPERATURE_IN_HEADER);
      // Clear the area used by Help.  The screen will need to be redrawn
      eraseHelpScreen(440, HELP_BOX_HEIGHT(7));
      break;

    case SCREEN_SETUP_OUTPUTS:
      drawHelpBorder(430, HELP_BOX_HEIGHT(6));
      displayHelpLine((char *) "Configure the outputs to ");
      displayHelpLine((char *) "correspond to how you've"); 
      displayHelpLine((char *) "wired your oven.  These settings"); 
      displayHelpLine((char *) "are used to determine which");
      displayHelpLine((char *) "outputs (or relays) are turned on"); 
      displayHelpLine((char *) "and off during a bake or reflow.");
      getTap(SHOW_TEMPERATURE_IN_HEADER);
      // Clear the area used by Help.  The screen will need to be redrawn
      eraseHelpScreen(430, HELP_BOX_HEIGHT(6));
      break;
      
    case SCREEN_SERVO_OPEN:
      drawHelpBorder(445, HELP_BOX_HEIGHT(7));
      displayHelpLine((char *) "You can use a servo to open");
      displayHelpLine((char *) "and close the oven door"); 
      displayHelpLine((char *) "automatically. The door is opened");
      displayHelpLine((char *) "when cooling is needed and closed"); 
      displayHelpLine((char *) "again once the oven is safe to"); 
      displayHelpLine((char *) "handle.  If movement is jerky you");
      displayHelpLine((char *) "need a better 5V power supply.");
      getTap(SHOW_TEMPERATURE_IN_HEADER);
      // Clear the area used by Help.  The screen will need to be redrawn
      eraseHelpScreen(445, HELP_BOX_HEIGHT(7));
      break;
      
    case SCREEN_LINE_FREQUENCY:
      drawHelpBorder(445, HELP_BOX_HEIGHT(6));
      displayHelpLine((char *) "The MAX31856 thermocouple");
      displayHelpLine((char *) "IC has 50Hz/60Hz noise"); 
      displayHelpLine((char *) "rejection filtering to improve");
      displayHelpLine((char *) "the accuracy of the temperature"); 
      displayHelpLine((char *) "readings.  It filters noise introduced"); 
      displayHelpLine((char *) "by the 110V/230V power lines.");
      getTap(SHOW_TEMPERATURE_IN_HEADER);
      // Clear the area used by Help.  The screen will need to be redrawn
      eraseHelpScreen(445, HELP_BOX_HEIGHT(6));
      break;

    case SCREEN_RESET:
      drawHelpBorder(445, HELP_BOX_HEIGHT(6));
      displayHelpLine((char *) "A factory reset will erase all");
      displayHelpLine((char *) "settings, and return them to"); 
      displayHelpLine((char *) "their default values.  Stored");
      displayHelpLine((char *) "reflow profiles are also erased.");
      displayHelpLine((char *) "You can recalibrate the touchscreen"); 
      displayHelpLine((char *) "if you feel it isn't accurate."); 
      getTap(SHOW_TEMPERATURE_IN_HEADER);
      // Clear the area used by Help.  The screen will need to be redrawn
      eraseHelpScreen(445, HELP_BOX_HEIGHT(6));
      break;

    case SCREEN_STATS:
      drawHelpBorder(445, HELP_BOX_HEIGHT(4));
      displayHelpLine((char *) "Dude, it's just numbers!");
      displayHelpLine((char *) "Nothing more, nothing less ..."); 
      displayHelpLine((char *) "The kind of help you're looking for"); 
      displayHelpLine((char *) "you ain't going to find here.  Sorry!"); 
      getTap(SHOW_TEMPERATURE_IN_HEADER);
      // Clear the area used by Help.  The screen will need to be redrawn
      eraseHelpScreen(445, HELP_BOX_HEIGHT(5));
      break;


    case SCREEN_ABOUT:
      drawHelpBorder(445, HELP_BOX_HEIGHT(7));
      displayHelpLine((char *) "To take a screenshot just tap");
      displayHelpLine((char *) "3 times in the top left corner"); 
      displayHelpLine((char *) "of the screen.  The data is not");
      displayHelpLine((char *) "buffered so it may take 10 seconds"); 
      displayHelpLine((char *) "to write the bitmap to the SD card."); 
      displayHelpLine((char *) "The software freezes while taking a"); 
      displayHelpLine((char *) "screenshot so be careful using it!"); 
      getTap(DONT_SHOW_TEMPERATURE);
      // Clear the area used by Help.  The screen will need to be redrawn
      eraseHelpScreen(445, HELP_BOX_HEIGHT(7));
      break;

    case SCREEN_BAKE:
    case SCREEN_EDIT_BAKE1:
      drawHelpBorder(460, HELP_BOX_HEIGHT(6));
      displayHelpLine((char *) "Some components are sensitive");
      displayHelpLine((char *) "to moisture so it may be"); 
      displayHelpLine((char *) "necessary to bake them before");
      displayHelpLine((char *) "reflowing them.");
      displayHelpLine((char *) "You can also bake other non-food"); 
      displayHelpLine((char *) "items in the oven."); 
      getTap(SHOW_TEMPERATURE_IN_HEADER);
      // Clear the area used by Help.  The screen will need to be redrawn
      eraseHelpScreen(460, HELP_BOX_HEIGHT(6));
      break;

    case SCREEN_EDIT_BAKE2:
      drawHelpBorder(450, HELP_BOX_HEIGHT(7));
      displayHelpLine((char *) "Once baking is done, the servo");
      displayHelpLine((char *) "can open the oven door to");
      displayHelpLine((char *) "accelerate cooling. If you have"); 
      displayHelpLine((char *) "cooling fan this can also be turned"); 
      displayHelpLine((char *) "on at the same time. The fan will"); 
      displayHelpLine((char *) "only be turned on if the oven door"); 
      displayHelpLine((char *) "is open."); 
      getTap(SHOW_TEMPERATURE_IN_HEADER);
      // Clear the area used by Help.  The screen will need to be redrawn
      eraseHelpScreen(450, HELP_BOX_HEIGHT(7));
      break;

    case HELP_OUTPUTS_NOT_CONFIGURED:
      drawHelpBorder(430, HELP_BOX_HEIGHT(5));
      displayHelpLine((char *) "The outputs have not been");
      displayHelpLine((char *) "configured.  Please go to"); 
      displayHelpLine((char *) "Settings and configure which");
      displayHelpLine((char *) "outputs control which heating");
      displayHelpLine((char *) "elements."); 
      getTap(SHOW_TEMPERATURE_IN_HEADER);
      break;

    case SCREEN_LEARNING:
      drawHelpBorder(440, HELP_BOX_HEIGHT(8));
      displayHelpLine((char *) "The elements are measured");
      displayHelpLine((char *) "to see how quickly they can"); 
      displayHelpLine((char *) "heat up the oven.  Thermal lag");
      displayHelpLine((char *) "is also measured to determine how");
      displayHelpLine((char *) "quickly the element reaches the");
      displayHelpLine((char *) "desired temperature, and how"); 
      displayHelpLine((char *) "slowly does it cools down.  Oven"); 
      displayHelpLine((char *) "insulation is also measured."); 
      getTap(SHOW_TEMPERATURE_IN_HEADER);
      // Clear the area used by Help.  The screen will need to be redrawn
      eraseHelpScreen(440, HELP_BOX_HEIGHT(8));
      break;

    case HELP_LEARNING_NOT_DONE:
      drawHelpBorder(430, HELP_BOX_HEIGHT(4));
      displayHelpLine((char *) "The oven has not been");
      displayHelpLine((char *) "through the learning cycle."); 
      displayHelpLine((char *) "Please go to Settings and tap");
      displayHelpLine((char *) "the \"Learning\" button.");
      getTap(SHOW_TEMPERATURE_IN_HEADER);
      break;
    
    case SCREEN_RESULTS:
      drawHelpBorder(440, HELP_BOX_HEIGHT(8));
      displayHelpLine((char *) "Power: The duty cycle of");
      displayHelpLine((char *) " elements needed to maintain");
      displayHelpLine((char *) " temperature (lower is better)."); 
      displayHelpLine((char *) "Inertia: Indicates how quickly the");
      displayHelpLine((char *) " oven temperature can be changed");
      displayHelpLine((char *) " (lower is better)."); 
      displayHelpLine((char *) "Insulation: Time to cool 30~C"); 
      displayHelpLine((char *) " (higher is better)."); 
      getTap(SHOW_TEMPERATURE_IN_HEADER);
      // Clear the area used by Help.  The screen will need to be redrawn
      eraseHelpScreen(440, HELP_BOX_HEIGHT(8));
      break;
  }
}


void drawHelpBorder(uint16_t width, uint16_t height)
{
  uint16_t x = (479 - width) >> 1;
  uint16_t top = BOTTOM_OF_HELP_BOX - height;
  // Draw border and fill in the middle
  tft.drawRect(x, top, width, height, 0xD800);
  tft.drawRect(x+1, top+1, width-2, height-2, 0xF800);
  tft.drawRect(x+2, top+2, width-4, height-4, 0xFA00);
  tft.drawRect(x+3, top+3, width-6, height-6, 0xFC00);
  tft.drawRect(x+4, top+4, width-8, height-8, 0xF800);
  tft.drawRect(x+5, top+5, width-10, height-10, BLACK);
  tft.fillRect(x+6, top+6, width-12, height-12, WHITE);
  
  // Add the small help icon
  renderBitmap(BITMAP_HELP_ICON, x+width-81, top+6);
  
  // Clear the existing touch targets and add one that takes up the entire screen, less the
  // title area which is needed to be able to screenshots.
  clearTouchTargets();
  defineTouchArea(0, 45, 479, 319);

  // Define where the first line of text appears
  nextHelpLineY = top + 13;
  nextHelpLineX = x + 13;
}


void eraseHelpScreen(uint16_t width, uint16_t height)
{
  uint16_t x = (479 - width) >> 1;
  uint16_t top = BOTTOM_OF_HELP_BOX - height;
  tft.fillRect(x, top, width, height, WHITE);
}


void displayHelpLine(char *text)
{
  displayString(nextHelpLineX, nextHelpLineY, FONT_9PT_BLACK_ON_WHITE, text);
  nextHelpLineY += INTER_LINE_SPACING;
}

