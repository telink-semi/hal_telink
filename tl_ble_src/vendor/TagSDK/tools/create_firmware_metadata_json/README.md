# Firmware metadata creation tools

[![License](https://img.shields.io/badge/license-Apache%202.0-brightgreen.svg?style=flat)](./../../LICENSE)

## Summary

This repository provides a tools to create Firmware metadata.

## Requirement

* The Accessory options need to be changed according to the Tag's functions supported in TagSDK (TagSDK\conf\TagAccessoryOption.h).

## Usage

```sh
$ python create_firmware_metadata_json.py

 * The firmware metadata is created!
  - /home/developer/TagSDK/tools/create_test_csv/firmwareMetadata_EI-T7300_0AFD_431_1.999.901.zip
```
Output files
```sh
$ unzip firmwareMetadata_EI-T7300_0AFD_431_1.999.901.zip

$ ls
firmwareMetadata_EI-T7300_0AFD_431_1.999.901.json   TagDeviceInfo.h   TagOnboardingConfig.h   TagVersion.h

$ cat firmwareMetadata_EI-T7300_0AFD_431_1.999.901.json
{"metadata" : {"modelName": "EI-T7300", "setupId": "431", "mnId": "0AFD", "firmwareVersion": "1.999.901", "supportedFeatures": {"ringTheTag": true, "updateRingtone": true, "buttonAction": true, "batteryType": "replaceable", "leftBehindAlert": true, "firmwareUpdate": true, "ledBlinking": false}}}
```
