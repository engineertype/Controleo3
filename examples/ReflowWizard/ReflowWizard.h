// Written by Peter Easton
// Released under CC BY-NC-SA 3.0 license
// Build a reflow oven: http://whizoo.com
//

#define CONTROLEO3_VERSION             "v1.4"


// Fonts
#define FONT_9PT_BLACK_ON_WHITE        0
#define FONT_12PT_BLACK_ON_WHITE       1
#define FONT_9PT_BLACK_ON_WHITE_FIXED  2
#define FONT_12PT_BLACK_ON_WHITE_FIXED 3
#define FONT_22PT_BLACK_ON_WHITE_FIXED 4

#define IS_FONT_FIXED_WIDTH(x)         (x == FONT_9PT_BLACK_ON_WHITE_FIXED || x == FONT_12PT_BLACK_ON_WHITE_FIXED || x == FONT_22PT_BLACK_ON_WHITE_FIXED)

#define FONT_FIRST_9PT_BW              0
#define FONT_FIRST_12PT_BW             (FONT_FIRST_9PT_BW + 94)
#define FONT_FIRST_9PT_BW_FIXED        (FONT_FIRST_12PT_BW + 94)
#define FONT_FIRST_12PT_BW_FIXED       (FONT_FIRST_9PT_BW_FIXED + 11)
#define FONT_FIRST_22PT_BW_FIXED       (FONT_FIRST_12PT_BW_FIXED + 12)

#define FONT_IMAGES                    (FONT_FIRST_22PT_BW_FIXED + 16)
#define BITMAP_LEFT_ARROW              (FONT_IMAGES)
#define BITMAP_RIGHT_ARROW             (FONT_IMAGES + 1)
#define BITMAP_HOME                    (FONT_IMAGES + 2)
#define BITMAP_HELP                    (FONT_IMAGES + 3)
#define BITMAP_LEFT_BUTTON_BORDER      (FONT_IMAGES + 4)
#define BITMAP_RIGHT_BUTTON_BORDER     (FONT_IMAGES + 5)
#define BITMAP_SETTINGS                (FONT_IMAGES + 6)
#define BITMAP_ELEMENT_OFF             (FONT_IMAGES + 7)
#define BITMAP_ELEMENT_ON              (FONT_IMAGES + 8)
#define BITMAP_CONVECTION_FAN1         (FONT_IMAGES + 9)
#define BITMAP_CONVECTION_FAN2         (FONT_IMAGES + 10)
#define BITMAP_CONVECTION_FAN3         (FONT_IMAGES + 11)
#define BITMAP_COOLING_FAN1            (FONT_IMAGES + 12)
#define BITMAP_COOLING_FAN2            (FONT_IMAGES + 13)
#define BITMAP_COOLING_FAN3            (FONT_IMAGES + 14)
#define BITMAP_CONTROLEO3              (FONT_IMAGES + 15)
#define BITMAP_WHIZOO                  (FONT_IMAGES + 16)
#define BITMAP_CONTROLEO3_SMALL        (FONT_IMAGES + 17)
#define BITMAP_WHIZOO_SMALL            (FONT_IMAGES + 18)
#define BITMAP_TRASH                   (FONT_IMAGES + 19)
#define BITMAP_EXIT                    (FONT_IMAGES + 20)
#define BITMAP_UP_SMALL_ARROW          (FONT_IMAGES + 21)
#define BITMAP_DOWN_SMALL_ARROW        (FONT_IMAGES + 22)
#define BITMAP_UP_ARROW                (FONT_IMAGES + 23)
#define BITMAP_DOWN_ARROW              (FONT_IMAGES + 24)
#define BITMAP_HELP_ICON               (FONT_IMAGES + 25)
#define BITMAP_DECREASE_ARROW          (FONT_IMAGES + 26)
#define BITMAP_INCREASE_ARROW          (FONT_IMAGES + 27)
#define BITMAP_SMILEY_GOOD             (FONT_IMAGES + 28)
#define BITMAP_SMILEY_NEUTRAL          (FONT_IMAGES + 29)
#define BITMAP_SMILEY_BAD              (FONT_IMAGES + 30)
#define BITMAP_LAST_ONE                BITMAP_SMILEY_BAD


