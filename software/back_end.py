import serial
import time

class BackEndHandler:
    esp = None

    def get_available_ports(self):
        ports = ['COM%s' %(i+1) for i in range(256)]
        output = []
        for port in ports:
            try:
                s = serial.Serial(port)
                s.close()
                output.append(port)
            except serial.SerialException:
                pass
        return output

    def connect_to_esp(self):
        ports = self.get_available_ports();
        if len(ports) == 0:
            print('no available ports')
            return

        print("select Port:")
        for i in range(1, len(ports) + 1):
            print('%s) ' %i + ports[i-1])

        decision = input('>> ')
        port = ports[int(decision)-1]
        com = serial.Serial(port, baudrate=115200)
        print("connected to", com.name)
        print("waiting for response from device")

        got_response = False
        timed_out = False
        before = time.time()
        while not got_response and not timed_out:
            com.write('s'.encode())

            if com.inWaiting() > 1:
                response = com.read(2).decode()
                print('got', response)
                if response == 'ok':
                    got_response = True

            if time.time() - before > 3:
                timed_out = True

            time.sleep(.1)

        if not got_response:
            print('did not get response')
            return None

        print('connected to device')
        return com

    def read_message(self, length, raw=False):
        timed_out = False
        before = time.time()
        while self.esp.inWaiting() < length and not timed_out:
            if time.time() - before >= 3:
                timed_out = True
        if timed_out:
            return 'timed out'
        if raw:
            return self.esp.read(length)

        return self.esp.read(length).decode()

    def set_ssid_and_password(self, ssid, password):
        if self.esp == None:
            return'failed\nnot connected to device'

        message = 'set ssid and password'
        self.esp.write(bytes([len(message)]))
        self.esp.write(message.encode())

        response = self.read_message(2)

        if response == 'timed out':
            return 'failed\ninitial reponse timed out'

        if response != 'ok':
             response += self.read_message(self.esp.inWaiting())
             return 'failed\ndid not get ok response\nresponse: ' + response

        self.esp.write(bytes([len(ssid)]))
        self.esp.write(bytes([len(password)]))
        self.esp.write(ssid.encode())
        self.esp.write(password.encode())

        response = self.read_message(len(ssid) + len(password))

        if response == 'timed out':
            return 'failed\nechoed reponse timed out'

        echoed_ssid = response[:len(ssid)]
        echoed_password = response[len(ssid):]

        if echoed_ssid == ssid and echoed_password == password:
            return 'successful\nssid set to ' + ssid + '\npassword set to ' + password
        else:
            return 'failed\nresponded ssid: ' + echoed_ssid + '\nresponded password: ' + password

    def query_ssid_and_password(self):
        if self.esp == None:
            return'failed\nnot connected to device'

        message = 'query ssid and password'
        self.esp.write(bytes([len(message)]))
        self.esp.write(message.encode())

        response = self.read_message(2)

        if response == 'timed out':
            return 'failed\ninitial reponse timed out'

        if response != 'ok':
            response += self.read_message(self.esp.inWaiting())
            return 'failed\ndid not get ok response\nresponse: ' + response

        response = self.read_message(1, True)
        if response == 'timed out':
            return 'failed\nreponse times out'

        ssid_length = int.from_bytes(response, 'little')

        response = self.read_message(1, True)
        if response == 'timed out':
            return 'failed\nreponse times out'

        password_length = int.from_bytes(response, 'little')

        ssid = self.read_message(ssid_length)
        password = self.read_message(password_length)

        if ssid == 'timed out' or password == 'timed out':
            return 'failed\nreponse times out'

        return 'successful\nssid is ' + ssid + '\npassword is ' + password

    def set_port(self, port):
        if self.esp == None:
            return'failed\nnot connected to device'

        message = 'set port'
        self.esp.write(bytes([len(message)]))
        self.esp.write(message.encode())

        response = self.read_message(2)

        if response == 'timed out':
            return 'failed\ninitial reponse timed out'

        if response != 'ok':
             response += self.read_message(self.esp.inWaiting())
             return 'failed\ndid not get ok response\nresponse: ' + response

        high_byte = port >> 8
        low_byte = port & 0xff
        self.esp.write(bytes([high_byte, low_byte]))

        response = self.read_message(len(str(port)))

        if response == 'timed out':
            return 'failed\nresponse timed out'

        if int(response) == port:
            return 'successful\ndevice listening on port ' + str(port)
        return 'failed\nresponded port: ' + response

    def query_port(self):
        if self.esp == None:
            return'failed\nnot connected to device'

        message = 'query port'
        self.esp.write(bytes([len(message)]))
        self.esp.write(message.encode())

        response = self.read_message(2)

        if response == 'timed out':
            return 'failed\ninitial reponse timed out'

        if response != 'ok':
            response += self.read_message(self.esp.inWaiting())
            return 'failed\ndid not get ok response\nresponse: ' + response

        response = self.read_message(1, True)
        if response == 'timed out':
            return 'failed\nreponse times out'

        length = int.from_bytes(response, 'little')

        port = self.read_message(length)

        if port == 'timed out':
            return 'failed\nreponse times out'

        return 'successful\ndevice listening to port ' + port

    def connect_to_WiFi(self):
        if self.esp == None:
            return'failed\nnot connected to device'

        message = 'connect to WiFi'
        self.esp.write(bytes([len(message)]))
        self.esp.write(message.encode())

        response = self.read_message(2)

        if response == 'timed out':
            return 'failed\ninitial reponse timed out'

        if response != 'ok':
            response += self.read_message(self.esp.inWaiting())
            return 'failed\ndid not get ok response\nresponse: ' + response

        print('connecting...')
        while self.esp.inWaiting() == 0:
            pass

        time.sleep(.5)
        return self.read_message(self.esp.inWaiting())




