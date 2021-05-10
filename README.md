# ESPHome Single Button Cover
Project to control a garage cover or gate with ESPHome and Home Assistant.  
This project uses an esp board, a relay and two reed switches.

## Cover description
Cover is controlled with a single button push. Each time the button is pushed, it performs an action according following state machine:  
open -> stop -> close -> stop -> open

## Project features
* Position reporting based on time (no position control for now).
* Calculate the number of times the button need to be pushed to perform the action requested.
* Detect and update position when the cover is externally commanded when full open or closed.
* Configuration options for GPIOs, debounce time, open/close durations. time between clicks...

## TODO
* Position control
* Increase position accuracy when button is pushed more than once (need ability to wait for a script to finish)