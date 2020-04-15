import getpass
import json
import ESPCon
import subprocess

def readCredentials():
    ssid = input('Please enter the Wifi SSID: ')
    wpa = getpass.getpass('Please enter the Wifi WPA key: ')
    return ssid, wpa

def saveConfigPrompt(cred):
    saveConfRes = input('Would you like to save this configuration? (y/n): ')

    if (saveConfRes.lower() == 'y'):
        saveConfig(cred)

def loadConfig():
    try:
        file = open('cred.conf', 'r')   
        return json.loads(file.read())
    except:
        print("Config file not found")
        return None

def saveConfig(cred):
    file = open('cred.conf', 'w')
    file.write(json.dumps(cred))
    file.close()
    
class ESPController():
    def __init__(self):
        self.cred = {"ssid": "", "wpa": "", "lastKnownIP": ""}
        self.port = 11400

    def connectSerial(self):
        self.ser = ESPCon.SerialConnection()
        self.cd = self.ser.detect()
        if (self.cd == ESPCon.CODE_DENIED):
            print('Couldn\'t Pass Authentication...')
            return False
        elif (self.cd == ESPCon.CODE_ERROR):
            print('Couldn\'t Find Device...')
            return False
        print('Established Serial Connection!')
        return True

    def connectWiFi(self):
        if (self.cd == ESPCon.CODE_OK):
            filecred = loadConfig()
            if (filecred):
                res = input('Would you like to use the existing configuration? (y/n): ')
                if (res.lower() == 'y'):
                    self.cred["ssid"] = filecred['ssid']
                    self.cred["wpa"] = filecred['wpa']
                    self.cred["lastKnownIP"] = filecred['lastKnownIP']
                else:
                    self.cred["ssid"], self.cred["wpa"] = readCredentials()
                    saveConfigPrompt(self.cred)
            else:
                self.cred["ssid"], self.cred["wpa"] = readCredentials()
                saveConfigPrompt(self.cred)

            if (self.ser.sendCred(self.cred["ssid"], self.cred["wpa"]) != ESPCon.CODE_OK):
                print('Device unable to connect to Wifi')
                return False
        print('Device connected to Wifi')
        self.cred["lastKnownIP"] = self.ser.getIp()
        print('Host IP: ' + self.cred["lastKnownIP"])
        saveConfig(self.cred)
        return True

    def changeIP(self):
        res = input('Enter the new IP:')
        self.cred["lastKnownIP"] = res
        saveConfig(self.cred)

    def startSending(self):
        self.cred = loadConfig()
        print(self.cred)
        if (self.cred):
            if (self.cred["lastKnownIP"]):
                self.wc = ESPCon.WirelessConnection(self.cred["lastKnownIP"], self.port)
                while True:
                    try:
                        self.wc.sendPixels()
                    except KeyboardInterrupt:
                        break
                    except ConnectionResetError:
                        print('The device was reset... Disconnected!')
                        break
            else:
                self.changeIP()
                self.startSending()
       


if __name__ == "__main__":
    cont = ESPController()
    print("Welcome to the ESPController")
    while True:
        try:
            ans = input("\n1. Configure through serial port\n2. Connect through wifi\n3.Change connection ip\n")
            if (ans == '1'):
                if cont.connectSerial():
                    cont.connectWiFi()
            elif (ans == '2'):
                cont.startSending()
            elif (ans == '3'):
                cont.changeIP()
            else:
                print("Not valid")
        except KeyboardInterrupt:
            print("\nBye")
            exit(0)
