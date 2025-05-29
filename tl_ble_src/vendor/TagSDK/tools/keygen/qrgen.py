#!/usr/bin/env python3
#-*- coding: utf-8 -*-

import os
import argparse
from urllib import parse

REF_PATH = os.path.dirname(os.path.abspath(__file__))

class HeaderParser():
    def __init__(self):
        self._sn = ""
        self._mnid = ""
        self._setupId = ""

    def parse_deviceInfo(self, path):
        try:
            file = open(path, 'r', encoding='utf8')
        except:
            raise FileNotFoundError("Failed to open deviceInfo : %s", path)

        while True:
            try:
                line = file.readline()
            except:
                raise ValueError("Failed to read deviceInfo : %s", path)

            if not line:
                break
            elif "conf_device_serial_number" in line:
                self._sn = line.split('"')[1]
                break
        if not self._sn or self._sn == "serialNumber_here":
            raise ValueError("SERIAL NUMBER has not been set in " + path)

    def parse_onboardingConfig(self, path):
        try:
            file = open(path, 'r', encoding='utf8')
        except:
            raise FileNotFoundError("Failed to open onboardingConfig : %s", path)

        while True:
            try:
                line = file.readline()
            except:
                raise ValueError("Failed to read onboardingConfig : %s", path)

            if not line:
                break
            elif "conf_setupId" in line:
                self._setupId = line.split('"')[1]
            elif "conf_mnId" in line:
                self._mnid = line.split('"')[1]

        if not self._setupId:
            raise ValueError("SETUP ID has not been set in '" + path)
        if not self._mnid or self._mnid == "MNID":
            raise ValueError("MNID has not been set in '" + path)


class Qr():
    def generate_using_header(self, outputPath, device_info_path, onboarding_config_path):
        import qrcode

        headerParser = HeaderParser();

        headerParser.parse_deviceInfo(device_info_path)
        headerParser.parse_onboardingConfig(onboarding_config_path)


        self._qr_url = "https://qr.samsungiots.com/?m=%s&s=%s&r=%s" % (
            headerParser._mnid, headerParser._setupId, headerParser._sn)
        
        qr = qrcode.QRCode(
            error_correction=qrcode.constants.ERROR_CORRECT_M,
            box_size=10,
            # border=4,
        )

        qr.add_data(self._qr_url)
        qr.make(fit=True)

        if outputPath is None:
            outputPath = "output_" + headerParser._sn

        pos = outputPath.find("bulk")

        if pos == -1:
            print("This is not bulk");
        else:
            sn_pos = outputPath.find(headerParser._sn) 
            if sn_pos == -1:
                outputPath = os.path.join(outputPath, headerParser._sn)
            
        if not os.path.exists(outputPath):
            os.mkdir(outputPath)

        img_path = os.path.join(outputPath, "qr-" + headerParser._sn + ".png")
        img = qr.make_image(fill_color="black", back_color="white")
        img.save(img_path)
        print("File:\t%s \nQR url:\t%s" % (img_path, self._qr_url))


def main():
    parser = argparse.ArgumentParser(description="SmartThings Find Device SDK QR gen tools")
    parser.add_argument(
        '--path',
        default=os.path.abspath(os.path.join(REF_PATH, "../../conf")),
        help="[I] path of dierectory containing TagOnboardingConfig.h and TagDeviceInfo.h")

    args = parser.parse_args()

    if args.path is None:
        print("ERROR : path is required")
        return

    dir = args.path

    device_info = os.path.join(dir, "TagDeviceInfo.h")
    onboarding_config = os.path.join(dir, "TagOnboardingConfig.h")

    qr = Qr()
    qr.generate_using_header(None, device_info, onboarding_config)

if __name__ == "__main__":
    main()
