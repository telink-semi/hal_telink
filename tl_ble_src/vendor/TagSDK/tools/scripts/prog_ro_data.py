"""
Description:
    Sample production programming as below:

    1. Read ro data from batch file.
    2. Program ro data (s/n, key)
"""

from os import path
from os import mkdir
import subprocess
import shlex
import csv
import binascii
import base64
from lib_dk_snr import DK_SNR

dk_snr = DK_SNR

USE_GCC = False

# Output file
hex_file_dir = path.relpath('./')
# Input files
batch_file_name = 'Factory_Data_Sample.csv'
batch_file_path = path.join(hex_file_dir, batch_file_name)
onboard_conf_file_name = 'TagOnboardingConfig.h'
onboard_conf_file_path = path.join(hex_file_dir, onboard_conf_file_name)

# Ro partition address of nRF52832/nRF52833
ro_start_addr = '0x6e000'
ro_end_addr = '0x6efff'
# Offsets
model_name_offset = 0
vendor_id_offset = 32
manufacturer_id_offset = 72
manufacturer_name_offset = 76
setup_id_offset = 108
serial_start_offset = 112
seckey_start_offset = 148
batch_items = []
# Sizes
model_name_size = 32
vendor_id_size = 40
manufacturer_id_size = 4
manufacturer_name_size = 32
setup_id_size = 4


class HeaderParser():
    def __init__(self):
        self._setupid = ''
        self._modelname = ''
        self._vid = ''
        self._mnid = ''
        self._mnmn = ''

    def parse_onboardingConfig(self, path):
        try:
            file = open(path, 'r', encoding='utf8')
        except:
            print("Failed to open onboardingConfig : ", path)
            exit(1)

        while True:
            try:
                line = file.readline()
            except:
                print("Failed to read onboardingConfig : ", path)
                exit(1)

            if not line:
                break
            elif 'conf_setupId' in line:
                self._setupid = line.split('"')[1]
            elif 'conf_modelName' in line:
                self._modelname = line.split('"')[1]
            elif 'conf_vid' in line:
                self._vid = line.split('"')[1]
            elif 'conf_mnId' in line:
                self._mnid = line.split('"')[1]
            elif 'conf_mnmn' in line:
                self._mnmn = line.split('"')[1]

        if not self._setupid:
            print('SETUP ID has not been set in ' + path)
            exit(1)
        if not self._modelname:
            print('MODEL NAME has not been set in ' + path)
            exit(1)
        if not self._vid:
            print('VID has not been set in ' + path)
            exit(1)
        if not self._mnid:
            print('MNID has not been set in ' + path)
            exit(1)
        if not self._mnmn:
            print('MNMN has not been set in ' + path)
            exit(1)


def run_command(_command_line, type):
        
    # print(_command_line)
    args = shlex.split(_command_line)
    cmd_process = subprocess.Popen(args,
        stdout = subprocess.PIPE,
        stderr = subprocess.PIPE,
        universal_newlines = True)
    msg, err = cmd_process.communicate()

    if not err == '':
        if type == '1' :
            return
        else:
            print('(error): {}'.format(err))
            exit(1) 

def bulk():

    print("Loading ")
    
    index = 0

    with open(batch_file_path, newline='') as csvinput:
        reader = csv.DictReader(csvinput)
        for row in reader:
            # Generate csv rows in 'csv_items' for batch csv file
            batch_item = dict()
            sn = row['sn']
            if len(sn) < 11:
                print("Error : SN '" + sn + "' is too short. SN must be at least 11 characters.")
                return

            seckey = row['seckey']

            batch_item['sn'] = sn
            batch_item['seckey'] = seckey
            batch_items.append(batch_item)
            print('{num}. :'.format(num = index), sn)
            index += 1
            
