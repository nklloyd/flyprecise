# Pressure Sensor Calibration for Homemade Densimeter

The sensor outputs an analog voltage that varies with applied force. 
This voltage is converted into a digital value using `analogRead()` (range: 0–1023).

A linear calibration equation is used to convert this reading into mass

## Calibration Data

The following hardcoded values are used:

* `zeroMass = 49.8`
* `zeroReading = 778`
* `knownMass = 110.6`
* `knownReading = 259`

## Output

The program continuously:

1. Reads the analog sensor value
2. Converts it to mass using the calibration equation
3. Prints both raw reading and calculated mass to the serial monitor


```
Raw reading: 512
Mass: 85.3 g
```

---

## Known Bugs / Limitations

Different resistor values are being tested in the voltage divider to improve sensitivity and accuracy. 
Results may vary depending on the resistor.

### 2. Non-Linear Sensor Behavior

FSRs are inherently non-linear. This implementation assumes a linear relationship, which may introduce error, especially outside the calibrated range.

## Hardware Notes

* Sensor: Force-Sensitive Resistor (FSR)
* Input Pin: A0
* Microcontroller: Arduino-compatible board
* Circuit: Voltage divider (FSR + resistor)

---

## Future Work

* Implement non-linear calibration (e.g., exponential/log fit)
* Integrate mass readings into full density calculation pipeline
* Add real-time calibration interface via serial input
* Improve repeatability and measurement stability

---

## Alpha Build Documentation
* Alpha Build - https://docs.google.com/document/d/1_ZNRy5aV_9XcpFfJprM5AeEUtTBAZkFf1owPfBoO9iQ/edit?usp=sharing
* Alpha Test Plan - https://docs.google.com/document/d/1SXkQXstxGwYJmzBSkHG-z-8G0-C6oroAPGTkn4TiCtQ/edit?usp=sharing

## Beta Build Documentation
* Beta Build - https://docs.google.com/document/d/1fs_zSv_gkhbn85_mFjjC6zrKmSh5nXOdgN2fiGV2G-g/edit?usp=sharing
* Beta Test Plan - 
