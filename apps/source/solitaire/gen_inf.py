#!/usr/bin/env python3
"""Generate solitaire.inf from raw icon data."""
import sys

name = sys.argv[1]
icon_path = sys.argv[2]
out_path = sys.argv[3]

with open(icon_path, 'rb') as f:
    icon = f.read()

with open(out_path, 'wb') as f:
    f.write(name.encode('ascii') + b'\n')
    f.write(icon)
