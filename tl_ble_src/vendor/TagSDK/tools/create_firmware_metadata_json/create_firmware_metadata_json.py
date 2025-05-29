#!/usr/bin/python3

import os
import sys
import datetime
import zipfile

CONF_PATH = sys.path[0] +'/../../conf/'
DEV_INFO_NAME = 'TagDeviceInfo.h'
ONB_CFG_NAME  = 'TagOnboardingConfig.h'
ACC_OPT_NAME  = 'TagAccessoryOption.h'
FW_VERSION_NAME = 'TagFwVersion.h'
SPEC_VERSION_NAME = 'TagVersion.h'

INC_PATH = sys.path[0] +'/../../inc/'
VERSION_NAME  = 'TagVersion.h'

fields = \
    "serial Number,MNID,modelName,onboardingId,partner,sku,vendor,state,type," \
    "productId,key.type,crv,key,macAddress,buttonAction,ringTheTag,updateRingtone," \
    "batteryType,leftBehindAlert,firmwareUpdate,ledBlinking,powerSaving,nfc"
dev = {
        'SN':'',
        'MNID':'',
        'NAME':'',
        'ONB_ID':'',
        'PARTNER':'',
        'SKU':'',
        'VENDOR':'',
        'STATE':'PROVISIONED',
        'TYPE':'TEST',
        'PROD_ID':'',
        'KEY_TYPE':'ECPUBKEY',
        'CRV':'ED25519',
        'KEY':'',
        'MAC':'',
        'BUTTON_ACTION':'',
        'RING_THE_TAG':'',
        'UPDATE_RINGTONE':'',
        'BATTERY_TYPE':'',
        'LEFT_BEHIND_ALERT':'',
        'FIRMWARE_UPDATE':'',
        'CATEGORY':'',
        'SDK_VERSION':'',
        'SPEC_VERSION':'',
        'FW_VERSION':'',
        'VID':'',
        'MNMN':'',
        'LED_BLINKING':'',
        'POWER_SAVING_MODE':'',
        'NFC':'',
}

def check_valid(f, name, pattern):
    f.seek(0, 0)
    for line in f:
        if line.find(pattern) >= 0:
            print("\n[WARNING] Please create a valid %s file" % (name))
            f.close()
            exit()

def search_data(f, pattern):
    f.seek(0, 0)
    for line in f:
        pos = line.find(pattern)
        if pos >= 0:
            start = pos + len(pattern + ' = "')
            end = line.find('"',start)
            #print("    -Find `%s` : %s" % (pattern, line[start:end]))
            return line[start : end]

    print("DO NOT find %s, please check!" % (pattern))
    f.close()
    exit()

def search_option(f, pattern):
    f.seek(0, 0)
    for line in f:
        pos = line.find(pattern)
        if pos >= 0:
            start = pos + len(pattern + ' (')
            end = line.find(')',start)
            #print("    -Find `%s` : %s" % (pattern, line[start:end]))
            return line[start : end]

    print("DO NOT find %s, please check!" % (pattern))
    f.close()
    exit()

def search_pattern(f, pattern):
    f.seek(0, 0)
    for line in f:
        pos = line.find(pattern)
        if pos >= 0:
            start = pos + len(pattern)
            end = len(line)
            #print("    -Find `%s` : %s" % (pattern, line[start:end]))
            return line[start : end]

    print("DO NOT find %s, please check!" % (pattern))
    f.close()
    exit()

def get_date_from_device_info():
    global dev
    f = open(CONF_PATH+DEV_INFO_NAME, 'r')

    #check_valid(f, DEV_INFO_NAME, '_here')
    #print("\n * Find valid %s file" % (DEV_INFO_NAME))

    dev['SN']  = search_data(f, 'conf_device_serial_number')
    dev['KEY'] = search_data(f, 'conf_device_pubkey_ed25519')
    f.close()

def is_exist(f, name, pattern):
    f.seek(0, 0)
    for line in f:
        if line.find(pattern) >= 0:
            print("Cannot find data %s in %s file" % (pattern, name))
            return 0
    return 1

def get_data_from_accessory_option():
    global dev

    f = open(CONF_PATH+ACC_OPT_NAME, 'r')
    dev['BUTTON_ACTION']     = search_option(f, 'TAG_ACCESSORY_OPTION_BUTTON_ACTION')
    dev['RING_THE_TAG']      = search_option(f, 'TAG_ACCESSORY_OPTION_RING_THE_TAG')
    dev['UPDATE_RINGTONE']   = search_option(f, 'TAG_ACCESSORY_OPTION_UPDATE_RINGTONE')
    dev['LEFT_BEHIND_ALERT'] = search_option(f, 'TAG_ACCESSORY_OPTION_LEFT_BEHIND_ALERT')
    dev['FIRMWARE_UPDATE']   = search_option(f, 'TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE')
    dev['LED_BLINKING']      = search_option(f, 'TAG_ACCESSORY_OPTION_LED_BLINKING')
    dev['POWER_SAVING_MODE'] = search_option(f, 'TAG_ACCESSORY_OPTION_POWER_SAVING_MODE')
    dev['NFC']               = search_option(f, 'TAG_ACCESSORY_OPTION_LOST_MESSAGE')
    newStr                   = search_pattern(f, 'TAG_ACCESSORY_OPTION_BATTERY_TYPE')
    newStr = newStr.strip()
    dev['BATTERY_TYPE']       = newStr.strip('"')
    f.close()

