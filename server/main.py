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
import psutil
import json
import socket
import time

mqttServerIP = "192.168.1.208"#"127.0.0.1"
mqttServerPort = 1883
mqttKeepAlive = 600
mqttTopic = "nxez/cube/dashboard/server"
alertCPUTempMax = 70
alertDiskUsagePercentMax = 95
alertLoadAvgMax = 9

def on_connect(client, userdata, flags, rc):
    print("Connected with result code: " + str(rc))

if __name__ == "__main__":
    data_old = {
        "net_io_counters": {
            "bytes_sent": psutil.net_io_counters().bytes_sent,
            "bytes_recv": psutil.net_io_counters().bytes_recv
        },
        "disk_io_counters": {
            "read_bytes": psutil.disk_io_counters().read_bytes,
            "write_bytes": psutil.disk_io_counters().write_bytes
        }
    }

    client = mqtt.Client()
    client.on_connect = on_connect
    client.connect(mqttServerIP, mqttServerPort, mqttKeepAlive)

    while True:
        cpu_temperature = -128
        ip = "127.0.0.1"
        host_name = "localhost"
        try:
            ip = socket.gethostbyname(socket.gethostname())
            host_name = socket.gethostname()
            cpu_temperature = (int(os.popen('cat /sys/class/thermal/thermal_zone0/temp').read())) / 1000
        except:
            pass

        alert = 0

        if cpu_temperature > alertCPUTempMax:
            alert = 1

        if psutil.disk_usage("/").percent > alertDiskUsagePercentMax:
            alert = 1

        if psutil.getloadavg()[2] > alertLoadAvgMax:
            alert = 1


        data = {
            "cpu_percent": psutil.cpu_percent(),
            "cpu_times_percent": {
                "user": psutil.cpu_times_percent().user,
                "nice": psutil.cpu_times_percent().nice,
                "system": psutil.cpu_times_percent().system,
                "idle": psutil.cpu_times_percent().idle
            },
            "cpu_count": psutil.cpu_count(),
            "cpu_freq": {
                "current": psutil.cpu_freq().current,
                "min": psutil.cpu_freq().min,
                "max": psutil.cpu_freq().max
            },
            "loadavg": {
                "1m": psutil.getloadavg()[0],
                "5m": psutil.getloadavg()[1],
                "15m": psutil.getloadavg()[2]
            },
            "virtual_memory": {
                "total": psutil.virtual_memory().total/1024/1024,
                "available": psutil.virtual_memory().available/1024/1024,
                "percent": psutil.virtual_memory().percent,
                "used": psutil.virtual_memory().used/1024/1024,
                "free": psutil.virtual_memory().free/1024/1024,
                "active": psutil.virtual_memory().active/1024/1024,
                "inactive": psutil.virtual_memory().inactive/1024/1024,
                #"wired": psutil.virtual_memory().wired/1024/1024
            },
            "swap_memory": {
                "total": psutil.swap_memory().total/1024/1024,
                "used": psutil.swap_memory().used/1024/1024,
                "free": psutil.swap_memory().free/1024/1024,
                "percent": psutil.swap_memory().percent,
                "sin": psutil.swap_memory().sin/1024/1024,
                "sout": psutil.swap_memory().sout/1024/1024
            },
            "disk_usage": {
                "total": psutil.disk_usage("/").total/1024/1024,
                "used": psutil.disk_usage("/").used/1024/1024,
                "free": psutil.disk_usage("/").free/1024/1024,
                "percent": psutil.disk_usage("/").percent
            },
            "disk_io_counters": {
                "read_count": psutil.disk_io_counters().read_count,
                "write_count": psutil.disk_io_counters().write_count,
                "read_bytes": psutil.disk_io_counters().read_bytes,
                "write_bytes": psutil.disk_io_counters().write_bytes,
                "read_time": psutil.disk_io_counters().read_time,
                "write_time": psutil.disk_io_counters().write_time
            },
            "net_io_counters": {
                "bytes_sent": psutil.net_io_counters().bytes_sent,
                "bytes_recv": psutil.net_io_counters().bytes_recv,
                "packets_sent": psutil.net_io_counters().packets_sent,
                "packets_recv": psutil.net_io_counters().packets_recv,
                "errin": psutil.net_io_counters().errin,
                "errout": psutil.net_io_counters().errout,
                "dropin": psutil.net_io_counters().dropin,
                "dropout": psutil.net_io_counters().dropout,
                "sent": psutil.net_io_counters().bytes_sent/1024/1024,
                "recv": psutil.net_io_counters().bytes_recv/1024/1024,
            },
            # "sensors_battery": {
            #     "percent": psutil.sensors_battery().percent,
            #     "secsleft": psutil.sensors_battery().secsleft,
            #     "power_plugged": 1 if psutil.sensors_battery().power_plugged else 0
            # },
            "up_time": int(time.time() - psutil.boot_time()),
            "ip": ip,
            "cpu_temperature": cpu_temperature,
            "host_name": host_name,
            "net_io_speed": {
                "recv": psutil.net_io_counters().bytes_recv - data_old.get("net_io_counters").get("bytes_recv"),
                "sent": psutil.net_io_counters().bytes_sent - data_old.get("net_io_counters").get("bytes_sent")
            },
            "disk_io_speed": {
                "read": psutil.disk_io_counters().read_bytes - data_old.get("disk_io_counters").get("read_bytes"),
                "write": psutil.disk_io_counters().write_bytes - data_old.get("disk_io_counters").get("write_bytes")
            },
            "time": {
                "timestamp": int(time.time()),
                "timestamp_cst": int(time.time()) + 3600 * 8,
                "tm_year": time.localtime().tm_year,
                "tm_mon": time.localtime().tm_mon,
                "tm_mday": time.localtime().tm_mday,
                "tm_hour": time.localtime().tm_hour,
                "tm_min": time.localtime().tm_min,
                "tm_sec": time.localtime().tm_sec,
                "tm_wday": time.localtime().tm_wday,
                "tm_yday": time.localtime().tm_yday,
                "tm_isdst": time.localtime().tm_isdst
            },
            "alert": alert

        }

        data_old = data
        client.publish(mqttTopic, payload = json.dumps(data), qos = 0)
        #print(json.dumps(data, sort_keys = False, indent = 4, separators = (',', ': ')))
        time.sleep(1)

