<div align="center">
  <img src="https://github.com/Anvarys/lightclock/blob/main/images/simplified_design.png?raw=true" alt="Light clock" width="300">
</div>

<h2 align="center">Light Clock</h2>
<p align="center">
A custom PCB-based Clock/Alarm with a runrise alarm, a circle of capacitive touch controls to easily choose the time and a 2.42" OLED display
</p>

## About the project

A custom PCB-based Clock/Alarm with a 3d printed enclosure. It features a runrise alarm option (72 LEDs), a 2.42" OLED display, many capacitive touch controls (24 around in a circle and 4 for the main controls) and a buzzer.

I made this project because I wanted to have a runrise alarm to wake up more naturally and also be easy to set up alarms with few clicks and without any additional installation required.

## Light Clock's features in more detail

### Sunrise alarm & LEDs

Light Clock has a runsrise type of alarm, which will slowly rise the luminosity up to a set time for you to wake up naturally, like with a real sunrise. This can be calibrated, meaning you can set up a test alarm on a day you don't need to wake up at a certain time and see at which luminosity you will wake up. There is also a buzzer (the shaky circle on the simplified design picture) to wake you up with it, just in case, as you might not wake up from the light.

It uses 72LEDs with 3000K warm lighting, with a total maximum luminosity flux of over 2000 lumens. They can also be lighten up in individual groups of 3, so a total of 24 groups is present, to show the current time using those same LEDs but dimmed, also as it has 24 groups it can do both 24 and 12 sections for the time, which you can choose as you prefer.

### Capacitive touch controls

This project has a total of 28 capacitive touch sensors (you can identify all of them on the simplified design picture as the ones having unclear borders), they were made by placing pads on the PCB and using ICs to measure precise capacitance values. 

4 of them are used to control the menu: 2 arrows for navigating between options and change values, a circle to select an option and a square be used as a return button.

And the other 24 are distributed evenly in a circle next to the LEDs, they will be used to easily choose time of your alarm or change number variables. For example if you need to setup an alarm for 10:15am you can simply click the 10th pad counting clockwise from the top to set the hour, and then click on 7th pad to set the minutes.


### Display

The display is a 2.42" OLED display from Waveshare. It has a resolution of 128x64, which is not detailed, but very sufficent for pixeled fonts and interfaces. It will display the time in the format you prefer, and it can be set into a "show on touch" mode, so that the display would be off by default but when you touch any capacitive touch sensor it will show the time for a certain period of time.


### Other

It also has an RTC clock with a coin battery to sustain precise time for months, while the battery is not required it is very recommended as then if you power off the clock you won't need to set up the time again each time.

Since the LEDs and ICs produce heat there is a 40x40x7mm fan on the back which is very quiet, so should not be a worry that it might be disturbing.

Also I added 4 exposed GPIO pads on the PCB that you can use them to integrate something extra to this device if you want to.


## Tools I used

<ul style="list-style-type:none;">
  <li>PCB design: <a href="https://kicad.org">KiCAD</a></li>
  <li>3D design: <a href="https://www.autodesk.com/products/fusion-360/overview">Autodesk Fusion</a> (free version)</li>
  <li>Art design: <a href="https://figma.com">Figma</a></li>
</ul>