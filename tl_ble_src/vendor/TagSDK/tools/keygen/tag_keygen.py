#!/usr/bin/env python3

import os
import argparse
import base64
import binascii
import csv
import random
import string

from datetime import datetime
from nacl.signing import SigningKey
from nacl._sodium import ffi, lib

import pkgutil

from qrgen import Qr, HeaderParser

__version__ = "1.0.0"
ROOT_PATH = "output"

def ed_to_curve_seckey(seckey):
    curve_seckey_c = ffi.new("unsigned char[]", 32)
    lib.crypto_sign_ed25519_sk_to_curve25519(curve_seckey_c, seckey)
    return ffi.buffer(curve_seckey_c)[:]

class Ed25519Key():
    def __init__(self, sn):
        if not isinstance(sn, str):
            raise TypeError("SerialNumber is not a string")

        self._sn = sn

    def generate(self):
        # Generate Ed25519 keypair
        seckey = SigningKey.generate()
        pubkey = seckey.verify_key

        self._seckey_hex = seckey.encode()
        self._pubkey_hex = pubkey.encode()

        self._ed25519_seckey_hex_b64 = base64.b64encode(self._seckey_hex)
        self._ed25519_pubkey_hex_b64 = base64.b64encode(self._pubkey_hex)
        self._curve25519_seckey_hex_b64 = base64.b64encode(ed_to_curve_seckey(self._seckey_hex)[:])

    def get_pubkey_b64(self):
        return self._ed25519_pubkey_hex_b64

    def get_seckey_b64(self):
        return self._curve25519_seckey_hex_b64

    def _do_save(self, sub_path, fname, key_hex_b64):
        path = os.path.join(sub_path, fname)

        if os.path.exists(path):
            raise TypeError("WARNING: '%s' is already generated" % path)

        if not os.path.isdir(sub_path):
            os.makedirs(sub_path, exist_ok=True)

        with open(path, "wb") as fp:
            fp.write(key_hex_b64)

    def savetofile(self, sub_path):
        self._do_save(sub_path, "device.ed25519_pubkey.b64", self._ed25519_pubkey_hex_b64)
        self._do_save(sub_path, "device.ed25519_seckey.b64", self._ed25519_seckey_hex_b64)
        self._do_save(sub_path, "device.curve25519_seckey.b64", self._curve25519_seckey_hex_b64)

    def dump(self):
        print("Dump Ed25519 keypair,")
        print("seckey      :", binascii.hexlify(self._seckey_hex))
        print("pubkey      :", binascii.hexlify(self._pubkey_hex))
        print("ed25519 pubkey(b64) :", self._ed25519_pubkey_hex_b64)
        print("ed25519 seckey(b64) :", self._ed25519_seckey_hex_b64)
        print("curve25519 seckey(b64) :", self._curve25519_seckey_hex_b64)

class Batch():
    def __init__(self, time, output):
        if not isinstance(time, str):
            raise TypeError("time is not a string")

        self._time = time
        self._batch_output = output

    def generate_csv(self, sub_path, items, factory_csv):

        if factory_csv is True:
            path = "Factory_Data_Sample.csv"
                
            with open(path, 'w', newline='') as csvfile:
                fieldnames = ['sn', 'keyType', 'keyCrv', 'seckey', 'pubkey']
                write = csv.DictWriter(csvfile, fieldnames=fieldnames)
                write.writeheader()
                write.writerows(items)
            print("Factory Data has been saved: " + path)
        else:
            if self._batch_output is None:
                batch_file = "DI_Batch_" + self._time + ".csv"
                path = os.path.join(sub_path, batch_file)
            else:
                path = self._batch_output

            if os.path.exists(path):
                raise TypeError("WARNING: '%s' is already registered" % path)

            dir = os.path.dirname(path)

            if os.path.isdir(dir):
                os.makedirs(dir, exist_ok=True)
                
            with open(path, 'w', newline='') as csvfile:
                fieldnames = ['sn', 'keyType', 'keyCrv', 'pubkey']
                write = csv.DictWriter(csvfile, fieldnames=fieldnames)
                write.writeheader()
                # Remove seckey
                for item in items:
                    del item['seckey']
                write.writerows(items)
                
            print("Batch file has been saved: " + path)

class Formatter():
    def __init__(self, sn, pubkey_b64, seckey_b64):
        self._sn = sn
        self._pubkey_b64 = pubkey_b64
        self._seckey_b64 = seckey_b64
    def print_to_console(self):
        print("Use following serial number and public key")
        print("for the identity of your device in Developer Workspace.\n")
        print("Serial Number:")
        print(self._sn)
        print("")
        print("Public Key:")
        print(self._pubkey_b64)
        print("")

    def generate_device_info_header(self, sub_path):
        path = os.path.join(sub_path, "TagDeviceInfo.h")
        header_define = "TAGSDK_TAGDEVICEINFO_H_"

        with open(path, "w") as device_info:
            device_info.write('#ifndef ' + header_define + '\n')
            device_info.write('#define ' + header_define + '\n')
            device_info.write('\n')
            device_info.write('#include "TagConfig.h"\n')
            device_info.write('\n')
            device_info.write('#if defined(TAG_CONFIG_USE_DEVICE_INFO_HEADER)\n')
            device_info.write('/* Private key */\n')
            device_info.write('const char *conf_device_seckey_curve25519 = "' + self._seckey_b64 + '";\n')
            device_info.write('\n')
            device_info.write('/* Public key */\n')
            device_info.write('const char *conf_device_pubkey_ed25519 = "' + self._pubkey_b64 + '";\n')
            device_info.write('\n')
            device_info.write('/* Serial number */\n')
            device_info.write('const char *conf_device_serial_number = "' + self._sn + '";\n')
            device_info.write('#endif /* TAG_CONFIG_USE_DEVICE_INFO_HEADER */\n')
            device_info.write('\n')
            device_info.write('#endif /* ' + header_define + ' */\n')



