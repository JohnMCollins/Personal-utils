#! /usr/bin/env python3

"""Create and install personal key"""

import os
import os.path
import sys
import argparse
from cryptography.fernet import Fernet, InvalidToken

parsearg = argparse.ArgumentParser(description='Create modify encypt or decryptcrpytography key file', formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parsearg.add_argument('files', nargs='*', help='Files to encrypt"')
parsearg.add_argument('--keyfile', type=str, help='Key file name if not standard place')
parsearg.add_argument('--force', action='store_true', help='Force create even if file exists')
parsearg.add_argument('--delete', action='store_true', help='Delete file')
parsearg.add_argument('--decrypt', action='store_true', help='Decrypt files otherwise encrypt')
resargs = vars(parsearg.parse_args())
files = resargs['files']
keyfile = resargs['keyfile']
force = resargs['force']
delfile = resargs['delete']
decrypt = resargs['decrypt']

iscrypt = []
filecontents = []
errors = 0

for f in files:
    try:
        with open(f, 'rb') as fin:
            cont = fin.read()
    except  IOError as e:
        print("Cannot open", f, "error was", e.strerror, file=sys.stderr)
        errors += 1
        continue
    filecontents.append(cont)

    iscrypt.append(min({len(b) for b in cont.split(b'\n')}) > 80)

if errors > 0:
    print("Aborting due to", errors, "errors", file=sys.stderr)
    sys.exit(10)

if keyfile is None:
    keyfile = os.path.expanduser("~/.jmc/sec.key")

if not os.path.isfile(keyfile):
    if decrypt:
        if sum(iscrypt) != 0:
            print("No key file present - cannot decrypt", file=sys.stderr)
            sys.exit(11)
    if delfile:
        if not force:
            if  sum(iscrypt) != 0:
                print("Some files still encrypted, use --force if you really want this", file=sys.stderr)
                sys.exit(12)
            print("File does not exist, nothing to delete (or use --force)", file=sys.stderr)
    print("Generating key....", file=sys.stderr)
    enc_key = Fernet.generate_key()
    keydirs, keyname = os.path.split(keyfile)
    dbits = keydirs.split('/')
    dpath = dbits.pop(0)
    for d in dbits:
        dpath += '/' + d
        if not os.path.isdir(dpath):
            try:
                os.mkdir(dpath, 0o777)
            except  OSError as e:
                print("Could not create", dpath, e.strerror, file=sys.stderr)
                sys.exit(13)
    try:
        with open(keyfile, 'wb') as fout:
            fout.write(enc_key)
    except OSError as e:
        print("Could not create", keyfile, e.strerror, file=sys.stderr)
        sys.exit(14)
    try:
        os.chmod(keyfile, 0o400)
    except OSError as e:
        print("Could not fix mode of", keyfile, e.strerror, file=sys.stderr)

else:
    try:
        with  open(keyfile, 'rb') as fin:
            enc_key = fin.read()
    except OSError as e:
        print("Could not read", keyfile, e.strerror, file=sys.stderr)
        sys.exit(15)

try:
    fern_key = Fernet(enc_key)
except ValueError:
    print("Key format incorrect", filen=sys.stderr)
    sys.exit(16)

for fil, crypted, currcontents in zip(files, iscrypt, filecontents):
    if decrypt:
        if not crypted:
            continue
        try:
            newcontents = fern_key.decrypt(currcontents)
        except InvalidToken:
            print("Key not correct to decrypt", fil, file=sys.stderr)
            errors += 1
            continue
    else:
        if crypted:
            continue
        newcontents = fern_key.encrypt(currcontents)

    try:
        ss = os.stat(fil)
        sm = ss.st_mode & 0o7777
        os.chmod(fil, sm | 0o666)
        with open(fil, 'wb') as fout:
            fout.write(newcontents)
        os.chmod(fil, sm)
    except OSError as e:
        print("Could not write", fil, e.strerror, file=sys.stderr)
        errors += 1

if errors > 0:
    print("Had", errors, "errors", file=sys.stderr)
    sys.exit(50)

if delfile:
    try:
        os.unlink(keyfile)
    except OSError as e:
        print("Cannot delete", keyfile, e.strerror)
        sys.exit(51)
