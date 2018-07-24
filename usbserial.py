#!/usr/bin/env python
# coding: utf-8

import serial, sys, os

sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)

if len(sys.argv) > 1:
    port = sys.argv[1]
else:
    port = '/dev/ttyUSB3'

while 1:
    print '> opening %s' % port
    s = serial.Serial(port=port, baudrate=115200)
    while 1:
        try:
            print s.readline().strip()
        except KeyboardInterrupt:
            print '> stop'
            sys.exit(0)

