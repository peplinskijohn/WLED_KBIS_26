
# WLED_KBIS_26

Fork of WLED that utalizes usermod and custom hardware


## Overview

This project consisted of 22 electronic units each which controlled an addressable led string and a high-watt addressable led.  WLED was used to control the leds in conjunction with a usermod, KBIS_26, that monitored the status of a reed switch and switched wled presets accordingly.  A custom pcb was made to integrate with a DIG UNO driver board and contained a port for the above mentioned reed switch, a button, a potentiometer, and an lcd screen.  The usermod/pcb allows for the in-the-field adjustment of three dynamic variables using button and pot.  

platformio_override_reference is included in the this docs folder as a reference for setting up usermod/pcd.


## Flashing esp32

The process for uploading the pre-flashed esp32 boards:  find in AP-mode, upload presets, upload config, (wled restarts), look for new AP: wled-AP-Jay, use standard wled1234 password if necessary, change credentials in wled wifi, save, find AP again with new AP name/password, upload custom build (firmware.bin), test.


## Manual with project specific instructions

Here is the email I sent to client as user manual:

Are you still going to be glueing some doors closed?  I realized too late that this might cause an issue.  When the units are powered on for the first time they go into a bootup stage which is just a dim blue on the rings.  Changing the unit's state (after power is applied) by either opening or closing the door, pulls the unit out of the bootup mode into either the preset active or preset idle mode until the next power cycle. However, if the doors are sealed shut there is no way to pull them out of the bootup stage with the door itself.  The fix is pretty simple:  Instead of plugging in the glued-in sensors to the control unit, have a spare sensor or button (the ones I gave to Ian) hang off the back side.  After powering on, press the back button on the sealed doors once and you're good.  The same goes for doors that aren't sealed but might intentionally have their sensors unplugged, there needs to be something to get them out of bootup mode in the morning.


Modes:  the bootup state as mentioned above, then we have the idle state which is a solid blue ring at 40% brightness (no high power led), and the active state which is a swirling ring of blue and a pulsing high power led at 100% that lasts for 20s when door is first closed.  These are preprogrammed but can be altered, see below.


Installation:  I recommend installing in this order:  flange, saddle, door, electronics, then tube with ring light preinstalled.  I like to hang the plywood plate on the 1/4"-20 bolts, thumb screw the heat sink on the topmost holes, and then fasten the plywood plate with wing nuts.  The wires to the heatsink are long enough to wrap under, towards the front, and onto the top of the heatsink, see images. 


Power:  There are two power hubs on 18"x8" plywood.  The idea is to mount them in the middle of the wall, directly to the side, or maybe at the bottom.  Without knowing where, I've made a bunch of cables at different lengths: 6@8', 8@6', 9@5', 12@3'.  You could also connect cable to cable if you need more length.  They only connect one way, red to red and black to black.  There are two new-in-box 24v PSUs as backups.


Each power hub has three breakers labeled 1 thru 3 that you'll use as switches.  The first is between the house AC and the 24v PSU.  The second and third are between the PSU and a fused breakout.  One breakout has 6 cables, the other is 4.  I suggest turning the power on in sequence 1..2..3 to limit the current inrush.  If you were to apply power to all units at once I don't think anything bad would happen but I never tried.  I think in the worst case you might blow a fuse.  Likewise, all three would be turned off EOD.  


Don't plug stuff in while powered up!  Again, you probably could without incident but its best practice to power down the branch before connecting or detaching anything.


Ring Lights:  There are 24 units.  4 have black cables and are labeled as backups.  They are supposed to be the same brand but I noticed a very subtle difference in their brightness and I think the other 20 are better.  Mount the rings to the back of the tubes before installing them.  The plug should hang out at 4 o'clock looking from the back side.  There is a 3 wire female plug that the rings plug into.  They only go in one way but be gentle and make sure the red, green, and white wires line up.  The pins inside can get crushed. 


High Power LED:  These are mounted on the heatsinks and are pre-plugged ready to go.  They only come on during the active sequence so the non-opening doors don't need them.  Regardless, there are 22 of them. (one isn't mounted to a heatsink because we only have 21?).


UV Lights:  These are also mounted to the heatsinks and use a barrel connector for power.  They will always be on if plugged in.  We have only 15 of them so they should be used with the opening doors.  All the units have a barrel ready for a UVC led so you could also swap just the UVC leds if you needed to.  If someone were to forget to connect them to power, there would be no difference with anything.


Sensors:  Each sensor has a 2 pin XH connector that plugs into the black port on the control board (a few are white) .  Don't yank out the sensors.  I like to pry them out with a small flathead. You can plug in/out the sensors while powered on, I don't see the harm.  There are spare sensors packed as well.  Some have longer wire extensions.  Hopefully you don't need them.


Control board:  Each board has its own 10A fuse.  If the board is on but the leds are off it's probably because the fuse has blown or is missing.  Don't handle the fuse with the board powered up.  The alphabet labeling was for me and doesn't have any relevance.  Everything comes up automatically.  Each board has a screen, a pot, and a button.  The button allows you to scroll through 3 different variables, the pot adjusts them.  


Our default settings:  20s swirling effect time (ACT TIME), 100% brightness with this effect (ACT BRI), 40% brightness during the idle state(IDL BRI).  

Turning the pot all the way to the left displays: DFLT and uses these settings.  Any setting not default is saved to memory in about a minute and is used.  

You can also see CLSD when the sensor is tripped.  It stays "CLSD" even after the effect has timed out. 


Start up behavior:  I programmed the units to boot up in idle mode regardless of whether the door is closed or not.  This is to alleviate in-rush to the high power leds.  Additionally, they come up a little dim.  Once each door is opened or closed for the first time then everything works as intended.


TLDR:  use common sense.  take extra steps if some doors are sealed shut or have sensors intentionally disabled.