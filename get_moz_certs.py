#!/usr/bin/env python3

# This script pulls the list of Mozilla trusted certificate authorities
# from the web at the "mozurl" below, parses the file to grab the PEM
# for each cert, and then generates DER files in a new ./data directory
# Upload these to an on-chip filesystem and use the CertManager to parse
# and use them for your outgoing SSL connections.
#
# Script by Earle F. Philhower, III.  Released to the public domain.
from __future__ import print_function
import csv
import os
import sys
from shutil import which

from pathlib import Path

from subprocess import Popen, PIPE, call
try:
    from urllib.request import urlopen
except Exception:
    from urllib2 import urlopen
try:
    from StringIO import StringIO
except Exception:
    from io import StringIO

print("\nGenerating certificate store from mozilla trusted certificates")

ar_path = Path.home() / Path('.platformio/packages/toolchain-xtensa/bin/xtensa-lx106-elf-ar.exe')  # noqa E501

# check if ar and openssl are available
if not ar_path.is_file() and which('ar') is None:
    raise Exception("You need the program 'ar' from xtensa-lx106-elf found here: (esp8266-arduino-core)/hardware/esp8266com/esp8266/tools/xtensa-lx106-elf/xtensa-lx106-elf/bin/ar")  # noqa E501
if which('openssl') is None and not Path('./openssl').is_file() and not Path('./openssl.exe').is_file():  # noqa E501
    raise Exception("You need to have openssl in PATH, installable from https://www.openssl.org/")  # noqa E501

if not ar_path.is_file():
    ar_path = which('ar')

# Mozilla's URL for the CSV file with included PEM certs
mozurl = "https://ccadb-public.secure.force.com/mozilla/IncludedCACertificateReportPEMCSV"  # noqa E501

# Load the names[] and pems[] array from the URL
names = []
pems = []
response = urlopen(mozurl)
csvData = response.read()
if sys.version_info[0] > 2:
    csvData = csvData.decode('utf-8')
csvFile = StringIO(csvData)
csvReader = csv.reader(csvFile)
for row in csvReader:
    names.append(row[0]+":"+row[1]+":"+row[2])
    for item in row:
        if item.startswith("'-----BEGIN CERTIFICATE-----"):
            pems.append(item)
del names[0]  # Remove headers
del pems[0]  # Remove headers

# Try and make ./data, skip if present
try:
    os.mkdir("data")
except Exception:
    pass

derFiles = []
idx = 0
# Process the text PEM using openssl into DER files
for i in range(0, len(pems)):
    certName = "data/ca_%03d.der" % (idx)
    thisPem = pems[i].replace("'", "")
    ssl = Popen([
        'openssl', 'x509', '-inform', 'PEM', '-outform', 'DER',
        '-out', certName
        ],
        shell=False, stdin=PIPE)

    pipe = ssl.stdin
    pipe.write(thisPem.encode('utf-8'))
    pipe.close()
    ssl.wait()
    if os.path.exists(certName):
        derFiles.append(certName)
        idx = idx + 1

if os.path.exists("data/certs.ar"):
    os.unlink("data/certs.ar")

arCmd = [str(ar_path), 'q', 'data/certs.ar'] + derFiles
call(arCmd)

print(f"\nFetched {len(derFiles)} certificates\n")

for der in derFiles:
    os.unlink(der)
