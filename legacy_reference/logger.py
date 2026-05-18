import serial
import time

with serial.Serial(
        'COM49',
        baudrate=115200,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        dsrdtr=False,
        timeout=0) as ser:
    while True:
        lin = ser.readline()
        if lin:
            print(ser.readline())
  

