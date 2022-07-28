<!--
  Copyright (c) 2022 John Buonagurio (jbuonagurio at exponent dot com)
  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or copy at
  https://www.boost.org/LICENSE_1_0.txt)
-->

## Fan Control Board

**See [jbuonagurio/fanboard-pcb](https://github.com/jbuonagurio/fanboard-pcb) for hardware details.**

This is a custom Wi-Fi module project for the [Haiku L Ceiling Fan](https://bigassfans.com/haiku-l/) from Big Ass Fans
which provides native support for Apple HomeKit. This project includes a comprehensive port of the Apple HomeKit Open
Source ADK ([apple/HomeKitADK](https://github.com/apple/HomeKitADK)) to the TI
[CC3235MODASF](https://www.ti.com/product/CC3235MODASF) Wi-Fi MCU running FreeRTOS. As an experimental tool for
professional IoT device development, the project also includes many additional features to improve
the developer experience:

* CMake support for the [TI SimpleLink CC32xx SDK](https://www.ti.com/tool/SIMPLELINK-CC32XX-SDK).
* Minimal dependencies: uses native FreeRTOS API as opposed to TI DPL; simplified startup code and linker script.
* Supports both ARM-GCC and [LLVM Embedded Toolchain for Arm](https://github.com/ARM-software/LLVM-embedded-toolchain-for-Arm).
* CMSIS SVD files for CC32xx MCUs provide interoperability with industry-standard debugging tools.
* FreeRTOS tracing support using [SEGGER SystemView](https://www.segger.com/products/development-tools/systemview/).
* Local logging support using [SEGGER RTT](https://www.segger.com/products/debug-probes/j-link/technology/about-real-time-transfer/).
* Remote logging support using syslog.
* REST API for local firmware OTA updates.
* Hardware includes boot mode switches, indicator LEDs and I2C interface for external sensors.

Electrical safety and equipment protection are key considerations in this design:

* The [TVS3301](https://www.ti.com/product/TVS3301) bidirectional SPD and [TPS26624](https://www.ti.com/product/TPS2662)
  eFuse provide industrial-grade power path protection against short-circuit, overcurrent, undervoltage, overvoltage,
  and reverse polarity conditions.
* The [ESDS302](https://www.ti.com/product/ESDS302) TVS diode array protects UART signal lines from ESD and surge events.

Licensed under the [Boost Software License](http://www.boost.org/LICENSE_1_0.txt).

### Notice

This is an experimental design intended for developer use only. You are solely responsible for ensuring your application
meets applicable standards, and any other safety, security, regulatory or other requirements.
