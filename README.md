Controleo3 Oven Controller
==========================

Controleo3 is a closed-loop oven controller, reading the oven temperature using a thermocouple and modulating power to relays to achieve the desired temperature curve or profile.  The operation is guided by "profiles" (see https://whizoo.com/profiles) which are easy to write, flexible and powerful.  While Controleo3 is primarily a reflow oven controller, it can be used for any oven like kilns or curing ovens.

Reflow oven build guide:  
https://www.whizoo.com/reflowoven

Buying Controleo3:  
https://www.whizoo.com/buy

Updating the firmware running on Controleo3:  
https://www.whizoo.com/update

### This is the GitHub source code repository for Controleo3.

This software has been released under the MIT license.  You are free
To use it any way you wish.

In this folder are:
1. README - this file
2. Files that comprise the Controleo3 library for Arduino
3. An "examples” folder containing the Reflow Wizard software

To install the Controleo3 library, please refer to:  
http://arduino.cc/en/Guide/Libraries or http://www.whizoo.com/update

## Reflow Wizard

* 1.0  Initial public release. (21 August 2017)  
* 1.1  Bug fixes and features (9 September 2017)
  * Fixed bug where servo movement would be erratic
  * Fixed bug where running learning twice could cause the oven to overheat
  * Added message when SD card wasn't FAT formatted
  * Profile commands: added "beep" and ability to specify just how much the oven door should open
* 1.2  Tweaks for ovens with slow-response heating elements (1 November 2017)
  * More responsiveness for all ovens, especially those with slow-response heating elements
  * Minor oven scoring change to better reflect reality
  * Number of reflows is now incremented as expected (Settings -> Stats)
* 1.3  Added new profile command "maintain" (15 November 2017)
  * The oven can now hold a specific temperature for a certain duration
  * See https://www.whizoo.com/profiles for more information
* 1.4  Minor improvement (18 December 2017)
  * When switching between non-FAT and FAT16/FAT32 formatted cards, a reboot was needed.  This has been fixed.
  * Added comments and fixed spelling mistakes in the source code
* 1.5 Major update (15 October 2019)
  * Added graphing to the reflow screen, controlled by new profile commands "show graph" and "start plotting"
  * Added option to log the time and temperature to the SD card
  * Improved, more consistent learning sequence that takes 25 minutes instead of 1 hour
  * Added ability to manually tune PID 
  * Better support for other oven types, like kilns or curing ovens
  * Added support for single-element ovens
  * Improved "max deviation" logic, and more information is provided when it occurs
  * New "user taps screen" profile command
  * Changed licensing to less restrictive MIT license.  You can use the source code as you wish


Peter Easton

whizoo.com


