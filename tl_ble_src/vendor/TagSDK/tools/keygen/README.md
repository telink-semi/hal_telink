# Ed25519 Key Pair generation tools for Individual and Commercial

[![License](https://img.shields.io/badge/license-Apache%202.0-brightgreen.svg?style=flat)](./../../LICENSE)

## Summary

This repository provides a tools to generate key pair and batch file for Developer Workspace.
Ed25519 is a signature algorithm with EdDSA over curve25519.

## Remark

This utility is limited to generate key pairs for test devices.
Each manufacturer should use their own method to meet security requirements for commercial devices.  

For example
 * The operation of key management facility must be conducted in a secure manner.
 * If device key pair is generated outside device and later injected, the generated private key must only be maintained outside the device in encrypted form and should be deleted after successful injection.
 * The generated key pairs must be globally uniquely identified independently based on its identifiers.

Please refer security guideline document which is available at `Publish` menu of Developer Workspace.

## Requirement

* The length of serial number should be between 11 and 30.
* The generated private key must only be maintained outside the device in encrypted form and should be deleted after successful injection to device.
* The generated key pairs must be globally uniquely identified independently based on its identifiers.
* The key management facility must provide a way to check the validity of each key pair for known compromised devices.

## Prerequisites

Install pynacl python package
```sh
pip install pynacl --user
```

## Usage

```sh
python3 tag_keygen.py [-h] [--mnid MNID] [--input csv] [--output csv] [--config_path CONFIG_PATH] [--qr]
```

### Individual

Example
```sh
$ python3 tag_keygen.py
Use following serial number and public key
for the identity of your device in Developer Workspace.

Serial Number:
STDKod********63

Public Key:
GbZr************************************PDw=
```
Output files
```sh
$ tree output_STDKod********63/
output_STDKod********63/
├── TagDeviceInfo.h
├── device.curve25519_seckey.b64
├── device.ed25519_pubkey.b64
└── device.ed25519_seckey.b64

```
You can add your *MNID* in serial number.
```sh
$ python3 tag_keygen.py --mnid TEST
Use following serial number and public key
for the identity of your device in Developer Workspace.

Serial Number:
STDKTEST****BCkH

Public Key:
6HMs***********************************QMko=


$ cat output_STDKTEST********/TagDeviceInfo.h
#ifndef TAGSDK_TAGDEVICEINFO_H_
#define TAGSDK_TAGDEVICEINFO_H_

const char *conf_device_seckey_curve25519 = "iAcQ***********************************HfWA=";
const char *conf_device_serial_number = "STDKTEST****BCkH";
#endif /* TAGSDK_TAGDEVICEINFO_H_ */
```

You can generate QR code which could be helpful to add your device in ST app.
You need to enter the path of `TagOnboardingConfig.h` to generate QR code.
```sh
$ python ./tag_keygen.py --qr --config_path ../../conf/TagOnboardingConfig.h
Use following serial number and public key
for the identity of your device in Developer Workspace.

Serial Number:
STDKTEST****PiS2

Public Key:
Zx9/***********************************OdLA=

File:    output_STDKTEST****PiS2/qr-STDKTEST****PiS2.png 
QR url: https://qr.samsungiots.com/?m=TEST&s=999&r=STDKTEST****PiS2

$ tree output_STDKTEST****PiS2/
output_STDKTEST****PiS2/
├├── TagDeviceInfo.h
├── device.curve25519_seckey.b64
├── device.ed25519_pubkey.b64
├── device.ed25519_seckey.b64
└── qr-STDKTEST****PiS2.png

```

Copy the Serial Number and Public Key after running the command. You will need to upload these values to the SmartThings Cloud via [Developer Workspace](https://developer.smartthings.com/workspace/projects) during the next phase.

If you create a device identity with a command with an option like above, you can get the ready-to-use `TagDeviceInfo.h` file directly.

### Commercial

Example
```sh
$ python3 tag_keygen.py --input sn.csv
Loading sn.csv...
Factory Data has been saved: Factory_Data_Sample.csv
Batch file has been saved: output_bulk/2020****_193746/DI_Batch_2021****_201357.csv
```
Output files:
- factory data csv file (Factory_Data_Sample.csv in below example)
    ``` sh
    $ ls Factory_Data_Sample.csv
    Factory_Data_Sample.csv
    ```
- output csv file (DI_Batch_2021****_201357.csv in below example)
- serial number named sub directory which contains key pair
    ```sh
    $ tree output_bulk/
    output_bulk/
    └── 2021****_201357
        ├── DI_Batch_2021****_201357.csv
        ├── TESTSEREAL001
        │   ├── device.curve25519_seckey.b64
        │   ├── device.ed25519_pubkey.b64
        │   └── device.ed25519_seckey.b64
        ├── TESTSEREAL002
        │   ├── device.curve25519_seckey.b64
        │   ├── device.ed25519_pubkey.b64
        │   └── device.ed25519_seckey.b64
        └── TESTSEREAL003
            ├── device.curve25519_seckey.b64
            ├── device.ed25519_pubkey.b64
            └── device.ed25519_seckey.b64

    ```
You can generate QR code for commercial
You need to enter the path of `TagOnboardingConfig.h` to generate QR code.
```sh
$ python3 tag_keygen.py --input sn.csv --qr --config_path ../../conf/TagOnboardingConfig.h
Loading sn.csv...
...
Factory Data has been saved: Factory_Data_Sample.csv
Batch file has been saved: output_bulk/2021****_193746/DI_Batch_2021****_193746.csv
```
Output files:
- factory data csv file (Factory_Data_Sample.csv in below example)
    ``` sh
    $ ls Factory_Data_Sample.csv
    Factory_Data_Sample.csv
    ```
- output csv file (DI_Batch_2021****_193746.csv in below example)
- serial number named sub directory which contains key pair and QR code image
    ```sh
    $ tree output_bulk/
    output_bulk/
    └── 2021****_193746
        ├── DI_Batch_2021****_193746.csv
        ├── TESTSEREAL001
        │   ├── device.curve25519_seckey.b64
        │   ├── device.ed25519_pubkey.b64
        │   ├── device.ed25519_seckey.b64
        │   └── qr-TESTSEREAL001.png
        ├── TESTSEREAL002
        │   ├── device.curve25519_seckey.b64
        │   ├── device.ed25519_pubkey.b64
        │   ├── device.ed25519_seckey.b64
        │   └── qr-TESTSEREAL002.png
        └── TESTSEREAL003
            ├── device.curve25519_seckey.b64
            ├── device.ed25519_pubkey.b64
            ├── device.ed25519_seckey.b64
            └── qr-TESTSEREAL003.png

    ```
Change the output batch file location
```sh
python3 tag_keygen.py --input sn.csv --output output/batch.csv
```

#### To inject the information of csv file for commercial
* [RO Data Update Guide for nRF52](../../doc/RO_update_guide_for_nR52.md)
* [RO Data Update Guide for atm33](../../doc/RO_update_guide_for_atm33.md)

#### csv format
* Input csv file
has 'serial number' list
**header**
sn
**example**
    ```bash
    sn
    TEST12345678
    TEST23456789
    TEST34567890
    ...
    ```
* Output csv file
**header**
sn,keyType,keyCrv,pubkey
**example**
    ```bash
    sn,keyType,keyCrv,pubkey
    TEST12345678,ECPUBKEY,ED25519,JSHp***********************************02Y0=
    TEST23456789,ECPUBKEY,ED25519,AYzF***********************************zl8k=
    TEST34567890,ECPUBKEY,ED25519,5rfr***********************************8wQo=
    ...
    ```

#### QR generator
You can generate QR code image using `TagDeviceInfo.h` generated by keygen and `TagOnboardingConfig.h`.
You need to enter the path of directory contain `TagDeviceInfo.h` and `TagOnboardingConfig.h`.

```sh
$ python3 ./qrgen.py  --path ../../conf/
File:    output_STDKTEST****PiS2/qr-STDKTEST****PiS2.png
QR url: https://qr.samsungiots.com/?m=TEST&s=999&r=STDKTEST****PiS2
```
