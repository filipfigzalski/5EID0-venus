import paho.mqtt.client as mqtt
import json
import logging

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.DEBUG)


def get_data() -> dict:
    x = int(input("Enter x: "))
    y = int(input("Enter y: "))

    typ = str(input("Enter type: "))
    color = str(input("Enter color: "))
    size = str(input("Enter size: "))

    return {"x": x, "y": y, "type": typ, "color": color, "size": size}


def main():
    # username and password of mqtt client
    username, password = "Student38", "quaiMie6"
    host = "mqtt.ics.ele.tue.nl"
    topic = "/pynqbridge/19/recv"

    # create client and set username and password
    mqttc: mqtt.Client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    mqttc.username_pw_set(username, password)
    mqttc.connect(host=host)
    mqttc.loop_start()

    while True:
        data = get_data()
        string = json.dumps(data)

        logger.debug(string)

        msg = mqttc.publish(topic, string, qos=1)
        msg.wait_for_publish()


if __name__ == "__main__":
    main()
