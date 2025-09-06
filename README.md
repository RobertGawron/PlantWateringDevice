# Plant Watering Device

This is a device for watering indoor plants based on the 8-pin PIC10F200-IOT microcontroller.

The main goal is to try to make a project on this small microcontroller and deal with its limitations.

Features:
* Dual watering modes:
 * Automatic: Waters the plant when the soil humidity sensor detects dry soil
 * Scheduled: Waters at configurable intervals with configurable water amounts
* 3D-printable chassis that securely mounts both the pump and PCB
* LED bar indicator for status monitoring


![PCB render](./Documentation/Pictures/render_06_09_2025.png)

## Software

The software will be written in assembler.

## Hardware

A lot of cheap modules were used to simplify the design.

![Electronic Design](./Hardware/PlantWateringDevice/PlantWateringDevice-DataHandling.svg)
![Electronic Design](./Hardware/PlantWateringDevice/PlantWateringDevice-HumiditySensor.svg)
![Electronic Design](./Hardware/PlantWateringDevice/PlantWateringDevice-InMUX.svg)
![Electronic Design](./Hardware/PlantWateringDevice/PlantWateringDevice-Mechanic.svg)
![Electronic Design](./Hardware/PlantWateringDevice/PlantWateringDevice-OutMUX.svg)
![Electronic Design](./Hardware/PlantWateringDevice/PlantWateringDevice-PowerManagement.svg)
![Electronic Design](./Hardware/PlantWateringDevice/PlantWateringDevice.svg)

Design tools: KiCad

## Mechanical

[Mechanical specifications and assembly details.](./Mechanic/README.md)