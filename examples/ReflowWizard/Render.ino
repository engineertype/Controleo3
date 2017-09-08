// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//
// Some fonts are fixed width.  This is so that when updating a field (like temperature), you can just overwrite
// the previous number instead of erasing the number and then redrawing it.


// Render a bitmap to the screen
// All the bitmaps exist in external flash, but some are duplicated in microcontroller flash.
// Reading from microcontroller flash is 20 times faster than external flash, so it makes sense
// to keep some of the most used bitmaps there.
uint16_t renderBitmap(uint16_t bitmapNumber, uint16_t x, uint16_t y) {
  // Render from microcontroller flash, if the bitmap exists there
  if (flashBitmaps[bitmapNumber])
    return renderBitmapFromMicrocontrollerFlash(bitmapNumber, x, y);
  return renderBitmapFromExternalFlash(bitmapNumber, x, y);
}


// Render a bitmap from external flash
// Returns the width of the rendered bitmap (needed when writing text)
uint16_t renderBitmapFromExternalFlash(uint16_t bitmapNumber, uint16_t x, uint16_t y)
{
    uint16_t bitmapHeight, bitmapWidth, pageWhereBitmapIsStored, pixelsInPage;
    uint32_t bitmapPixels;
    uint16_t buf[128];    // 256 bytes

    // Make sure this is a valid bitmap
    if (bitmapNumber > BITMAP_LAST_ONE) {
      SerialUSB.println("RenderBitmap: bitmap number if not valid");
      return 0;
    }

    // Get the flash page where this bitmap is stored 
    pageWhereBitmapIsStored = flash.getBitmapInfo(bitmapNumber, &bitmapWidth, &bitmapHeight);
    if (pageWhereBitmapIsStored > 0xFFF) {
      SerialUSB.println("RenderBitmap: pageWhereBitmapIsStored is too big");
      return 0;
    }

    if (0 && bitmapNumber >= BITMAP_LEFT_ARROW)
      SerialUSB.println("N=" + String(bitmapNumber) + " H=" + String(bitmapHeight) + " W=" + String(bitmapWidth) + " Center=" + String((480 - bitmapWidth) >> 1));

    // Calculate the number of pixels that need to be rendered
    bitmapPixels = bitmapWidth * bitmapHeight;

    // How many pixels are in the first flash page? (pixels are 2 bytes)
    pixelsInPage = bitmapPixels > 128? 128 : bitmapPixels;

    // Start the flash read.  
    flash.startRead(pageWhereBitmapIsStored, pixelsInPage << 1, (uint8_t *) buf);

    // Start rendering the bitmap
    tft.startBitmap(x, y, bitmapWidth, bitmapHeight);

    while (bitmapPixels) {
       tft.drawBitmap((uint16_t *) buf, pixelsInPage);
       bitmapPixels -= pixelsInPage;
       // Read the next page of the bitmap
       if (bitmapPixels) {
          pixelsInPage = bitmapPixels > 128? 128 : bitmapPixels;
          flash.continueRead(pixelsInPage << 1, (uint8_t *) buf);
       }
    }
    tft.endBitmap();
    flash.endRead();
    return bitmapWidth;
}


// Render a bitmap from microcontroller flash
// Returns the width of the rendered bitmap (needed when writing text)
// The first 2 bytes of the bitmap is the width and height of the bitmap
uint16_t renderBitmapFromMicrocontrollerFlash(uint16_t bitmapNumber, uint16_t x, uint16_t y)
{
    uint16_t bitmapHeight, bitmapWidth;
    uint32_t bitmapPixels;
    char *fontBitmap;
    
    // Get a pointer to the bitmap from the bitmap table
    fontBitmap = (char *) flashBitmaps[bitmapNumber];

    // The bitmap width and height are the first 2 bytes
    bitmapHeight = *(fontBitmap++);
    bitmapWidth = *(fontBitmap++);
    bitmapPixels = bitmapWidth * bitmapHeight;
    if (0 && bitmapNumber >= FONT_IMAGES && bitmapNumber < BITMAP_CONVECTION_FAN1 && bitmapNumber > BITMAP_COOLING_FAN3)
      SerialUSB.println("N=" + String(bitmapNumber) + " H=" + String(bitmapHeight) + " W=" + String(bitmapWidth) + " Center=" + String((480 - bitmapWidth) >> 1));

    // Start rendering the bitmap
    tft.startBitmap(x, y, bitmapWidth, bitmapHeight);
    tft.drawBitmap((uint16_t *) fontBitmap, bitmapPixels);
    tft.endBitmap();
    return bitmapWidth;
}


