#! /usr/bin/env python3

"""DIsplay files which may have been encrypted"""

import os
import os.path
import sys
import argparse
from cryptography.fernet import Fernet, InvalidToken

parsearg = argparse.ArgumentParser(description='Create modify encypt or decryptcrpytography key file', formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parsearg.add_argument('files', nargs='+', help='Files to output"')
parsearg.add_argument('--keyfile', type=str, help='Key file name if not standard place')
resargs = vars(parsearg.parse_args())
files = resargs['files']
keyfile = resargs['keyfile']

if keyfile is None:
    keyfile = os.path.expanduser("~/.jmc/sec.key")

try:
    with  open(keyfile, "rb") as kin:
        enc_key = kin.read()
except OSError as e:
    print("Could not read", keyfile, e.strerror, file=sys.stderr)
    sys.exit(10)
try:
    fern_key = Fernet(enc_key)
except ValueError:
    print("Key format incorrect", filen=sys.stderr)
    sys.exit(16)

errors = 0

for f in files:
    try:
        with open(f, 'rb') as fin:
            cont = fin.read()
    except  IOError as e:
        print("Cannot open", f, "error was", e.strerror, file=sys.stderr)
        errors += 1
        continue

    if  min({len(b) for b in cont.split(b'\n')}) > 80:
        try:
            newcontents = fern_key.decrypt(cont)
        except InvalidToken:
            print("Key not correct to decrypt", f, file=sys.stderr)
            errors += 1
            continue
        cont = newcontents

    print(cont.decode(), end='')

if errors > 0:
    sys.exit(100)
sys.exit(0)
