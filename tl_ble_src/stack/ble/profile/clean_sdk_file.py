# -*- coding: utf-8 -*-

import os

common_file_path = "source-code/B91_ble_multi_conn_src"

c_file_paths = [
    '/algorithm/crypto',
    '/algorithm/ecc',
    '/stack/ble/bqb',
    '/stack/ble/controller',
    '/stack/ble/debug',
    '/stack/ble/device',
    '/stack/ble/hci',
    '/stack/ble/host',
    '/stack/ble/os_sup',
    '/stack/ble/service',
    '/stack/ble/ble.c',
    '/stack/ble/profile/audio',
    '/drivers/B91/ext_driver/ext_misc.c',
    '/drivers/B91/ext_driver/dbgport.c',
    '/drivers/B91/ext_driver/ext_ase.c',
    '/drivers/B91/ext_driver/ext_gpio.c',
    '/drivers/B91/ext_driver/ext_misc.c',
    '/drivers/B91/ext_driver/ext_pm.c',
    '/drivers/B91/ext_driver/ext_rf.c',
    '/drivers/B91/lib/src',
]

h_file_paths = [
    '/stack',
]

def get_all_file(paths, suffix='.c'):
    if os.path.isdir(paths):
        for root, ds, fs in os.walk(paths):
            for f in fs:
                # print("find file is ", f)
                if "buf.c" in f:
                    continue
                if "test_prf" in f:
                    continue
                if "aud_cfg_tbl" in f:
                    continue
                if "cap.c" in f:
                    continue
                if os.path.join(root, f).endswith(suffix):
                    yield os.path.join(root, f)
    else:
        yield paths

if __name__ == "__main__":
    # common_file_path = input("Please input the files path:")  # ����·��
    common_file_path = "E:\gitlab\\telink_b91m_ble_audio_sdk\B91m_ble_sdk"
    print("clean all file")
    for path in c_file_paths:
        new_path = common_file_path + path
        # print("new path is ", new_path)
        for file in get_all_file(new_path):
            # print(file)
            if os.path.exists(file):
                os.remove(file)
    
    for path in h_file_paths:
        new_path = common_file_path + path
        for file in get_all_file(new_path, suffix='stack.h'):
            # print(file)
            os.remove(file)
        for file in get_all_file(new_path, suffix='internal.h'):
            # print(file)
            os.remove(file)