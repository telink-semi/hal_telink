# Firmware Update Guide for nRF Connect SDK (V2.9.0)

nRF54L15 or nRF52840 board with nRF Connect SDK(NCS) implements bootloader and firmware by applying [MCUboot](https://docs.nordicsemi.com/bundle/ncs-2.9.0/page/mcuboot/readme-ncs.html).

To include MCUboot with NCS application, the necessary configuration options like `SB_CONFIG_BOOT_SIGNATURE_XXXX` are enabled in `example/nrf_ncs/nRF5XXXX/sysbuild.conf`.

## 1. Sign the image

nRF Connect SDK provides helpful tools to sign the built image for secure boot and firmware update.

### 1.1 Preconfiguration
``` sh
cd ${ncs_path}/bootloader/mcuboot
pip install --user -r scripts/requirements.txt
```

### 1.2 Generate a new keypair
``` sh
cd ${ncs_path}/bootloader/mcuboot
./scripts/imgtool.py keygen -k mykey.pem -t rsa-2048
```

### 1.3 Specify the keypair
Enable `SB_CONFIG_BOOT_SIGNATURE_XXXX` options in `sysbuild.conf` file and Modify `SB_CONFIG_BOOT_SIGNATURE_KEY_FILE` option to path of your `pem` file.

Then the new keypair can be used to sign the image.

```diff
-# SB_CONFIG_BOOT_SIGNATURE_TYPE_RSA=y
-# SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=n
-# SB_CONFIG_BOOT_SIGNATURE_KEY_FILE="/path/to/mykey.pem"
+SB_CONFIG_BOOT_SIGNATURE_TYPE_RSA=y
+SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=n
+SB_CONFIG_BOOT_SIGNATURE_KEY_FILE="/path/to/mykey.pem"
```

**Note**: if `CONFIG_MCUBOOT_SIGNATURE_KEY_FILE=""`, the default keypair will be used and this should NOT be applied in production.

## 2. Build the image

In  `example/nrf_ncs/` example, the signed update image is built automatically during the building operation.

The signed update image is located in `example/nrf_ncs/nRF5XXXX/build/nRF5XXXX/zephyr/zephyr.signed.bin`.

## 3. Upload the image to Developer Workspace
The signed image  `zephyr.signed.bin`  can be uploaded to Developer workspace directly, more information please refer to [Firmware Update Test Method](./Firmware_update_guide.md#3-Firmware-Update-Test-Method).
