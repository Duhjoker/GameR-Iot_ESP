 This is the read me for the GameRIot diy game and Iot gadget. Created by Jeremy Wood Duhjoker Based loosely on 
the original Gamebuino gaming library created by Aurellian Rodot and the Arduboy library by MLXXp. 
The libraries contained with in this file are a collaboration of the best minds online including KurtE, 
PaulS, Sumotoy, with functionality Borroed from Gamebuino, Arduboy and the nice people at adafruit. It was designed 
to accomadate most modern Mcu's but runs best using a teensy 3.2 to 3.6 and esp8266 with work in progress to include 
esp32. The library is complete but some functions need help. For example it needs a bootloader gui, titlescreen needs 
to be added as well as a popup feature. There are batterymeter functions as well that can work with a 
monitor. As of writing this i have just recently added the spi tft function to draw full color bitmaps 
straight from An SD card. Optimizations incude DMA and a screen buffer plus the fastest function versions available. 

  Use of this library is at your own risk. Please refer to manufactures data sheets and pin 
outs for your particular hardwares. All credentials are listed in the Grafx file. I do not take 
credit for others work. If you feel you have not been represented, kindly email me and i will update your credit.

 For most mcu's and teensy a 3.7v lipo is the best option. Use the 3.3v 
out put pins to power your hardwares including the tft. DO Not power the tft using 5v as the screen will 
overheat and cause damage to your hardware. Do not allow the teensy to be powered by the lipo chargers 5v output.
The buttons configuration will need to be updated to match your particular pin out and can be found in the 
settings.c file. 

Hardware::::

works best with 

Teensy 3.2 to 3.6 and esp8266, seperate provisional library collaboration by myself and kurtE for the esp32 included

TFT = any TFT that shares its drivers wuth the ILI9341 SPI tft shield on pcb. Does not work with 8-bit parrelel

3.7v lipo 1500mah you dont have to use a 1500mah 1000mah wil do but the battery/ies need to be as compact and or flat as possible to accomadte other hardwares.

SD card reader if teensy 3.2 or earlier

micro usb lipo charger TP4056

12 x mini tactile switches 6x6x5mm 

on/off switch

Dupont wire jumpers

breadboard

included are stl files for cases that use 2.4 and 2.8 tfts 

hobby screws with extra wide head. screws work best

Tools::

small saw blade, nibs or bandsaw to cut breadboard

soldering iron

solder and flux

shrink tube 

lighter to heat shrink tube






If you would like to contribute please email me @ GameRIotAdvanvedGaming@yahoo.com

Case Enclosure instructions and packing::

Start by printing your choice of case or use your choice printing shop. there are two sizes one is for the 2.8 
tft and the other for a 2.4 tft's. Be sure to iclude the right button sets and back as they are different in size.
no printer ive seen prints an affordable perfect print so you will need to do some sanding and painting.start with
a good sanding using 220grit paper then spraying with color then repeating til the paint lays flat. then hit it 
with some 400 and 600 and spray one last coat of color or let cure on your fridge over night and clearcoat twice.

 Next use a 2mm drill bit and drill motor to drill out holes in each screw turret to make the screws easier to screw 
in other wise they can get stuck and break or strip. Then clip each screw to 3mm to 4mm length on the screw. 

Now we can carefully hot glue the far left or right of the case depending on your preference but.... looking from the
inside of the case from the rear there will be an indented place just wide enough to fit the screen. if the tft connectors
are on the left slide the screen all the way to the left. if on the right slide it right. Use only drops of glue to keep 
from melting your case then add more later if not secure enough.

Next we need to build the button pad. Start by making measurements of the screw turrets and thier centers so you can mark 
you bread board and cut. Next drill your mounting holes then make two crosses with two more buttons inbetween for start and 
select. next solder the buttons where all share the same ground. Now drill a hole through the center of the crosses 
and poke your jumper wires through the backside then wist each wire and poke it through any hole not soldered to 
ground at the prongs of the tactile switches and flip and solder. This makes the wires secure so they dont break
when you have to manipulate and tuck wires. do the same with the select and start buttons. Now install the actual 
buttons and screm your new button pad in tight enough to not click the tactile switches.

Next use some insulating tape or masking tape and cover all the circuits and smt diodes. Heat the tape using a lighter 
carefully not burning the tape but heating it to heat the adhesive to seal it to the board.Now use the black plastic clip 
attached to your jumpers and bent each pin 90 very carefully to the board. Next attach the battery 
to the back of the tft with a lil hot glue. The flatter the battery the more space you will have to fit more hardware.
I try to use a 5mm thick battery. at this poit you can wire the power switch the case.

Its time to wire. First solder the pins to the MCU for this build we are using a teensy3x. You want to do this 
with the long part on top and soldred from the back. Now again using the black plastic clip on your jumper wires
and bend each pin carefully over 90*. Now look at the profile and the whole mcu is 5mm thick.

The next step is to remove the plastic clips from enough jumpers to populate the tft pins. Except for the led 
pin which needs to be tied and soldred to the vcc pin wire. The tft only needs 3.3v to power both the led backlight
and the VCC. This will also save space. Place a strip of shrinktube long enough to cover the jumpers metal clip 
then solder each wire jumper head to each pin. after letting it cool pull the shrink yubes up to cover the bare metal
clips and shrink them using a lighter.

Next take your TP4056 and solder two black wires to the negative output pad and solder two red wires to the positive
pad. Now take four unsoldered bare pins and push the black plastic clip where its halfway between he pins. now use this attach 
the jumpers to the battery and one goes from the positive output to your switch and then to the teensy's vcc. 

Now ground and power the tft from the ground and 3.3v out puts on the teensy. Test and watch overheating or any
other problems. Now test the charger while the power is off. i have found that adding 5v to my teensy tends to drain power 
and send out too much heat especially where the tft is involved. Theres also the weird way in which the teensy regulates
power which can send more power through the outputs than whats called for needed. In my observations at least.

Glue you charger down in a place where you can get to it and connect your mcu. 

Now place the library files included in a folder called Gamebuinosquared in your arduino libraries. if using up to esp8266
use the main library. If using esp32 place the file marked esp into the arduino libraries folder instead. Make sure to 
also include the spin library with the main library for the buffer and dma settings.

All you have to do now is study the function list in the file marked Grafx.h and start building games.





Teensy tft wiring

vcc-----3.7v
gnd-----ground
cs--------------------10
reset---3.3
dc--------------------9
mosi------------------11
sck-------------------13
led------3.3v
miso------------------12


Esp32 tft wiring

vcc-----3.7v
gnd-----ground
cs---------------------22
reset---3.3v
dc---------------------5
mosi-------------------23
sck--------------------18
led-----3.3v
miso-------------------19