// Height of the button in pixels
#define BUTTON_HEIGHT                  61

#define BUTTON_SMALL_FONT              0
#define BUTTON_LARGE_FONT              1

#define LINE(x)           ((x *30) + 50)

// Tunes
#define TUNE_STARTUP                   0
#define TUNE_BUTTON_PRESSED            1
#define TUNE_INVALID_PRESS             2
#define TUNE_REFLOW_DONE               3
#define TUNE_REMOVE_BOARDS             4
#define TUNE_SCREENSHOT_BUSY           5
#define TUNE_SCREENSHOT_DONE           6
#define TUNE_REFLOW_BEEP               7
#define MAX_TUNES                      8

// Screens
#define SCREEN_HOME                    0
#define SCREEN_SETTINGS                1
#define SCREEN_TEST                    2
#define SCREEN_SETUP_OUTPUTS           3
#define SCREEN_SERVO_OPEN              4
#define SCREEN_SERVO_CLOSE             5
#define SCREEN_LINE_FREQUENCY          6
#define SCREEN_RESET                   7
#define SCREEN_ABOUT                   8
#define SCREEN_STATS                   9
#define SCREEN_BAKE                    10
#define SCREEN_EDIT_BAKE1              11
#define SCREEN_EDIT_BAKE2              12
#define SCREEN_REFLOW                  13
#define SCREEN_CHOOSE_PROFILE          14
#define SCREEN_LEARNING                15
#define SCREEN_RESULTS                 16

// When displaying edit arrow on the screen
#define ONE_SETTING                    0
#define ONE_SETTING_WITH_TEXT          1
#define ONE_SETTING_TEXT_BUTTON        2
#define TWO_SETTINGS                   3

// Help
#define HELP_OUTPUTS_NOT_CONFIGURED    80
#define HELP_LEARNING_NOT_DONE         81

// Outputs
#define NUMBER_OF_OUTPUTS              6
// Types of outputs
#define TYPE_UNUSED                    0
#define TYPE_WHOLE_OVEN                0  // Used to store whole oven values (learned mode)
#define TYPE_BOTTOM_ELEMENT            1
#define TYPE_TOP_ELEMENT               2
#define TYPE_BOOST_ELEMENT             3
#define TYPE_CONVECTION_FAN            4
#define TYPE_COOLING_FAN               5
#define NO_OF_TYPES                    6
#define isHeatingElement(x)            (x == TYPE_TOP_ELEMENT || x == TYPE_BOTTOM_ELEMENT || x == TYPE_BOOST_ELEMENT)

// To be used with setOvenOutputs()
#define ELEMENTS_OFF                   0
#define LEAVE_ELEMENTS_AS_IS           1
#define CONVECTION_FAN_OFF             0
#define CONVECTION_FAN_ON              1
#define COOLING_FAN_OFF                0
#define COOLING_FAN_ON                 1

// Having the top element on too much can damage the insulation, or the IR can heat up dark components
// too much.  The boost element is typically not installed in the ideal location, nor designed for
// continuous use.  Restrict the amount of power given to these elements
#define MAX_TOP_DUTY_CYCLE             80  
#define MAX_BOOST_DUTY_CYCLE           60

const char *outputDescription[NO_OF_TYPES] = {"Unused", "Bottom Element", "Top Element", "Boost Element", "Convection Fan","Cooling Fan"};
const char *longOutputDescription[NO_OF_TYPES] = {
  "",
  "Controls the bottom heating element.",
  "Controls the top heating element.",
  "Controls the boost heating element.",
  "On at start, off once cooling is done.",
  "Turns on to cool the oven."
};


// Reflow defines
#define REFLOW_PHASE_NEXT_COMMAND      0  // Get the next command (token) in the profile
#define REFLOW_WAITING_FOR_TIME        1  // Waiting for a set time to pass
#define REFLOW_WAITING_UNTIL_ABOVE     2  // Waiting until the oven temperature rises above a certain temperature
#define REFLOW_WAITING_UNTIL_BELOW     3  // Waiting until the oven temperature drops below a certain temperature
#define REFLOW_MAINTAIN_TEMP           4  // Hold a specific temperature for a certain duration 
#define REFLOW_PID                     5  // Use PID to get to the specified temperature
#define REFLOW_ALL_DONE                6  // All done, waiting for user to tap screen
#define REFLOW_ABORT                   7  // Abort (or done)


