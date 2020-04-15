import serial
import mss
import socket

INST_CREDENTIALS = '$0001'
INST_GETIP = '$0003'

CODE_OK = '$1000'
CODE_WAIT = '$2000'
CODE_DENIED = '$3000'
CODE_READY = '$4000'
CODE_ERROR = '$5000'
INST_START = "$0004"
INST_COLOR = "$0005" 
INST_COLOR_F = "$0006"
INST_COLOR_R = "$0007"
INST_COLOR_L = "$0008" 

BAUD = 74880
SERIALPORTS = ['/dev/ttyUSB', '/dev/ttyS', 'COM']

MSG0 = ':{code}:'
MSG1 = ':{code}:{data1}:'
MSG2 = ':{code}:{data1}:{data2}:'
MSG3 = ':{code}:{data1}:{data2}:{data3}:'
MSG10 = ':{code}:{data1}:{data2}:{data3}:{data4}:{data5}:{data6}:{data7}:{data8}:{data9}:'

class SerialConnection():
    def __init__(self):
        self.portsFound = []
        self.authenticated = False

    def readline(self):
        return self.deviceSerial.readline().decode('ascii')

    def sendCred(self, ssid = '', wpa = ''):
        return (self.send2(INST_CREDENTIALS, ssid, wpa))
    
    def getIp(self):
        return (self.send0(INST_GETIP))

    def send(self, text):
        self.deviceSerial.write(text.encode())
        line = self.readline()
        return line

    def send0(self, code):
        return self.send((MSG0.format(code=code)))
    
    def send1(self, code, arg1):
        return self.send((MSG1.format(code=code, data1=arg1)))

    def send2(self, code, arg1, arg2):
        return self.send((MSG2.format(code=code, data1=arg1, data2=arg2)))

    def send3(self, code, arg1, arg2, arg3):
        return self.send((MSG3.format(code=code, data1=arg1, data2=arg2, data3=arg3)))        

    def detect(self):
        print('Searching', end="")
        for port in SERIALPORTS:
            for i in range(0, 100):
                try:
                    ser = serial.Serial(port + str(i), BAUD, timeout = 10 )
                    self.portsFound.append(ser)
                except:
                    print('.', end="")
        print('\n')
        if (len(self.portsFound) == 0):
            return CODE_DENIED
        elif (len(self.portsFound) == 1):
            self.deviceSerial = self.portsFound[0]
            return CODE_OK
        else:
            print('Devices found: ')
            for ser in self.portsFound:
                print(ser)
            res = input('Enter a number to select one as your device: ')
            self.deviceSerial = self.portsFound[int(res)]
            return CODE_OK
        return CODE_ERROR

import time

class WirelessConnection():
    def __init__(self, ip, port):
        try:
            print(ip)
            self.socket = socket.create_connection((ip, port), 1000)
        except:
            print('Connection to server failed!')
    
    def sendPixels(self):
        pixels = [getPixels(0), getPixels(1), getPixels(2)]
        self.socket.send(bytes([0, 1]))
        for i in range(3):
            self.socket.send(bytes([pixels[i]['r'], pixels[i]['g'], pixels[i]['b']]))
            res = self.socket.recv(1)

def getPixelsRGB(res = 144):
    mon = mss.mss().monitors[0]
    img = mss.mss().grab(mon)
    rgb = {'r': 0, 'g': 0, 'b': 0}
    boundx = int(mon["width"])
    boundy = int(mon["height"])
    startx = 0
    endx = boundx
    starty = 0 
    endy = boundy
    for x in range(startx, endx, res):
        for y in range(starty, endy, res):
            pix = img.pixel(x, y)
            rgb['r'] += pix[0]
            rgb['g'] += pix[1]
            rgb['b'] += pix[2]
    total = (endx - startx) * (endy - starty) // (res * res)
    rgb['r'] //= total
    rgb['g'] //= total
    rgb['b'] //= total
    rgb['r'] //= 4
    rgb['g'] //= 4
    rgb['b'] //= 4
    return rgb

def getPixels(pos, res = 16):
    mon = mss.mss().monitors[0]
    img = mss.mss().grab(mon)
    rgb = {'r': 0, 'g': 0, 'b': 0}
    boundx = int(mon["width"])
    boundy = int(mon["height"])
    startx = 0 if (pos != 2) else 3 * (boundx // 4)
    endx = boundx // 4 if (pos == 0) else boundx
    starty = 0 
    endy = boundy if (pos != 1) else boundy // 4
    for x in range(startx, endx, res):
        for y in range(starty, endy, res):
            pix = img.pixel(x, y)
            rgb['r'] += pix[0]
            rgb['g'] += pix[1]
            rgb['b'] += pix[2]
    total = (endx - startx) * (endy - starty) // (res * res)
    rgb['r'] //= total
    rgb['g'] //= total
    rgb['b'] //= total
    return rgb
