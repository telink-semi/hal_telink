import os

DK_SNR = 0

stream = os.popen('nrfjprog -i')
output = stream.read()
snrLen = len(output)

if snrLen == 10:
    DK_SNR = output
else :
    print('Connected device list(snr  port  vcom):')
    print('------------------------------')
    os.system('nrfjprog --com')
    print('------------------------------')

    DK_SNR = input('Input your device serial number(snr): ')

print('device snr is:', DK_SNR)
