# RO Data Update Guide for nRF52

This guide explains how to flash read-only factory data to the NV storage.

Please note that this guide is applicable to **nRF52832 or nRF52833** chip only. So if you want to use other chipsets, Please contact a technical support channel.

***


## 1. Download TagOnboardingConfig.h

For instructions on downloading the `TagOnboardingConfig.h` file and copying it to the `conf` directory, refer to the following document

## 2. Update data to the device

(1) Prerequisites
- Download [nrf-command-line-tools](https://www.nordicsemi.com/Products/Development-tools/nRF-Command-Line-Tools/Download)

(2) Copy `TagOnboardingConfig.h` file to `tools/scripts` directory
```sh
$ cp conf/TagOnboardingConfig.h tools/scripts
```

(3) Create a `Factory_Data_Sample.csv` file and place it in the `tools/scripts` directory
```sh
$ cd tools/keygen
$ python3 tag_keygen.py --input sn.csv
Loading sn.csv...
Factory Data has been saved: Factory_Data_Sample.csv
Batch file has been saved: output_bulk/2024****_144457/DI_Batch_2024****_******.csv

$ cp Factory_Data_Sample ../../tools/scripts
```

(4) To update the read-only factory data to the NV storage, run the following script:
```sh
cd tools/scripts
python3 prog_ro_data.py
```

- The updated factory data binary format is as follows.

    | Item              | Offset | Length   |
    | ----------------- | ------ | -------- |
    | Model Name        | 0      | 32 bytes |
    | Vendor Id         | 32     | 40 bytes |
    | Manufacturer ID   | 72     | 4 bytes  |
    | Manufacturer Name | 76     | 32 bytes |
    | Setup ID          | 108    | 4 bytes  |
    | Length of S/N     | 112    | 4 bytes  |
    | S/N               | 116    | 32 bytes |
    | Length of Sec Key | 148    | 4 bytes  |
    | Sec Key           | 152    | 48 bytes |


## 3. Disable TAG_CONFIG_USE_ONBOARD_CONF_HEADER and TAG_CONFIG_USE_DEVICE_INFO_HEADER
To use the updated read-only data on the NV storage, comment out the `TAG_CONFIG_USE_ONBOARD_CONF_HEADER` option and `TAG_CONFIG_USE_DEVICE_INFO_HEADER` option in the `ProjectConfig.h` file.
