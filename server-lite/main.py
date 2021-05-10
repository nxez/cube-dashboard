#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright (c) 2021 NXEZ.COM.
# https://www.nxez.com
#
# Licensed under the GNU General Public License, Version 3.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.gnu.org/licenses/gpl-3.0.html
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

__author__ = 'Spoony'
__version__  = 'version 0.0.1'
__license__  = 'Copyright (c) 2021 NXEZ.COM'

import paho.mqtt.client as mqtt
import os
import json
import time

mqttServerIP = "192.168.1.204"#"127.0.0.1"
mqttServerPort = 1883
mqttKeepAlive = 600
mqttTopic = "nxez/cube/dashboard/server"

def on_connect(client, userdata, flags, rc):
    print("Connected with result code: " + str(rc))


# Return % of CPU used by user as a character string
def getCPUUsage():
    return(str(os.popen("top -n1 | awk '/Cpu\(s\):/ {print $2}'").readline().strip()))

def getDiskStats():
    p = os.popen("df -h /")
    i = 0
    while 1:
        i = i +1
        line = p.readline()
        if i==2:
            return(line.split()[1:5])

# Return RAM information (unit=kb) in a list
# Index 0: total RAM
# Index 1: used RAM
# Index 2: free RAM
def getRAMinfo():
    p = os.popen('free')
    i = 0
    while 1:
        i = i + 1
        line = p.readline()
        if i==2:
            return(line.split()[1:4])

if __name__ == "__main__":
    client = mqtt.Client()
    client.on_connect = on_connect
    client.connect(mqttServerIP, mqttServerPort, mqttKeepAlive)

    while True:
        cpu_temperature = -128
        boot_time = 0

        # RAM information
        # Output is in kb, here I convert it in Mb for readability
        RAM_stats = getRAMinfo()
        RAM_total = round(int(RAM_stats[0]) / 1000,1)
        RAM_used = round(int(RAM_stats[1]) / 1000,1)
        RAM_free = round(int(RAM_stats[2]) / 1000,1)

        # Disk information
        DISK_stats = getDiskStats()
        DISK_total = DISK_stats[0]
        DISK_used = DISK_stats[1]
        DISK_free = DISK_stats[2]
        DISK_percent = DISK_stats[3]

        try:
            cpu_temperature = (int(os.popen('cat /sys/class/thermal/thermal_zone0/temp').read())) / 1000
            boot_time = os.popen('cat /proc/uptime').read().split()[0]
            #print((int(float(boot_time))))
        except:
            pass

        data = {
            "cpu_percent": getCPUUsage(),
            "virtual_memory": {
                "total": RAM_total,
                "used": RAM_used,
                "free": RAM_free
            },
            "disk_usage": {
                "total": DISK_total,
                "used": DISK_used,
                "free": DISK_free,
                "percent": DISK_percent
            },
            "up_time": (int(float(boot_time))),
            "cpu_temperature": cpu_temperature,
            "time": {
                "timestamp_cst": int(time.time()) + 3600 * 8
            }
        }

        client.publish(mqttTopic, payload = json.dumps(data), qos = 0)
        print(json.dumps(data, sort_keys = False, indent = 4, separators = (',', ': ')))
        time.sleep(5)

