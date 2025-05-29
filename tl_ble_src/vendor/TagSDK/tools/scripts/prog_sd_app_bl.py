"""
Description:
    Perform general DFU programming as below:

    1. Genrate bootloader settings
    2. Merge bootloader hex and bootloader settings hex
    3. Erase
    4. Program softdevice
    5. Program bootloader merged hex
    6. Program application
    7. Reset
"""

from os import path
from os import mkdir
import subprocess
import shlex
from lib_dk_snr import DK_SNR


dk_snr = DK_SNR
print('------------------------------')
print('1 : nRF52832_pca10040_s113');
print('2 : nRF52832_pca10040_s132');
print('3 : nRF52833_pca10100_s113');
print('------------------------------')
ic_num = input('Input your IC part number idx: ')

BOOTLOADER_PROJECT = 'BLE_BL_NO_LOG'
USE_GCC = False

# Folders
hex_file_dir = path.relpath('./in_files')
out_file_dir = path.relpath('./out_files')

DEV_FAMILY = 'NRF52'

# Input files
if ic_num == '1' :
    TARGET_BOARD = 'pca10040'    
    TARGET_IC = 'nrf52832'
    TARGET_SD = 's113'
    bootloader_file_path = '../../../../examples/dfu/secure_bootloader/pca10040_s113_ble/ses/Output/Release/Exe/secure_bootloader_ble_s113_pca10040.hex'
    application_file_path = '../../example/nrf/pca10040/s113/ses/Output/Release/Exe/tag_example_pca10040_s113.hex'
    softdevice_file_name = 's113_nrf52_7.2.0_softdevice.hex'
elif ic_num == '2' :
    TARGET_BOARD = 'pca10040'
    TARGET_IC = 'nrf52832'
    TARGET_SD = 's132'
    bootloader_file_path = '../../../../examples/dfu/secure_bootloader/pca10040_s132_ble/ses/Output/Release/Exe/secure_bootloader_ble_s132_pca10040.hex'
    application_file_path = '../../example/nrf/pca10040/s132/ses/Output/Release/Exe/tag_example_pca10040_s132.hex'
    softdevice_file_name = 's132_nrf52_7.2.0_softdevice.hex'
elif ic_num == '3' :
    TARGET_BOARD = 'pca10100'    
    TARGET_IC = 'nrf52833'
    TARGET_SD = 's113'
    bootloader_file_path = '../../../../examples/dfu/secure_bootloader/pca10100_s113_ble/ses/Output/Release/Exe/secure_bootloader_ble_s113_pca10100.hex'
    application_file_path = '../../example/nrf/pca10100/s113/ses/Output/Release/Exe/tag_example_pca10100_s113.hex'
    softdevice_file_name = 's113_nrf52_7.2.0_softdevice.hex'
else :
    exit(1)

print('device snr       :', dk_snr)
print('target board     :', TARGET_BOARD)
print('target IC        :', TARGET_IC)
print('Softdevice ver.  :', softdevice_file_name)
print('\n')
    
private_key_name = 'private_key.pem'

# Output files
bl_settings_file_name = 'bl_settings.hex'
bl_merged_file_name = 'bl_merged.hex'

softdevice_file_path = path.join(hex_file_dir, softdevice_file_name)
private_key_path = path.join(hex_file_dir, private_key_name)

bl_settings_file_path = path.join(out_file_dir, bl_settings_file_name)
bl_merged_file_path = path.join(out_file_dir, bl_merged_file_name)

def run_command(_command_line):

    args = shlex.split(_command_line)
    cmd_process = subprocess.Popen(args,
        stdout = subprocess.PIPE,
        stderr = subprocess.PIPE,
        shell = True,
        universal_newlines = True)
    msg, err = cmd_process.communicate()

    if not err == '':
        print('')
        print('(error): {}'.format(err))
        exit(1)
    else:
        print('(done)')


if __name__ == '__main__':
    print('Start...')

    if not path.exists(hex_file_dir):
        print('hex file director is not existed, exit')
        exit(1)

    if not path.exists(out_file_dir):
        mkdir(out_file_dir)

    print('Generate bootloader settings...')
    command_line = '''nrfutil settings generate
                        --family {family}
                        --application "{application_file}"
                        --application-version 1
                        --bootloader-version 1
                        --bl-settings-version 2
                        "{bl_settings}"
    '''.format(family = DEV_FAMILY,
        application_file = application_file_path,
        bl_settings = bl_settings_file_path)

    run_command(command_line)

    print('Merge bootloader and settings...')
    command_line = '''mergehex -m "{bl}" "{bl_settings}" -o "{bl_merged}"
    '''.format(bl = bootloader_file_path,
        bl_settings = bl_settings_file_path,
        bl_merged = bl_merged_file_path)

    run_command(command_line)

    print('Erase...')
    command_line = '''nrfjprog -s {snr} -e
    '''.format(snr = dk_snr)

    run_command(command_line)

    print('Program softdevice...')
    command_line = '''nrfjprog -s {snr} --program "{hex}" --verify
    '''.format(snr = dk_snr, hex = softdevice_file_path)

    run_command(command_line)

    print('Program merged bootloader file...')
    command_line = '''nrfjprog -s {snr} --program "{hex}" --verify
    '''.format(snr = dk_snr, hex = bl_merged_file_path)

    run_command(command_line)

    print('Program application file...')
    command_line = '''nrfjprog -s {snr} --program "{hex}" --sectorerase --verify
    '''.format(snr = dk_snr, hex = application_file_path)

    run_command(command_line)

    print('Reset...')
    command_line = '''nrfjprog -s {snr} -r
    '''.format(snr = dk_snr)

    run_command(command_line)

    print('Bye :-)')