def get_random(mnid, length):
    random_string = ''
    pool = string.ascii_letters + string.digits
    if mnid is None:
        length += 4
    elif len(mnid) != 4:
        raise TypeError("ERROR: MNID length is not 4")
    else:
        random_string = mnid

    random_string += ''.join(random.choice(pool) for i in range(length))
    return random_string

def individual(args, qrcode_imported):
    root_path = ROOT_PATH
    sn = 'STDK' + get_random(args.mnid, 8)
    sub_path = root_path + "_" + sn

    edkey = Ed25519Key(sn)
    edkey.generate()
    edkey.savetofile(sub_path)

    formatter = Formatter(sn, edkey.get_pubkey_b64().decode('utf-8'), edkey.get_seckey_b64().decode('utf-8'))
    formatter.print_to_console()
    formatter.generate_device_info_header(sub_path)

    if args.qr is True and qrcode_imported is not None:
        if args.config_path is not None:
            device_info_path = os.path.join(sub_path, "TagDeviceInfo.h")
            qr = Qr()
            qr.generate_using_header(os.path.join(sub_path, sn), device_info_path, args.config_path)
        else:
            print("Failed to generate QR image : --config_path is required.")
            return

def bulk(args, qrcode_imported):
    root_path = ROOT_PATH + "_bulk"
    time = datetime.now().strftime("%Y%m%d_%H%M%S")
    sub_path = os.path.join(root_path, time)
    batch_items = []

    if not os.path.exists(args.input):
        raise TypeError("not found '%s'" % args.input)

    print("Loading " + args.input + "...")

    if args.qr is True and qrcode_imported is not None:
        mnid = args.mnid

    with open(args.input, newline='') as csvinput:
        reader = csv.DictReader(csvinput)
        for row in reader:
            # Generate csv rows in 'csv_items' for batch csv file
            batch_item = dict()
            sn = row['sn']
            if len(sn) < 11:
                print("Error : SN '" + sn + "' is too short. SN must be at least 11 characters.")
                return

            edkey = Ed25519Key(sn)
            edkey.generate()
            edkey.savetofile(os.path.join(sub_path, sn))
            batch_item['sn'] = sn
            batch_item['keyType'] = 'ECPUBKEY'
            batch_item['keyCrv'] = 'ED25519'
            batch_item['pubkey'] = edkey.get_pubkey_b64().decode('utf-8')
            batch_item['seckey'] = edkey.get_seckey_b64().decode('utf-8')
            batch_items.append(batch_item)            

            formatter = Formatter(sn, edkey.get_pubkey_b64().decode('utf-8'), edkey.get_seckey_b64().decode('utf-8'))
            formatter.generate_device_info_header(sub_path)
            #print(csv_item.items())

            if args.qr is True and qrcode_imported is not None:
                if args.config_path is not None:
                    device_info_path = os.path.join(sub_path, "TagDeviceInfo.h")
                    qr = Qr()
                    qr.generate_using_header(sub_path, device_info_path, args.config_path)
                else:
                    print("Failed to generate QR image : --config_path is required.")
                    return

    batch = Batch(time, args.output)
    batch.generate_csv(sub_path, batch_items, True)
    batch.generate_csv(sub_path, batch_items, False)

def version(args):
    print(__version__)

def main():
    parser = argparse.ArgumentParser(description="%s v%s - SmartThings Tag SDK keygen tools" %(__file__, __version__))
    parser.add_argument(
        '--mnid',
        help="[I] can set your MNID as prefix of Serial Number")

    parser.add_argument(
        '--input',
        metavar='csv',
        help="[C] input csv file that has a list of serial numbers")

    parser.add_argument(
        '--output',
        metavar='csv',
        help="[C] output batch csv file")

    parser.add_argument(
        '--config_path',
        help="path of TagOnboardingConfig.h to generate QR code image"
    )

    parser.add_argument(
        '--qr',
        action='store_true',
        default=False,
        help="generate QR code image. --config_path is required")

    args = parser.parse_args()

    if args.qr is True:
        try:
            import qrcode
        except ImportError:
            print("-----------------------------NOTICE----------------------------------")
            print("qrcode module failed to import. QR code generation will not execute.")
            print("If you want to make QR code, please install qrcode module first.")
            print("---------------------------------------------------------------------")
            pass
    qrcode_imported = pkgutil.find_loader('qrcode')

    if args.config_path is not None:
        if os.path.isdir(args.config_path):
            args.config_path = os.path.join(args.config_path, "TagOnboardingConfig.h")

        if args.mnid is None:
            headerParser = HeaderParser()
            headerParser.parse_onboardingConfig(args.config_path)
            args.mnid = headerParser._mnid


    if args.input is None:
        individual(args, qrcode_imported)
    else:
        bulk(args, qrcode_imported)

if __name__ == "__main__":
    main()
