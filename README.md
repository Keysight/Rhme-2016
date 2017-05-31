# Rhme-2016

## Challenge Binaries

You can use the challenge binaries on a normal Arduino Nano or Uno board (atmega328p chip). To upload the challenge to the board, use the following command:

    avrdude -c arduino -p atmega328p -P /dev/ttyUSB* -b115200 -u -V -U flash:w:CHALLENGE.hex

Please keep in mind that depending on the bootloader that is installed on your board, the baudrate will change. Stock Nano baudrate should be 57600, and stock Uno is 115200. (Thanks [HydraBus]kag for this info).

## Write-ups
* https://github.com/hydrabus/rhme-2016
* https://www.balda.ch/posts/2017/Mar/01/rhme2-writeup/
* https://www.youtube.com/playlist?list=PLhixgUqwRTjwNaT40TqIIagv3b4_bfB7M
* https://ctf.rip/rhme2-secretsauce/
* https://ctf.rip/rhme2-fridgejit/
* https://github.com/mrmacete/writeups/tree/master/rhme2/fridge-plugin
* https://github.com/gijsh/rhme2_writeups
* https://www.youtube.com/watch?v=6_Z5RcHoykE
* https://n0wblog.blogspot.nl/2017/03/rhme2-writeups-secure-filesystem.html
* https://n0wblog.blogspot.nl/2017/03/rhme2-writeups-jumpy.html
* https://firefart.at/post/rhme2_whac_the_mole/


## Old readme
The second round is coming!

The RHme2 (Riscure Hack me 2) is our low level hardware CTF challenge that comes in the form of an Arduino Nano board. The new edition provides a completely different set of new challenges to test your skills in side channel, fault injection, cryptoanalysis and software exploitation attacks.

For more information and registration, visit http://rhme.riscure.com

Follow us on twitter at @riscure and #rhme2 for updates. 

