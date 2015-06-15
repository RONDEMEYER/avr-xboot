# XBoot Intrduction #

XBoot is an extensible, modular bootloader for the ATMEL AVR processor series, compatible with both AVR ATMEGA and AVR XMEGA devices with sufficient memory. It is compatible with the AVR109 (butterfly) bootloader protocol with a few XMEGA specific extensions for access to the user and production signature rows. XBoot includes several advanced features including multiple serial busses and an API providing the ability for the running application to update itself without the need for external programming hardware. Many bootloaders only support RS232 for programming from a PC, but XBoot's modularity allows it to support the exact same set of commands over any hardware serial port. Currently, I²C, RS485, and parallel FIFO support have been incorporated. This allows for easy in-system reconfiguration of XBoot equipped chips with little additional time investment. Also, XBoot includes support for I²C address autonegotiation for when multiple, identically configured processors sit on the same I²C bus.

Note: This repository has been moved to https://github.com/alexforencich/xboot

Official xboot wiki is located at http://alexforencich.com/wiki/en/xboot/

# Features #

  * Supports all ATMEL XMEGA series processors
  * Supports any ATMEL ATMEGA processor with sufficient memory (needs 4K of boot space)
  * AVR109 (AVR Butterfly) compatible protocol
  * RS232, RS485, I²C, and parallel FIFO communication
  * Highly reconfigurable
  * Firmware update API to allow updating client firmware with no external programming hardware
  * Supports I²C bus address autonegotiation (XMEGA only)

# Documentation #

  * http://alexforencich.com/wiki/en/xboot/