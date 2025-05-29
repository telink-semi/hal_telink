# SmartThings Find Device SDK

[![License](https://img.shields.io/badge/license-Samsung%20License-blue.svg?style=flat)](./LICENSE)

SmartThings Find Device SDK is the core device library that makes BLE device to work with SmartThings Find Service. 
It would supply ready-to-use *port* code for some BLE chipsets. Of course, you can write own *port* code if you're using another BLE chipsets. 
You may simply build your own device firmware with given application example.

## Directory layout

SmartThings Find Device SDK is delivered via the following directory structure :

- `conf`: The configuration files which contains device and/or project specific information. 
  - *TagConfig.h* which defines feature set of this SDK 
  - *TagFwVersion.h* which contains firmware version information
  - *TagDeviceInfo.h* which contains device's identity information (test device only) 
  - *TagOnboardingConfig.h* which contains project specific information
- `doc`: documents for helping developer to understand this SDK
- `example`: application example source code
- `inc`: header files
- `src`: SmartThings Find Device SDK *core* logic including tag state manager, NV manager, log manager, firmware update manager and sound player
- `port/inc`: prototype of SmartThings Find Device SDK port interface between core and port implementation.
- `port/XXX`: Implementation of BSP to support *Core*'s operation. Some *Port* functions are dependent on BSP,
  but some port functions are dependent on products. In this case, it is necessary to implement it according to the product.
- `tools`: Tools for SmartThings Find Device SDK

## How to get started?

The SmartThings Find Device SDK *Core* need to be built with platform-dependent *Port* part
and board support package ("BSP") supplied by chipset vendor. 

The supported Chipset is like below

* [Chipset information](../../../TechSupport/wiki/Supported-Chipset-Information)

And If you want to add a new chipset in the SDK, you need to implement Port layer follow the below guide.

* [How to add new chipset](./doc/How_to_add_new_chipset.md)

You can start development your own device by referring below documents.

* [Getting Started](./doc/Getting_started.md)
* [Firmware update guide](./doc/Firmware_update_guide.md)
* [FAQ](../../../TechSupport/wiki/FAQ)