// Display a string on the screen, using the specified font
// Only ASCII-printable character are supported
uint16_t displayString(uint16_t x, uint16_t y, uint8_t font, char *str) {
  boolean firstChar = true, printData = true;
  uint16_t start = x;
  if (true /* strstr(str, "~") > 0 || strstr(str, ":") > 0 */)
    printData = false;
  else {
    SerialUSB.print("Width of [");
    SerialUSB.print(str);
    SerialUSB.print("] is ");
  }
  while (*str != 0) {
    if (!firstChar)
      x += preCharacterSpace(font, *str);
    firstChar = false;
    x += displayCharacter(font, x, y, *str);
    x += postCharacterSpace(font, *str++);
  }
  if (printData) {
    SerialUSB.print(x - start - postCharacterSpace(font, *(str-1)));
    SerialUSB.print(". Center = ");
    SerialUSB.println((480 - x + start + postCharacterSpace(font, *(str-1))) / 2);
  }
  return x - start - postCharacterSpace(font, *(str-1));
}


// Display a character on the screen, using the specified font
uint16_t displayCharacter(uint8_t font, uint16_t x, uint16_t y, uint8_t c)
{
  uint16_t bitmapNumber = 0;

  // Make sure the character can be printed
  if (!isSupportedCharacter(font, c)) {
    SerialUSB.println("displayCharacter: Not supported");
    return 0;
  }
    
  // Special case for space
  if (c == ' ') {
    // Don't display anything.  Just return the width of the space character
    if (font == FONT_9PT_BLACK_ON_WHITE || font == FONT_9PT_BLACK_ON_WHITE_FIXED)
      return 10;
    if (font == FONT_12PT_BLACK_ON_WHITE || font == FONT_12PT_BLACK_ON_WHITE_FIXED)
      return 16;
    // Must be the 22-point font
    return 22;
  }

  // Get the bitmap number
  switch (font) {
    case FONT_9PT_BLACK_ON_WHITE:
      bitmapNumber = FONT_FIRST_9PT_BW + c - 33;
      break;

    case FONT_12PT_BLACK_ON_WHITE:
      bitmapNumber = FONT_FIRST_12PT_BW + c - 33;
      break;
      
    case FONT_9PT_BLACK_ON_WHITE_FIXED:
      // Only '0' through '9' have fixed width
      if (c >= '0' && c <= '9')
        bitmapNumber = FONT_FIRST_9PT_BW_FIXED + c - '0';
      else if (c == 'F')
        bitmapNumber = FONT_FIRST_9PT_BW_FIXED + 10;
      else {
        bitmapNumber = FONT_FIRST_9PT_BW + c - 33;
        font = FONT_9PT_BLACK_ON_WHITE;
      }
      break;

    case FONT_12PT_BLACK_ON_WHITE_FIXED:
      // Only '0' through '9' have fixed width
      if (c >= '0' && c <= '9')
        bitmapNumber = FONT_FIRST_12PT_BW_FIXED + c - '0';
      else if (c == 'F')
        bitmapNumber = FONT_FIRST_12PT_BW_FIXED + 10;
      else if (c == 'C')
        bitmapNumber = FONT_FIRST_12PT_BW_FIXED + 11;
      else {
        bitmapNumber = FONT_FIRST_12PT_BW + c - 33;
        font = FONT_12PT_BLACK_ON_WHITE;
      }
      break;

    case FONT_22PT_BLACK_ON_WHITE_FIXED:
      switch (c) {
        case '~' : bitmapNumber = FONT_FIRST_22PT_BW_FIXED + 15; break;
        case '%' : bitmapNumber = FONT_FIRST_22PT_BW_FIXED + 14; break;
        case '.' : bitmapNumber = FONT_FIRST_22PT_BW_FIXED + 13; break;
        case ':' : bitmapNumber = FONT_FIRST_22PT_BW_FIXED + 12; break;
        case 'C' : bitmapNumber = FONT_FIRST_22PT_BW_FIXED + 11; break;
        case 'F' : bitmapNumber = FONT_FIRST_22PT_BW_FIXED + 10; break;
        default  : bitmapNumber = FONT_FIRST_22PT_BW_FIXED + c - '0';
      }
      break;

  }

  // Display the character
  return renderBitmap(bitmapNumber, x, y + getYOffsetForCharacter(font, c));
}