def get_version_info_from_file():
    global dev

    f = open(INC_PATH+VERSION_NAME, 'r')
    temp = search_option(f, 'TAGSDK_VERSION_STRING')
    temp = temp.strip("_STAG_VERSION_STR(")
    temp = temp.replace(", ", ".")
    dev['SDK_VERSION'] = temp

    temp = search_option(f, 'SPEC_VERSION_STRING')
    temp = temp.strip("_STAG_VERSION_STR(")
    temp = temp.replace(", ", ".")

    dev['SPEC_VERSION'] = temp
    f.close()

    #TAG_FWVER_MAJOR, TAG_FWVER_MINOR, TAG_FWVER_PATCH

def get_fw_version_info_from_file():
    global dev

    f = open(CONF_PATH + FW_VERSION_NAME, 'r')
    major = search_pattern(f, 'TAG_FWVER_MAJOR')
    major = major.strip()
    minor = search_pattern(f, 'TAG_FWVER_MINOR')
    minor = minor.strip()
    patch = search_pattern(f, 'TAG_FWVER_PATCH')
    patch = patch.strip()
    dev['FW_VERSION'] = major + '.' + minor +'.' + patch
    f.close()

def get_date_from_onboard_config():
    global dev
    default_val =['VID', 'MNID', 'MNMN']

    f = open(CONF_PATH+ONB_CFG_NAME, 'r')

    for pattern in default_val:
        check_valid(f, ONB_CFG_NAME, pattern)
    #print(" * Find the valid %s file"%(ONB_CFG_NAME))

    dev['ONB_ID'] = search_data(f, 'conf_setupId')
    dev['NAME']   = search_data(f, 'conf_modelName')
    dev['MNID']   = search_data(f, 'conf_mnId')
    dev['VID']   = search_data(f, 'conf_vid')
    dev['MNMN']   = search_data(f, 'conf_mnmn')
    f.close()

def build_profile():
    global dev

    values = dev['SN'] + ',' \
           + dev['MNID'] + ',' \
           + dev['NAME'] + ',' \
           + dev['ONB_ID'] + ',' \
           + dev['PARTNER'] + ',' \
           + dev['SKU'] + ',' \
           + dev['VENDOR'] + ',' \
           + dev['STATE'] + ',' \
           + dev['TYPE'] + ',' \
           + dev['PROD_ID'] + ',' \
           + dev['KEY_TYPE'] + ',' \
           + dev['CRV'] + ',' \
           + dev['KEY'] + ',' \
           + dev['MAC'] + ',' \
           + dev['BUTTON_ACTION'] + ',' \
           + dev['RING_THE_TAG'] + ',' \
           + dev['UPDATE_RINGTONE'] + ',' \
           + dev['BATTERY_TYPE'] + ',' \
           + dev['LEFT_BEHIND_ALERT'] + ',' \
           + dev['FIRMWARE_UPDATE'] + ',' \
           + dev['LED_BLINKING'] + ',' \
           + dev['POWER_SAVING_MODE'] + ',' \
           + dev['NFC']
    profile = fields + '\n' + values
    #print("\n*** The created profile: \n%s" % profile)
    return profile

def create_test_dev_profile(name, profile):
    f = open(name + '.csv', 'w+')
    f.write(profile)
    f.close()
    print("\n * The Device Configuration File is created!")
    print("  - %s\n" % (os.path.realpath(name + '.csv')))

def create_accessary_feature_profile(name):
    string_true = 'true'
    string_false = 'false'

    acjsonData = '"ringTheTag": ' + dev['RING_THE_TAG'] +', ' \
                + '"updateRingtone": ' + dev['UPDATE_RINGTONE'] +', ' \
                + '"buttonAction": ' + dev['BUTTON_ACTION'] +', ' \
                + '"batteryType": "' + dev['BATTERY_TYPE'] + '", ' \
                + '"leftBehindAlert": ' + dev['LEFT_BEHIND_ALERT'] +', ' \
                + '"firmwareUpdate": ' + dev['FIRMWARE_UPDATE'] +', ' \
                + '"ledBlinking": ' + dev['LED_BLINKING'] +', ' \
                + '"powerSavingMode": ' + dev['POWER_SAVING_MODE'] +', ' \
                + '"nfc": ' + dev['NFC']

    acjsonData = acjsonData.replace("1", "true")
    acjsonData = acjsonData.replace("0", "false")

    jsonData = '{"metadata" : {' \
                + '"modelName": "'+ dev['NAME'] + '", ' \
                + '"setupId": "' + dev['ONB_ID'] + '", ' \
                + '"mnId": "' + dev['MNID'] + '", ' \
                + '"firmwareVersion": "' + dev['FW_VERSION'] + '", ' \
                + '"supportedFeatures": {' \
                + acjsonData + '}}}'

    f = open(name + '.json', 'w+')
    f.write(jsonData)
    f.close()

    with zipfile.ZipFile(name + '.zip', 'w') as zf:
        zf.write(name + '.json')
        zf.write(CONF_PATH + DEV_INFO_NAME, DEV_INFO_NAME)
        zf.write(CONF_PATH + ONB_CFG_NAME, ONB_CFG_NAME)
        zf.write(INC_PATH + SPEC_VERSION_NAME, SPEC_VERSION_NAME)

    # Remove the firmware meta data JSON file
    os.remove(name + '.json')

    print(" * The firmware metadata is created!")
    print("  - %s\n" % (os.path.realpath(name + '.zip')))

if __name__ == '__main__':
    #get_date_from_device_info()
    get_data_from_accessory_option()
    get_version_info_from_file()
    get_fw_version_info_from_file()
    get_date_from_onboard_config()
    #create_test_dev_profile(dev['NAME']+ '_' + dev['SN'][-4:], build_profile())
    nowDate = datetime.datetime.now()
    create_accessary_feature_profile('firmwareMetadata_' + dev['NAME']+ '_' + dev['MNID'] + '_' + dev['ONB_ID'] + '_' + dev['FW_VERSION'])

