# Anemometer Calibration

Calibration of the anemometer is accomplished trough a transfer of calibration
from a known instrument. It is recommended that this be compared with an
instrument in the field as well to ensure there is no laboratory bias. The
NF-A4x10 FLX fans have proven to be consistent enough that a single calibration
curve derived from 5 units can be used for all units with no more than 0.5 m/s
error.

The result of five anemometer calibrations is summarized in the charts below
showing the individual fits, the fit to all data, and the resulting prediction
error from using the master calibration curve on all data.

To perform a calibration on an anemometer, the following procedure has proven to
be very reproducible.

## Tools and Materials Required

* 4" exhaust fan
* Flexible 4" duct hose
* 3D printed calibration tool
* (2) 4" hose clamps
* Variable transformer
* Handheld anemometer
* Feather M0 or mini-GNI unit
* Anemometer
* Screwdriver

## Calibration Procedure
1. Attach the flexible duct hose to the exhaust fan with a hose clamp such that
the fan will pull air into the hose.

2. Place the 3D printed calibration tool in the end of the flexible hose such
that it is flush with the hose end and attach it with a hose clamp.

3. Secure the hose to a workbench with clamps, tape, or another method of
fixation.

4. Plug the exhaust fan into the variable transformer.

5. Connect the anemometer to the mini-GNI or M0 logger and begin logging
frequency data from the fan. If using a bare M0 there is a calibration program
in the repository. This program sends data over a serial terminal and should be
recorded with a serial terminal application such as CoolTerm.

6. Turn on the variable transformer and set it to 120 VAC.

7. Place the anemometer onto the calibration fixture, paying attention to the
air flow direction arrows.

8. Collect data for a minimum of 30 seconds and/or unit the frequency reading
stabilizes. (As you get to lower voltages this takes longer and you'll need to
average the readings as fan speeds may surge with line voltage variations.)

9. Remove the anemometer from the fixture, being careful to not get your fingers
in the spinning blades. Immediately place the handheld anemometer in the flow
and record its reading.

10. Repeat steps 6-9 for as many calibration speeds as desired - change the
voltage on the variable transformer to change the wind speed.

11. Plot the time series frequency data and pick out the average frequency for
each calibration speed/voltage.

12. Construct the calibration curve of wind speed and frequency.


Note that the exhaust fan will take some time to spin up/down. Also the handheld
anemometer readings after each step are essential as small changes in line
voltage from the electric utility can result in fractional m/s differences in
the wind speed generated.
