import zipfile
import os
from crc import Crc32, CrcCalculator
from enum import IntEnum, unique
from os import path
from os import mkdir
from lib_run_cmd import run_cmd
import json
import sys

# Base paths
in_file_path = path.relpath('./in_files')
out_file_path = path.relpath('./out_files')

# File names
private_key_file_name = 'private_key.pem'
nordic_package_file_name = 'nordic_pkg.zip'
firmware_file_name = 'firmware.bin'

# File paths
private_key_file_path = path.join(in_file_path, private_key_file_name)
nordic_package_file_path = path.join(out_file_path, nordic_package_file_name)
firmware_file_path = path.join(out_file_path, firmware_file_name)

# sd-req value, by command: `nrfutil pkg generate --help`

@unique
class DfuImageType(IntEnum):
    application = 0
    bootloader = 1
    softdevice = 2
    softdevice_bootloader = 3


class DfuImage:
    def __init__(self):
        self.type = DfuImageType.application
        self.size = 0

        self.init_packet_name = ''
        self.init_packet_addr = 0
        self.init_packet_size = 0
        self.init_packet_data = []

        self.firmware_name = ''
        self.firmware_addr = 0
        self.firmware_size = 0
        self.firmware_data = []

    def __repr__(self):
        ret_msg = 'Image\n'
        ret_msg += 'Type: {}\n'.format(str(DfuImageType(self.type)))
        ret_msg += 'Size: {:d} bytes\n'.format(self.size)
        ret_msg += 'Init packet\n name: {}\n addr: 0x{:08X}\n size: {} bytes\n'.format(self.init_packet_name, self.init_packet_addr, self.init_packet_size)
        ret_msg += 'Firmware\n name: {}\n addr: 0x{:08X}\n size: {} bytes\n'.format(self.firmware_name, self.firmware_addr, self.firmware_size)
        return ret_msg


class DfuFlatFile:
    magic_number_mcuboot = 0x96F3B83D
    magic_number_dfufile = 0x49535951
    file_header_size_max = 128
    init_packet_size_max = 512

    def __init__(self):
        self.file_name = ''
        self.file_size = 0
        self.file_data = []
        self.file_crc = 0
        self.images = []

    def make(self, zip_file_path, out_file_path):
        if not os.path.exists(zip_file_path):
            print('Input file is not existed')
            exit(1)

        self.file_name = out_file_path

        dfu_zip_file = zipfile.ZipFile(zip_file_path)
        if not len(dfu_zip_file.infolist()) == 3:
            print('Only support one file upgrading in this version')
            exit(1)

        manifest = json.loads(dfu_zip_file.read('manifest.json'))
        if manifest is None:
            print('Invalid NRF Secure DFU package')
            exit(1)

        self.file_size += DfuFlatFile.file_header_size_max

        if len(manifest['manifest']) != 1:
            print('Warning: it only supports one step of update now')
            exit(1)

        for image_type in ('application', 'softdevice', 'bootloader', 'softdevice_bootloader'):
            if image_type in manifest['manifest']:
                ip_file_name = manifest['manifest'][image_type]['dat_file']
                fw_file_name = manifest['manifest'][image_type]['bin_file']

                image = DfuImage()

                image.init_packet_name = ip_file_name
                image.init_packet_addr = self.file_size
                image.init_packet_size = dfu_zip_file.getinfo(ip_file_name).file_size
                image.init_packet_data = dfu_zip_file.read(ip_file_name)

                image.firmware_name = fw_file_name
                image.firmware_addr = image.init_packet_addr + DfuFlatFile.init_packet_size_max
                image.firmware_size = dfu_zip_file.getinfo(fw_file_name).file_size
                image.firmware_data = dfu_zip_file.read(fw_file_name)

                image.type = DfuImageType[image_type]

                # Make the size of firmware word-aligned, so the following elements
                # can also get a word-aligned address
                if image.firmware_size % 4 == 0:
                    image.size = DfuFlatFile.init_packet_size_max + image.firmware_size
                else:
                    padding_size = 4 - (image.firmware_size & 3)
                    image.size = DfuFlatFile.init_packet_size_max + image.firmware_size + padding_size

                #print(image)

                self.images.append(image)

                self.file_size += image.size

        self.file_size += 4     # len(CRC32)

        #  Fill file header
        self.file_data.extend(DfuFlatFile.get_le32(DfuFlatFile.magic_number_mcuboot))
        self.file_data.extend(DfuFlatFile.get_le32(DfuFlatFile.magic_number_dfufile))
        self.file_data.extend(DfuFlatFile.get_le32(self.file_size))
        self.file_data.extend(DfuFlatFile.get_le32(len(self.images)))

        # Fill image info
        for img in self.images:
            self.file_data.extend(DfuFlatFile.get_le32(img.type))
            self.file_data.extend(DfuFlatFile.get_le32(img.init_packet_addr))
            self.file_data.extend(DfuFlatFile.get_le32(img.init_packet_size))
            self.file_data.extend(DfuFlatFile.get_le32(img.firmware_addr))
            self.file_data.extend(DfuFlatFile.get_le32(img.firmware_size))

        # Fill padding with 0xFF
        left = DfuFlatFile.file_header_size_max - len(self.file_data)
        self.file_data.extend([0xFF] * left)

        # print('-----------------')
        # print(' '.join(['{:02X}'.format(x) for x in self.file_data]))

        # Fill image data
        for img in self.images:
            self.file_data.extend(img.init_packet_data)
            # Fill padding with 0xFF
            if img.init_packet_size < DfuFlatFile.init_packet_size_max:
                left = DfuFlatFile.init_packet_size_max - img.init_packet_size
                self.file_data.extend([0xFF] * left)

            self.file_data.extend(img.firmware_data)
            # Fill padding with 0xFF
            if not img.firmware_size % 4 == 0:
                left = 4 - (image.firmware_size & 3)
                self.file_data.extend([0xFF] * left)


        # Fill CRC32
        self.file_crc = CrcCalculator(Crc32.CRC32).calculate_checksum(self.file_data)
        self.file_data.extend(DfuFlatFile.get_le32(self.file_crc))

        try:
            out_file = open(self.file_name, 'wb+')
            out_file.write(bytes(self.file_data))
        finally:
            out_file.close()

    @staticmethod
    def get_le32(number: int):
        return number.to_bytes(4, 'little')



