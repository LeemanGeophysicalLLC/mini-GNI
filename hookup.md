# Adafruit Feather Hookup

Hooking up the anemometer to the Adafruit Feather and logger shield is a
quick and simple task, requring only minimal soldering.

## Tools and Materials Required

* Soldering Iron
* Solder
* Adafruit Feather M0 with RFM95 LoRa Radio
* CR1220 12mm Diameter - 3V Lithium Coin Cell Battery
* Adalogger FeatherWing - RTC + SD Add-on For All Feather
* 10k resistor
* Completed anemometer and cable
* Small diagonal style wire cutters
* Heat shrink tube
* Heat gun

## Hookup
1\. Solder the male headers onto the logger shield.

2\. Solder the female headers onto the feather M0.

3\. Cut the cable that came with the fan kit at the end that does not plug into
the fan. Strip and tin the three wires.

4\. Solder the red wire and one end of the 10k resistor to the 3V3 pad on
the logger shield.

5.\ Solder the black wire to a ground pad on the logger shield.

6\. Solder the yellow wire and a second short length (2") of wire
to pin 6 (digital I/O) of the shield.

7\. Slip a short piece of heat shrink tube over the yellow wire. Trim the free
end of the 10k resistor to about 1/4" and then trim and strip the yellow wire
such that it reaches the resistor.

8\. Solder the yellow wire and resistor together.

9\. Install the coin cell battery for the real time clock.

10\. Slip the heat shrink over the resistor and solder joint. Shrink it with the
heat gun. Be careful not to burn yourself. A hot air station set at 125C works
well also.

<center>
  <img src="assets/img/hookup_close.JPG" alt="hookup close up" width="300"/>
</center>

10\. Plug the fan into the cable - hookup is complete!

<center>
  <img src="assets/img/hookup_complete.JPG" alt="hookup complete" width="300"/>
</center>
