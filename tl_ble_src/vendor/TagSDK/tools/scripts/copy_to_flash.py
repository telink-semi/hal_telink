"""
Description: Copy the DFU file to the flash of BLE central
"""
import sys
import re
from os import stat
from os import path
from os import mkdir
from lib_run_cmd import run_cmd
from lib_dk_snr import DK_SNR


dk_snr = DK_SNR

# Base paths
out_file_path = path.relpath('./out_files')

# File names
dfu_bin_to_hex_name = 'dfu_bin_to_hex.hex'

# File paths
dfu_bin_to_hex_path = path.join(out_file_path, dfu_bin_to_hex_name)

UICR_CUSTOMER_0_ADDR = 0x10001080

if __name__ == '__main__':

    if len(sys.argv) != 2:
        print('usage: python copy_to_flash.py <bin-file>')
        exit(1)

    bin_file = sys.argv[1]


    # Read bank 1 start address from NRF_UICR->CUSTOMER[0]
    print('Read bank 1 start address from  UICR customer[0]')
    cmd = '''nrfjprog -s {snr} --memrd {addr} --n {size}
    '''.format(snr = dk_snr, addr = UICR_CUSTOMER_0_ADDR, size = 4)

    message = run_cmd(cmd)
    regex = re.match(r'0x[0-9a-fA-F]{8}: ([0-9a-fA-F]{8})', message)
    address = '0x' + regex.group(1)
    address_offset = int(address, 16)
    print('Bank 1 start address is 0x{:08x}'.format(address_offset))

    print('Convert DFU bin to hex...')
    cmd = '''python bin_to_hex.py --offset={offset} "{bin_file}" "{hex_file}"
    '''.format(offset = address_offset, bin_file = bin_file, hex_file = dfu_bin_to_hex_path)

    run_cmd(cmd)

    if path.exists(dfu_bin_to_hex_path):
        print('DFU bin_to_hex file is generated:\n{}'.format(path.abspath(dfu_bin_to_hex_path)))

    print('Write hex to flash(address: 0x{:x})...'.format(address_offset))
    cmd = '''nrfjprog -s {snr} --program "{hex_file}" --sectorerase
    '''.format(snr = dk_snr, hex_file = dfu_bin_to_hex_path)

    run_cmd(cmd)

    print('Reset...')
    cmd = '''nrfjprog -s {snr} -r
    '''.format(snr = dk_snr)

    run_cmd(cmd)