// Baking Defines
#define BAKE_DOOR_OPEN                 0
#define BAKE_DOOR_OPEN_CLOSE_COOL      1
#define BAKE_DOOR_LEAVE_CLOSED         2
#define BAKE_DOOR_LAST_OPTION          2

const char *bakeDoorDescription[BAKE_DOOR_LAST_OPTION+1] = {
  "Open oven door after bake.",
  "Open after bake, close when cool.",
  "Leave oven door closed."
};

const char *bakeUseCoolingFan[2] = {
  "Use cooling fan once baking is done.",
  "Don't use the cooling fan."
};

#define BAKE_MAX_DURATION              127  // 127 = 168 hours (see getBakeSeconds)
#define BAKE_TEMPERATURE_STEP          5    // Step between temperature settings
#define BAKE_MIN_TEMPERATURE           40   // Minimum temperature for baking
#define BAKE_MAX_TEMPERATURE           200  // Maximum temperature for baking

#define BAKING_PHASE_HEATUP            0    // Heat up the oven rapidly to just under the desired temperature
#define BAKING_PHASE_BAKE              1    // The main baking phase. Just keep the oven temperature constant
#define BAKING_PHASE_START_COOLING     2    // Start the cooling process
#define BAKING_PHASE_COOLING           3    // Wait till the oven has cooled down to 50°C
#define BAKING_PHASE_DONE              4    // Baking is done.  Stay on this screen
#define BAKING_PHASE_ABORT             5    // Baking was aborted
const char *bakePhaseStr[] = {"Pre-heat", "Baking", "Cooling", "Baking has finished", "Baking has finished", ""};
const uint8_t bakePhaseStrPosition[] = {190, 202, 198, 128, 128, 0};


// Should the temperature be displayed while waiting for a tap?
#define DONT_SHOW_TEMPERATURE          0
#define SHOW_TEMPERATURE_IN_HEADER     1
#define CHECK_FOR_TAP_THEN_EXIT        2

// Profiles
#define MAX_PROFILE_NAME_LENGTH        31
#define MAX_PROFILES                   28     // Defined by flash space

#define MAX_PROFILE_DISPLAY_STR        30     // Maximum length of "display" string in profile file

struct profiles {
  char name[MAX_PROFILE_NAME_LENGTH+1];       // Name of the profile
  uint16_t peakTemperature;                   // The peak temperature of the profile
  uint16_t noOfTokens;                        // Number of tokens (instructions) in the profile
  uint16_t startBlock;                        // Flash block where this profile is stored
};

#define FIRST_PROFILE_BLOCK            64     // First flash block available to store a profile
#define PROFILE_SIZE_IN_BLOCKS         16     // Each profile can take 4K (16 x 256 byte blocks)
#define LAST_PROFILE_BLOCK             (FIRST_PROFILE_BLOCK + (MAX_PROFILES * PROFILE_SIZE_IN_BLOCKS) - 1)