if __name__ == '__main__':
    """
    Usage: python create_firmware.py
    """
    print('------------------------------')
    print('1 : nRF52832_pca10040_s113');
    print('2 : nRF52832_pca10040_s132');
    print('3 : nRF52833_pca10100_s113');
    print('------------------------------')
    ic_num = input('Input your IC part number idx: ')

    if(ic_num == '1'):
        board_name = 'pca10040'
        sd_req_num = 0x0102 #0x101 = S113_v7.2.0
        sd_name = 's113'
        app_file_path = '../../example/nrf/pca10040/s113/ses/Output/Release/Exe/tag_example_pca10040_s113.hex'
    elif(ic_num == '2'):
        board_name = 'pca10040'
        sd_req_num = 0x0101 #0x101 = S132_v7.2.0
        sd_name = 's132'
        app_file_path = '../../example/nrf/pca10040/s132/ses/Output/Release/Exe/tag_example_pca10040_s132.hex'
    elif(ic_num == '3'):
        board_name = 'pca10100'
        sd_req_num = 0x0102 #0x102 = S113_v7.2.0
        sd_name = 's113'
        app_file_path = '../../example/nrf/pca10100/s113/ses/Output/Release/Exe/tag_example_pca10100_s113.hex'
    else:
        print('Not supported type, exit')
        exit(1)

    if not path.exists(in_file_path):
        print('Input file directory is invalid, exit')
        exit(1)

    # Make a `out_files` folder
    if not path.exists(out_file_path):
        mkdir(out_file_path)

    cmd = '''nrfutil pkg generate
                        --application "{new_app}"
                        --application-version 1
                        --hw-version 52
                        --sd-req {sd_req}
                        --key-file "{priv_key}"
                        "{nordic_pkg}"
    '''.format(new_app = app_file_path,
            sd_req = sd_req_num,
            priv_key = private_key_file_path,
            nordic_pkg = nordic_package_file_path)

    run_cmd(cmd)

    if not path.exists(nordic_package_file_path):
        print('error. Nordic package file is not generated')
        exit(1)       

    dfu_bin = DfuFlatFile()

    dfu_bin.make("./out_files/nordic_pkg.zip", "./out_files/firmware.bin")

    if path.exists(firmware_file_path):
        print('New firmware is generated: {}'.format(path.abspath(firmware_file_path)))
        print('\n')
    else:
        print('Not supported type, exit')
        exit(1)
        
    exit(0)