if __name__ == '__main__':

    print('Start...')
    
    if not path.exists(hex_file_dir):
        print('Hex file directory is not existed, exit')
        exit(1)

    headerParser = HeaderParser()
    headerParser.parse_onboardingConfig(onboard_conf_file_path)

    setupid = headerParser._setupid
    modelname = headerParser._modelname
    vid = headerParser._vid
    mnid =  headerParser._mnid
    mnmn = headerParser._mnmn

    bulk()        

    batch_item = dict()    

    if len(batch_items) < 1:
        print("Error : Cannot find item")
        exit(1)

    print('')
    sl_num = int(input('Select item to program secure data: '))
    print('')
    
    if sl_num >= len(batch_items):
        print("Error : invalid index")
        exit(1)

    #erase page        
    print('Erase...')
    command_line = '''nrfjprog -s {snr} --erasepage {startAddr}-{endAddr}
    '''.format(snr = dk_snr, startAddr = ro_start_addr, endAddr = ro_end_addr)
    run_command(command_line, '1')
    print('(done)')

    #Program Model Name
    print('Program Model Name... :', modelname)
    flashAddr = int(ro_start_addr, base = 16) + model_name_offset
    modelname_data = modelname.encode('utf8')
    if len(modelname_data) < model_name_size:
        modelname_data += b'\x00' * (model_name_size - len(modelname_data))
    modelname_hex_data = modelname_data.hex()
    modelname_list = [modelname_hex_data[i:i+8] for i in range(0, len(modelname_hex_data), 8)]
    for i in range(len(modelname_list)):
        hex_str = modelname_list[i]
        hex_str_lsb = ''.join([hex_str[i-2:i] for i in range(len(hex_str), 0, -2)])
        command_line = '''nrfjprog -s {snr} --memwr {addr} --val 0x{data}
        '''.format(snr = dk_snr, addr = hex(flashAddr), data = hex_str_lsb)
        run_command(command_line, '0')
        flashAddr += 4
    print('(done)')

    #Program Vendor ID
    print('Program Vendor ID... :', vid)
    flashAddr = int(ro_start_addr, base = 16) + vendor_id_offset
    vid_data = vid.encode('utf8')
    if len(vid_data) < model_name_size:
        vid_data += b'\x00' * (vendor_id_size - len(vid_data))
    vid_hex_data = vid_data.hex()
    vid_list = [vid_hex_data[i:i+8] for i in range(0, len(vid_hex_data), 8)]
    for i in range(len(vid_list)):
        hex_str = vid_list[i]
        hex_str_lsb = ''.join([hex_str[i-2:i] for i in range(len(hex_str), 0, -2)])
        command_line = '''nrfjprog -s {snr} --memwr {addr} --val 0x{data}
        '''.format(snr = dk_snr, addr = hex(flashAddr), data = hex_str_lsb)
        run_command(command_line, '0')
        flashAddr += 4
    print('(done)')

    #Program Manufacturer ID
    print('Program Manufacturer ID... :', mnid)
    flashAddr = int(ro_start_addr, base = 16) + manufacturer_id_offset
    mnid_data = mnid.encode('utf8')
    if len(mnid_data) < model_name_size:
        mnid_data += b'\x00' * (manufacturer_id_size - len(mnid_data))
    mnid_hex_data = mnid_data.hex()
    mnid_list = [mnid_hex_data[i:i+8] for i in range(0, len(mnid_hex_data), 8)]
    for i in range(len(mnid_list)):
        hex_str = mnid_list[i]
        hex_str_lsb = ''.join([hex_str[i-2:i] for i in range(len(hex_str), 0, -2)])
        command_line = '''nrfjprog -s {snr} --memwr {addr} --val 0x{data}
        '''.format(snr = dk_snr, addr = hex(flashAddr), data = hex_str_lsb)
        run_command(command_line, '0')
        flashAddr += 4
    print('(done)')

    #Program Manufacturer Name
    print('Program Manufacturer Name... :', mnmn)
    flashAddr = int(ro_start_addr, base = 16) + manufacturer_name_offset
    mnmn_data = mnmn.encode('utf8')
    if len(mnmn_data) < model_name_size:
        mnmn_data += b'\x00' * (manufacturer_name_size - len(mnmn_data))
    mnmn_hex_data = mnmn_data.hex()
    mnmn_list = [mnmn_hex_data[i:i+8] for i in range(0, len(mnmn_hex_data), 8)]
    for i in range(len(mnmn_list)):
        hex_str = mnmn_list[i]
        hex_str_lsb = ''.join([hex_str[i-2:i] for i in range(len(hex_str), 0, -2)])
        command_line = '''nrfjprog -s {snr} --memwr {addr} --val 0x{data}
        '''.format( snr = dk_snr, addr = hex(flashAddr), data = hex_str_lsb)
        run_command(command_line, '0')
        flashAddr += 4
    print('(done)')

    #Program Setup ID
    print('Program Setup ID... :', setupid)
    flashAddr = int(ro_start_addr, base = 16) + setup_id_offset
    setupid_data = setupid.encode('utf8')
    if len(setupid_data) < model_name_size:
        setupid_data += b'\x00' * (setup_id_size - len(setupid_data))
    setupid_hex_data = setupid_data.hex()
    setupid_list = [setupid_hex_data[i:i+8] for i in range(0, len(setupid_hex_data), 8)]
    for i in range(len(setupid_list)):
        hex_str = setupid_list[i]
        hex_str_lsb = ''.join([hex_str[i-2:i] for i in range(len(hex_str), 0, -2)])
        command_line = '''nrfjprog -s {snr} --memwr {addr} --val 0x{data}
        '''.format(snr = dk_snr, addr = hex(flashAddr), data = hex_str_lsb)
        run_command(command_line, '0')
        flashAddr += 4
    print('(done)')

    #binary to hex
    batch_item = batch_items[sl_num]

    sn_data = batch_item['sn'].encode('utf8')
    sn_hex_data = sn_data.hex()
    sn_list = [sn_hex_data[i:i+8] for i in range(0,len(sn_hex_data), 8)]
    #print(sn_list)

    seckey_data = batch_item['seckey'].encode('utf8')
    seckey_hex_data = seckey_data.hex()
    seckey_list = [seckey_hex_data[i:i+8] for i in range(0,len(seckey_hex_data), 8)]
    #print(seckey_list)

    #Program serial number len
    print('Program serial number len... :', len(sn_data))

    flashAddr = int(ro_start_addr, base = 16) + serial_start_offset
    hex_sn_data_len = '{:08x}'.format(len(sn_data))    
    hex_sn_data_len_lsb = ''.join([hex_sn_data_len[i-2:i] for i in range(len(hex_sn_data_len), 0, -2)])
    
    command_line = '''nrfjprog -s {snr} --memwr {addr} --val 0x{data}
    '''.format(snr = dk_snr, addr = hex(flashAddr), data = hex_sn_data_len_lsb)
    #print(command_line)
    run_command(command_line, '0')
    flashAddr += 4

    print('(done)')
    
    #Program serial number
    print('Program s/n... :', sn_data)
    
    for i in range(len(sn_list)):
    
        hex_str = sn_list[i]
        hex_str_lsb = ''.join([hex_str[i-2:i] for i in range(len(hex_str), 0, -2)])
        hex_len = len(hex_str)

        command_line = '''nrfjprog -s {snr} --memwr {addr} --val 0x{data}
        '''.format(snr = dk_snr, addr = hex(flashAddr), data = hex_str_lsb)
        #print(command_line)
        run_command(command_line, '0')
        flashAddr += 4

    print('(done)')

    flashAddr = int(ro_start_addr, base = 16) + seckey_start_offset
    
    #Program seckey len
    print('Program seckey len... :', len(seckey_data))

    hex_seckey_data_len = '{:08x}'.format(len(seckey_data))
    hex_seckey_data_len_lsb = ''.join([hex_seckey_data_len[i-2:i] for i in range(len(hex_seckey_data_len), 0, -2)])

    command_line = '''nrfjprog -s {snr} --memwr {addr} --val 0x{data}
    '''.format(snr = dk_snr, addr = hex(flashAddr), data = hex_seckey_data_len_lsb)
    #print(command_line)
    run_command(command_line, '0')
    flashAddr += 4

    print('(done)')
    
    #Program seckey
    print('Program seckey...', seckey_data)
    
    for i in range(len(seckey_list)):
        
        hex_str = seckey_list[i]
        hex_str_lsb = ''.join([hex_str[i-2:i] for i in range(len(hex_str), 0, -2)])

        command_line = '''nrfjprog -s {snr} --memwr {addr} --val 0x{data}
        '''.format(snr = dk_snr, addr = hex(flashAddr), data = hex_str_lsb)
        #print(command_line)
        run_command(command_line, '0')
        flashAddr += 4

    print('(done)')
    
    print('Reset...')
    command_line = '''nrfjprog -s {snr} -r
    '''.format(snr = dk_snr)

    run_command(command_line, '0')

    print('Bye :-)')    
