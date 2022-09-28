Modular Air Sensor MAS
===========

This is an application to build device with modular air sensors using the [RIOT-OS](https://www.riot-os.org/) middleware.


The project is based on the LORA E5 mini board of seeed studio https://wiki.seeedstudio.com/LoRa_E5_mini/

Usage
=====

## Dependecies

- RIOT-OS https://github.com/RIOT-OS/RIOT
- DLPP https://github.com/fabMSTICLig/dlpp
- Sensirion riot external drivers https://github.com/fabMSTICLig/riot-sensirion-modules

In the Makefile you can enable wich sensor you want to use and define for each of them a channel and a sampling interval.
