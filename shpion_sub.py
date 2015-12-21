#!/usr/bin/python -tu

import sys
import time
import yaml
import paho.mqtt.client as mqtt

def dtFmt(unixtime):
    return time.strftime("%F %T")

class ShpionSub:
    mqttc = None
    udict = None
    config = None

    def on_connect(self, mqttc, obj, flags, rc):
        #print("rc: "+str(rc))
        self.mqttc.subscribe("shpion/vmact/#", 0)
        
    def on_message(self, mqttc, obj, msg):
        #print(self, msg.topic+" "+str(msg.qos)+" "+str(msg.payload))
        tparts = msg.topic.split('/')
        unixtm,count = msg.payload.split(',')
        hash = tparts[len(tparts)-1]
        try: 
            person = self.config['users'][hash]
            ts = dtFmt(unixtm)
            count = int(count)
            print "%s,%s,%d" % (ts, person if person else hash, count)
        except KeyError:
            pass
        
    def on_publish(self, mqttc, obj, mid):
        #print("mid: "+str(mid))
        pass
        
    def on_subscribe(self, mqttc, obj, mid, granted_qos):
        #print("Subscribed: "+str(mid)+" "+str(granted_qos))
        pass
        
    def on_log(self, mqttc, obj, level, string):
        print(string)

    def __init__(self):
        self.mqttc = mqtt.Client()
        self.mqttc.on_message = self.on_message
        self.mqttc.on_connect = self.on_connect
        self.mqttc.on_publish = self.on_publish
        self.mqttc.on_subscribe = self.on_subscribe
        # Uncomment to enable debug messages
        #self.mqttc.on_log = self.on_log
        self.mqttc.connect("stap3poldwv", 1883, 60)
        self.config = yaml.load(file('/pnp/etc/shpion_sub.yaml', 'r'))


if "__main__" == __name__:
    me = ShpionSub()
    me.mqttc.loop_forever()


