# Must be run not in a venv. Jetson.GPIO doesn't install in a venv for some reason.
# Valid pins: https://maker.pro/nvidia-jetson/tutorial/how-to-use-gpio-pins-on-jetson-nano-developer-kit
# Jetson.GPIO is here in venv:

import Jetson.GPIO as GPIO
import serial
import time
from time import sleep
from serial.tools import list_ports
import re

# Board pins (i.e. the numbers that are on the Jetson itself)
# All active low relays
boost_0_en_pin = 7
boost_1_en_pin = 11
boost_2_en_pin = 12
boost_en_pins = [boost_0_en_pin, boost_1_en_pin, boost_2_en_pin]

# Toggle low then high.
charge_start_pin = 13

wing_power_en_pin = 15


class MCURXSerDataPkt:
    # 0=continous=batt detected
    BATT_NOT_DETECTED = 0
    BATT_DETECTED = 1
    battConts = []
    wingCont = 0
    battVoltsMv = []
    hubBattVoltMv = 0

    def __init__(self, batt0Cont=0, batt1Cont=0, batt2Cont=0, wingCont=0, batt0VoltMv=0,
                 batt1VoltMv=0, batt2VoltMv=0, hubBattVoltMv=0):
        self.battConts = [batt0Cont, batt1Cont, batt2Cont]
        self.wingCont = wingCont
        self.battVoltsMv = [batt0VoltMv, batt1VoltMv, batt2VoltMv]

    def getBattCont(self, slot: int) -> int:
        return self.battConts[slot]

    def getWingCont(self, slot: int) -> int:
        return self.wingCont

    def getBattVoltMv(self, slot: int) -> int:
        return self.battVolts[slot]

    def getBattVoltV(self, slot: int) -> int:
        return (self.battVolts[slot] * 1000.0)

    def getHubBattVoltMv(self, slot: int) -> int:
        return self.hubBattVoltMv

    def getHubBattVoltV(self, slot: int) -> int:
        return (self.hubBattVoltMv * 1000.0)


class MCUController:
    '''
    For controlling communication with the MCU
    Data must be sent over serial in this format:
    Example input data from MCU: >batt0Cont=0,batt1Cont=1,batt2Cont=1,wingCont=1
    '''
    mcu_port_VID = 1027
    mcu_port_PID = 24577
    USB_PORT = ""
    MCU_BPS = 9600
    SERIAL_CONNECTION: serial.Serial = None

    def __init__(self):
        self.USB_PORT = self.__get_com_port()
        print(self.USB_PORT)
        self.SERIAL_CONNECTION = self.__connect(self.USB_PORT, self.MCU_BPS)

    def get_mcu_serial_data_blocking(self) -> MCURXSerDataPkt:
        '''
        Will block here until we get the next packet of serial data from the MCU.
        Data from the MCU should all be sent in a single comma-separated line.
        '''
        mcu_rx_pkt = MCURXSerDataPkt()
        start = -1
        # Flush the input buffer since the MCU will have sent a bunch of data and
        # filled the Jetson's serial input buffer with a bunch of now old data.
        self.SERIAL_CONNECTION.flushInput()
        print("BLOCKING UNTIL SERIAL DATA RECEIVED FROM MCU")
        while (start == -1):
            line = self.SERIAL_CONNECTION.readline().decode("utf-8")
            start = line.find(">")
            print("Serial data from MCU:\n" + line)
        mcu_rx_pkt = self.__parse_mcu_raw_serial_data(line)
        print(mcu_rx_pkt)
        return mcu_rx_pkt

    def __get_com_port(self) -> str:
        print("Getting COM PORT")
        device_list = list_ports.comports()
        port = None
        print("Here are all detected COM Ports:")
        for device in device_list:
            print(device)
            print(device.vid)
            print(device.pid)
        print("===========================================\n\n")
        for device in device_list:
            if (device.vid != None or device.pid != None):
                if (device.vid == self.mcu_port_VID and
                        device.pid == self.mcu_port_PID):
                    port = device.device
                    print("MCU found on port:" + port)
                    break
                port = None
        if port == None:
            print("ERROR: MCU not found!!!!")
        return port

    def __connect(self, port: str, baud: int) -> serial.Serial:
        """Connect to the MCU controller"""
        print("Connecting to MCU...")
        tempSerConnection = serial.Serial(port,
                                          baudrate=baud,
                                          parity=serial.PARITY_NONE,
                                          stopbits=serial.STOPBITS_ONE,
                                          timeout=1)
        print(tempSerConnection)
        # Wake up MCU
        tempSerConnection.write(str.encode("\r\n\r\n"))
        # Wait for MCU to initialize and flush startup text in serial input
        time.sleep(2)
        tempSerConnection.flushInput()
        print("Connected to MCU")
        return tempSerConnection

    def __get_value_from_input_data(self, keyVal: str) -> int:
        valStart = keyVal.find("=")
        if valStart != -1:
            return int(keyVal[valStart+1])
        else:
            return None

    def __parse_mcu_raw_serial_data(self, raw_data: str) -> MCURXSerDataPkt:
        # Remove whitespace
        raw_data = re.sub(r"\s+", "", raw_data)
        # Split data up
        data = raw_data[1:].split(",")
        # Extract the values
        return MCURXSerDataPkt(self.__get_value_from_input_data(data[0]),
                               self.__get_value_from_input_data(data[1]),
                               self.__get_value_from_input_data(data[2]),
                               self.__get_value_from_input_data(data[3]))