// The whitespace around each character is not stored as part of the bitmap, to make
// rendering as fast as possible.  The result is that all characters are not at the same
// height so an offset is necessary.
int16_t getYOffsetForCharacter(uint8_t font, uint8_t c) {
  if (font == FONT_9PT_BLACK_ON_WHITE) {
    if (c == '$')
      return -1;
    if (c == '<' || c == '>')
      return 3;
    if (c == '+')
      return 4;
    if (c == '=')
      return 5;
    if (c == ',' || c == '.')
      return 17;
    if (c == '-')
      return 11;
    if (c == ':' || c == ';' || c == 'a' || c == 'c' || c == 'e' || c == 'g' || (c >= 'm' && c <= 's') || (c >= 'u' && c <= 'z'))
      return 5;
    if (c == '_')
      return 22;
    return 0;
  }
  
  if (font == FONT_12PT_BLACK_ON_WHITE) {
    if (c == '$')
      return -1;
    if (c == '+' || c == '<' || c == '>')
      return 3;
    if (c == '=')
      return 7;
    if (c == ',' || c == '.')
      return 20;
    if (c == '-')
      return 14;
    if (c == ':' || c == ';' || c == 'a' || c == 'c' || c == 'e' || c == 'g' || (c >= 'm' && c <= 's') || (c >= 'u' && c <= 'z'))
      return 6;
    if (c == '_')
      return 28;
    return 0;
  }

  // 22-point numbers have fixed height.  Other characters vary in height
  if (font == FONT_22PT_BLACK_ON_WHITE_FIXED) {
    if (c == ':')
      return 8;
    if (c == '.')
      return 41;
    if (c == 'z')
      return 13;
    return 0;
  }

  // Other fixed width fonts also have the same height
  if (font == FONT_9PT_BLACK_ON_WHITE_FIXED || font == FONT_12PT_BLACK_ON_WHITE_FIXED)
    return 0;

  return 0;
}


// Determine if the character is in the font set
// 9 and 12-point fonts contain all printable characters
// The 22-point font contains a small subset
boolean isSupportedCharacter(uint8_t font, uint8_t c)
{
  // Font must be supported
  if (font > FONT_22PT_BLACK_ON_WHITE_FIXED)
    return false;
    
  // Character must be printable
  if (c < 32 || c > 126)
    return false;
    
  // The 22-point fonts only support 0,1,2,3,4,5,6,7,8,9,:,.,~,C,H,z and %
  if (font == FONT_22PT_BLACK_ON_WHITE_FIXED) {
    if (c >= '0' && c <= '9')
      return true;
    if (c == ':' || c =='.' || c == '~' || c == 'C' || c == 'H' || c == 'z' || c == '%')
      return true;
    return false;
  }

  // 9-point and 12-point fonts support all printable ASCII characters
  return true;
}


// To make the text more readable (and best mimic computer fonts), it is sometimes good to have some space before or after a character.
// There is a whole science behind this called "kerning".  For example, if you have "AV" the letter 'V' actually overlaps the width
// allocated to 'A'.  Some fonts have up to 4000 "kerning pairs" but this is overkill for what we're trying to do here.
// This very simple method works sufficiently well.
                                     //  ! " # $ % & ' ( ) * + , - . / 0 1 2 3 4 5 6 7 8 9 : ; < = > ? @ A B C D E F G H I J K L M N
const uint8_t charsNeedingPreSpace[]  = {1,1,0,0,0,0,1,1,0,1,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,1,0,1,1,1,0,1,1,0,1,1,1,1,
                                     //  O P Q R S T U V W X Y Z [ \ ] ^ _ ' a b c d e f g h i j k l m n o p q r s t u v w x y z { | } ~
                                         0,1,0,1,0,0,1,0,0,0,0,0,1,0,0,1,0,1,0,1,0,0,0,0,0,1,1,0,1,1,1,1,1,1,0,1,0,0,1,0,0,0,0,0,1,1,0,0};
                                     //  ! " # $ % & ' ( ) * + , - . / 0 1 2 3 4 5 6 7 8 9 : ; < = > ? @ A B C D E F G H I J K L M N
const uint8_t charsNeedingPostSpace[] = {1,0,0,0,0,0,1,0,1,1,1,1,1,1,0,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0,1,1,1,0,1,2,1,0,0,1,1,
                                     //  O P Q R S T U V W X Y Z [ \ ] ^ _ ' a b c d e f g h i j k l m n o p q r s t u v w x y z { | } ~
                                         0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,1,0,1,0,0,0,1,0,0,0,1,1,1,0,1,1,1,0,0,1,0,0,0,1,0,0,0,0,0,0,1,1,0};


uint16_t preCharacterSpace(uint8_t font, char c) 
{
  // Fixed fonts don't get any pre-character spacing
  if (font == FONT_9PT_BLACK_ON_WHITE_FIXED || font == FONT_12PT_BLACK_ON_WHITE_FIXED)
    return 0;
  if (font == FONT_22PT_BLACK_ON_WHITE_FIXED) {
    if (c == ':')
      return 4;
    return 0;
  }
  // Make sure the character is valid
  if (c < '!' || c > '~')
    return 0;
  if (font == FONT_9PT_BLACK_ON_WHITE)
    return charsNeedingPreSpace[c - '!'];
  return 2 * (charsNeedingPreSpace[c - '!']);
}


