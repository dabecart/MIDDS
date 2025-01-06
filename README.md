<img src="media/logo/MIDDS_Logo2.png" width="800"/>

# MIDDS
*Monitoring Interface of Digital and Differential Signals, by @dabecart.*

## Overview

The **MIDDS** is a peripheral board designed to connect to a computer via USB, enhancing your software with timestamped GPIO capabilities. More than just a reliable General-Purpose Input/Output device, it delivers precise and high-accuracy timestamping. Featuring 14 configurable channels, the MIDDS supports TTL or LVDS input and output modes. These channels enable timestamping for both inputs and outputs, with each channel capable of being selected as the SYNC input. This SYNC input can accept a square signal from an external clock source to synchronize the MIDDS's time with the external clock.

For even greater precision, the MIDDS supports an external clock signal as HCLK, which can be connected through an onboard SMA connector. The board also includes a TFT LCD screen for convenient display of the monitor's current state. Additionally, the MIDDS offers 16 extra GPIOs, configurable as digital or analog inputs/outputs. All external connections are ESD-protected.

The MIDDS solves two problems on the same device:

- For one, it works as a digital oscilloscope of high timing accuracy relative to its price.
- And it works as a GPIO card, allowing the computer to generate both input and output signals.

It can therefore be used in the following applications:

- Cost-effective laboratory equipment and instrumentation.
- Sensor calibration.
- Multi-protocol interface adapter (SPI, I2C).
- Board to board communication.
- GPS.
- Test and measurement.

## How it works
Read more about it [here](docs\README.md)!

# License
This project is licensed under MIT License. Read the [LICENSE file](LICENSE).