class ChargeController:
    mcu = MCUController()

    def __init__(self):
        print(GPIO.JETSON_INFO)
        print(GPIO.VERSION)
        self.init()

    def loopHook():
        # If a battery is detected in a slot, then start charging it unless it is full.
        c = 1

    def enable_charging(self, slot: int, force: bool = False):
        print("Starting charging on slot " + str(slot) + "...")
        # Check that the battery is there
        if not is_batt_detected(slot):
            print("BATTERY NOT DETECTED in slot " +
                  str(slot) + ". CANNOT start charging!!")
        else:
            # Enable the Boost converter
            GPIO.output(boost_en_pins[slot], GPIO.LOW)
            print("Boost enabled.")
            sleep(1)
            GPIO.output(charge_start_pin, GPIO.LOW)
            print("Start button low.")
            sleep(1)
            GPIO.output(charge_start_pin, GPIO.HIGH)
            print("Start button high.")
            print("Enabled charging on slot " + str(slot))

    def disable_charging(self, slot: int):
        print("Disabling charging on slot " + str(slot) + "...")
        GPIO.output(boost_en_pins[slot], GPIO.HIGH)
        print("Disabled charging on slot " + str(slot))

    def init(self):
        print("Initializing ChargeController...")
        # Pin Setup:
        GPIO.setmode(GPIO.BOARD)

        # Outputs
        GPIO.setup(boost_0_en_pin, GPIO.OUT)
        GPIO.setup(boost_1_en_pin, GPIO.OUT)
        GPIO.setup(boost_2_en_pin, GPIO.OUT)
        GPIO.setup(charge_start_pin, GPIO.OUT)
        GPIO.setup(wing_power_en_pin, GPIO.OUT)

        # Start all HIGH
        GPIO.output(boost_0_en_pin, GPIO.HIGH)
        GPIO.output(boost_1_en_pin, GPIO.HIGH)
        GPIO.output(boost_2_en_pin, GPIO.HIGH)
        GPIO.output(charge_start_pin, GPIO.HIGH)
        GPIO.output(wing_power_en_pin, GPIO.HIGH)
        print("Initialized ChargeController SUCCESSFULLY")

    def is_batt_detected(self, slot: int) -> bool:
        '''
        Checks if the given slot has a battery in it by checking continuity.
        '''
        # Active low. Continuity when LOW.
        # First get a new status packet from mcu
        data: MCURXSerDataPkt = self.mcu.get_mcu_serial_data_blocking()
        if data.getBattCont(slot) == MCURXSerDataPkt.BATT_DETECTED:
            return False
        else:
            return True

    def is_wing_cont_detected(self) -> bool:
        # Active low. Continuity when LOW.
        # First get a new status packet from mcu
        data: MCURXSerDataPkt = self.mcu.get_mcu_serial_data_blocking()
        if data.wingCont == MCURXSerDataPkt.BATT_DETECTED:
            return False
        else:
            return True

    def ESTOP(self):
        exit(0)


def repl():
    cont = ChargeController()

    while (1):
        cmd_line = input(">")
        cmds = cmd_line.split()
        cmd = cmds.pop(0)
        if cmd == "e":
            ESTOP()
        elif cmd == "c0":
            enable_charging(0)
        elif cmd == "c1":
            enable_charging(1)
        elif cmd == "c2":
            enable_charging(2)
        elif cmd == "s0":
            disable_charging(0)
        elif cmd == "s1":
            disable_charging(1)
        elif cmd == "s2":
            disable_charging(2)
        elif cmd == "d0":
            print("BattCont: " + str(cont.is_batt_detected(0)))
        elif cmd == "d1":
            print("BattCont: " + str(cont.is_batt_detected(1)))
        elif cmd == "d2":
            print("BattCont: " + str(cont.is_batt_detected(2)))
        elif cmd == "d3":
            print("WingCont: " + str(cont.is_wing_cont_detected()))
        else:
            print("Command not found: " + cmd)


def loop_mode():
    controller = ChargeController()
    while (1):
        controller.loopHook()


def repl_mode():
    repl()


if __name__ == "__main__":
    try:
        repl_mode()
        # loop_mode()
    except KeyboardInterrupt:
        print("Keyboard Interrupts")
    finally:
        GPIO.cleanup()  # this ensures a clean exit
