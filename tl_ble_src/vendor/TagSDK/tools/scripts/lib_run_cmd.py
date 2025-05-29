import subprocess
import shlex
import sys


def run_cmd(cmd_string, enable_log=True):

    # print(cmd_string)
    args = shlex.split(cmd_string)
    cmd_process = subprocess.Popen(args,
        stdout = subprocess.PIPE,
        stderr = subprocess.PIPE,
        universal_newlines = True)
    msg, err = cmd_process.communicate()

    if not err == '':
        print('')
        print('(error): {}'.format(err))
        exit(1)
    else:
        if enable_log:
            print('(done)')

    return msg