// Tokens used for profile file
#define NOT_A_TOKEN                   0   // Used to indicate end of profile (no more tokens)
#define TOKEN_NAME                    1   // The name of the profile (max 31 characters)
#define TOKEN_COMMENT1                2   // Comment 1 = #
#define TOKEN_COMMENT2                3   // Comment 2 = //
#define TOKEN_DEVIATION               4   // The allowed temperature deviation before the reflow aborts
#define TOKEN_MAX_TEMPERATURE         5   // If this temperature is ever exceeded then the reflow will be aborted and the door opened
#define TOKEN_INITIALIZE_TIMER        6   // Initialize the reflow timer for logging so that comparisons can be made to datasheets
#define TOKEN_START_TIMER             7   // Initialize the reflow timer for logging so that comparisons can be made to datasheets
#define TOKEN_STOP_TIMER              8   // Initialize the reflow timer for logging so that comparisons can be made to datasheets
#define TOKEN_MAX_DUTY                9   // The highest allowed duty cycle of the elements
#define TOKEN_DISPLAY                10   // Display a message to the screen (progress message)
#define TOKEN_OVEN_DOOR_OPEN         11   // Open the oven door, over a duration in seconds
#define TOKEN_OVEN_DOOR_CLOSE        12   // Close the oven door, over a duration in seconds
#define TOKEN_BIAS                   13   // The bottom/top/boost bias (weighting) for the elements
#define TOKEN_CONVECTION_FAN_ON      14   // Turn the convection fan on
#define TOKEN_CONVECTION_FAN_OFF     15   // Turn the convection fan off
#define TOKEN_COOLING_FAN_ON         16   // Turn the cooling fan on
#define TOKEN_COOLING_FAN_OFF        17   // Turn the cooling fan off
#define TOKEN_TEMPERATURE_TARGET     18   // The PID temperature target, and the time to get there
#define TOKEN_ELEMENT_DUTY_CYCLES    19   // Element duty cycles can be forced (typically followed by WAIT)
#define TOKEN_WAIT_FOR_SECONDS       20   // Wait for the specified seconds, or until the specified temperature is reached
#define TOKEN_WAIT_UNTIL_ABOVE_C     21   // Wait for the specified seconds, or until the specified temperature is reached
#define TOKEN_WAIT_UNTIL_BELOW_C     22   // Wait for the specified seconds, or until the specified temperature is reached
#define TOKEN_PLAY_DONE_TUNE         23   // Play a tune to let the user things are done
#define TOKEN_PLAY_BEEP              24   // Play a beep
#define TOKEN_OVEN_DOOR_PERCENT      25   // Open the oven door a certain percentage
#define TOKEN_MAINTAIN_TEMP          26   // Maintain a specific temperature for a certain duration

#define NUM_TOKENS                   27   // Number of tokens to look for in the profile file on the SD card
#define TOKEN_NEXT_FLASH_BLOCK     0xFE   // Profile continues in next flash block 
#define TOKEN_END_OF_PROFILE       0xFF   // Safety measure.  Flash is initialized to 0xFF, so this token means end-of-profile 

#define MAX_TOKEN_LENGTH           (MAX_PROFILE_DISPLAY_STR + 5)

// Preferences (this can be 4Kb maximum)
struct Controleo3Prefs {
  uint32_t  sequenceNumber;                   // Prefs are rotated between 4 blocks in flash, each 4K in size
  uint16_t  versionNumber;                    // Version number of these prefs
  uint16_t  screenshotNumber;                 // Next file number of screenshot
  uint16_t  topLeftX;                         // Touchscreen calibration points
  uint16_t  topRightX;
  uint16_t  bottomLeftX;
  uint16_t  bottomRightX;
  uint16_t  topLeftY;
  uint16_t  bottomLeftY;
  uint16_t  topRightY;
  uint16_t  bottomRightY;
  uint8_t   outputType[NUMBER_OF_OUTPUTS];    // The type of output for each of the 6 outputs 
  uint8_t   lineVoltageFrequency;             // Used for MAX31856 filtering
  uint8_t   servoOpenDegrees;                 // Servo door open 
  uint8_t   servoClosedDegrees;               // Servo door closed
  uint8_t   learningComplete;                 // Have the learning runs been completed?
  uint8_t   learnedPower[4];                  // The power of the individual elements and the total power
  uint16_t  learnedInertia[4];                // The thermal inertia of individual elements and the total intertia
  uint16_t  learnedInsulation;                // The insulation value of the oven  
  uint16_t  bakeTemperature;                  // Bake temperature in Celsius
  uint16_t  bakeDuration;                     // Bake duration (see getBakeSeconds( ))
  uint8_t   openDoorAfterBake;                // What to do with the oven door
  uint8_t   bakeUseCoolingFan;                // Use the cooling fan to help cool the oven
  uint16_t  numReflows;                       // Total number of reflows
  uint16_t  numBakes;                         // Total number of bakes
  uint16_t  numProfiles;                      // The number of saved reflow profiles
  uint8_t   selectedProfile;                  // The reflow profile that was used last
  profiles profile[MAX_PROFILES];             // Array of profile information
  uint16_t  lastUsedProfileBlock;             // The last block used to store a profile.  Keep cycling them to reduce flash wear

  uint8_t   spare[100];                       // Spare bytes that are initialized to zero.  Aids future expansion
} prefs;

