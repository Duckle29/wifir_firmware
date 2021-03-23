#!/usr/bin/env python3
from pathlib import Path
from time import time
from sys import exit

Import("env")

certbundle = Path(env["PROJECT_DIR"]) / "data" / "certs.ar"

if certbundle.is_file():
    # If certificate bundle is more than 30 days, fetch and upload a new image
    if time() - certbundle.stat().st_mtime < 30 * 24 * 60 * 60:
        print("Certificate bundle is still recent. No change")
        exit(0)
else:
    if not certbundle.parent.is_dir():
        certbundle.parent.mkdir()

env.Execute("$PYTHONEXE get_moz_certs.py")
env.Execute("$PYTHONEXE -m platformio run --target uploadfs")