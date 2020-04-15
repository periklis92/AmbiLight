# AmbiLight
### DIY Ambilight for your PC
<p> 
  So I made an Ambilight from an RGB LED Strip and I wanted to share the project code and schematics here in case anyone wants to give it a shot.
</p>
  <p>
  The main idea is that the python program communicates with the device (in my case it is a wemos d1 mini board) in order to send RGB values for three parts of the screen (left, right, top), which then lights up the LEDs, which are split in three different parts, each one consisting of 6 LEDS (18 total), positioned accordingly around my computer.
</p>
<p>
  The program for the board is written in vscode using the platformio IDE and the arduino framework. It should work fine if you are just using the Arduino IDE, although I haven't tested it yet.
</p>

## Schematics

![alt text](https://github.com/periklis92/AmbiLight/blob/master/schematic.jpg "Schematics")
