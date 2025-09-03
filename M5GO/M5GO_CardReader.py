from m5stack import *
from m5ui import *
from uiflow import *
import unit
from umqtt.simple import MQTTClient
import network
import ubinascii
import time
import urequests


# === Secrets ===
WIFI_SSID = 'chrserGHz'
WIFI_PASSWORD = 'serchr123'
MQTT_BROKER = '192.168.1.124'
API_IP = '192.168.1.128:8000'

# WiFi connection
wifi = network.WLAN(network.STA_IF)
wifi.active(True)
wifi.connect(WIFI_SSID, WIFI_PASSWORD)

while not wifi.isconnected():
    time.sleep(0.5)

# MQTT setup
client = MQTTClient("m5go", MQTT_BROKER)
client.connect()

setScreenColor(0x222222)

# Get client id
mac = wifi.config('mac')
client_id = ubinascii.hexlify(mac).upper()


# Inititialize label
label = M5TextBox(10, 10, "Booting", lcd.FONT_DejaVu18, 0xFFFFFF, rotate=0)

#initialize rfid
rfid = unit.get(unit.RFID, unit.PORTA)


# Get Uids from database
def fetch_authorized_uids():
    try:
        label.setText("Waiting for api")
        url = "http://" + API_IP + "/getCards"
        response = urequests.get(url)
        if response.status_code == 200:
            data = response.json()
            response.close()
            if isinstance(data, list):
                return [str(item["identifier"]).upper() for item in data if "identifier" in item]
        response.close()
    except Exception as e:
        label.setText("Exception")
        wait(5)
        print("Failed to fetch authorized UIDs:", e)
    return []

authorized_uids = fetch_authorized_uids()

# Card read
def clean_uid(uid_raw):
    if isinstance(uid_raw, str):
        return uid_raw.replace(":", "").upper()
    elif isinstance(uid_raw, list):
        result = ''
        for x in uid_raw:
            h = hex(x)[2:]
            if len(h) == 1:
                h = '0' + h
            result += h.upper()
        return result
    return "INVALID"

def rgb_color(r, g, b):
    return (r << 16) | (g << 8) | b

def set_led_color(r, g, b):
    color = rgb_color(r, g, b)
    for i in range(10):
        rgb.setColor(i, color)


# Card scan
while True:
    label.setText("Please scan your card")
    if rfid.isCardOn():
        uid_raw = rfid.readUid()
        uid = clean_uid(uid_raw)
        
        if uid in authorized_uids:
            label.setText("Card Accepted")
            set_led_color(0, 255, 0)  # green
            client.publish(b"CardScan", b"Accepted" + "," + client_id)
        else:
            label.setText("Card Denied")
            set_led_color(255, 0, 0)  # red

        wait(2)
        set_led_color(0, 0, 0)  # turn off LEDs
        label.setText("Please scan your card")
    else:
        set_led_color(0, 0, 0)


    wait_ms(200)