uint16_t postCharacterSpace(uint8_t font, char c) 
{
  // Fixed fonts don't get any post-character spacing
  if (font == FONT_9PT_BLACK_ON_WHITE_FIXED || font == FONT_12PT_BLACK_ON_WHITE_FIXED)
    return 0;
  if (font == FONT_22PT_BLACK_ON_WHITE_FIXED) {
    if (c == ':')
      return 4;
    return 0;
  }
  // Make sure the character is valid
  if (c < '!' || c > '~')
    return 0;
  if (font == FONT_9PT_BLACK_ON_WHITE)
    return charsNeedingPostSpace[c - '!'];
  return 1 + (2 * (charsNeedingPostSpace[c - '!']));
}


// Display a fixed-width string (typically a number that keeps getting updated)
void displayFixedWidthString(uint16_t x, uint16_t y, char *str, uint8_t maxChars, uint8_t font)
{
  uint8_t numberOfCharacters = strlen(str);

  // Sanity check
  if (numberOfCharacters > maxChars) {
    SerialUSB.print("displayFixedWidthString: too many characters in string ");
    SerialUSB.println(str);
    return;
  }
  // What is the width of each character?
  uint16_t characterWidth = font == FONT_9PT_BLACK_ON_WHITE_FIXED? 15: font == FONT_12PT_BLACK_ON_WHITE_FIXED? 19: 37;
  
  // How much space should there be before writing the string?
  uint16_t spacePadding = (maxChars - numberOfCharacters) * characterWidth;

  // Write the space padding, if necessary
  if (spacePadding) {
    uint16_t characterHeight = font == FONT_9PT_BLACK_ON_WHITE_FIXED? 19: font == FONT_12PT_BLACK_ON_WHITE_FIXED? 25: 48;
    tft.fillRect(x, y, spacePadding, characterHeight, WHITE);
  }

  // Write the rest of the string now
  displayString(x + spacePadding, y, font, str);
}


// Draw a button on the screen
void drawButton(uint16_t x, uint16_t y, uint16_t width, uint16_t textWidth, boolean useLargeFont, char *text) {
  drawButtonOutline(x, y, width);
  if (useLargeFont)
    displayString(x + (width >> 1) - (textWidth >> 1), y+18, FONT_12PT_BLACK_ON_WHITE, text);
  else
    displayString(x + (width >> 1) - (textWidth >> 1), y+21, FONT_9PT_BLACK_ON_WHITE, text);
}


void drawButtonOutline(uint16_t x, uint16_t y, uint16_t width) {
  uint16_t lineX = x + 15, lineWidth = width - 30;
  renderBitmap(BITMAP_LEFT_BUTTON_BORDER, x, y);
  renderBitmap(BITMAP_RIGHT_BUTTON_BORDER, x + width - 15, y);

  tft.drawFastHLine(lineX, y, lineWidth, 0xEF5F);
  tft.drawFastHLine(lineX, y+1, lineWidth, 0xC61F);
  tft.drawFastHLine(lineX, y+2, lineWidth, 0xE71F);
//  tft.drawFastHLine(lineX, y+3, lineWidth, 0xFFFF); No need to draw white lines since the background is already white
//  tft.drawFastHLine(lineX, y+4, lineWidth, 0xFFFF);
  tft.drawFastHLine(lineX, y+5, lineWidth, 0x94BF);
  tft.drawFastHLine(lineX, y+6, lineWidth, 0x423F);
  tft.drawFastHLine(lineX, y+7, lineWidth, 0x423F);
  tft.drawFastHLine(lineX, y+8, lineWidth, 0x421F);
  tft.drawFastHLine(lineX, y+9, lineWidth, 0x4A7F);
  tft.drawFastHLine(lineX, y+10, lineWidth, 0xCE9F);
  y+= 50;
  tft.drawFastHLine(lineX, y, lineWidth, 0xCE9F);
  tft.drawFastHLine(lineX, y+1, lineWidth, 0x4A7F);
  tft.drawFastHLine(lineX, y+2, lineWidth, 0x421F);
  tft.drawFastHLine(lineX, y+3, lineWidth, 0x423F);
  tft.drawFastHLine(lineX, y+4, lineWidth, 0x423F);
  tft.drawFastHLine(lineX, y+5, lineWidth, 0x94BF);
//  tft.drawFastHLine(lineX, y+6, lineWidth, 0xFFFF);
//  tft.drawFastHLine(lineX, y+7, lineWidth, 0xFFFF);
  tft.drawFastHLine(lineX, y+8, lineWidth, 0xE71F);
  tft.drawFastHLine(lineX, y+9, lineWidth, 0xC61F);
  tft.drawFastHLine(lineX, y+10, lineWidth, 0xEF5F);
}

