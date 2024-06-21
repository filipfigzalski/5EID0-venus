import paho.mqtt.client as paho


class Broker:
    """
    Creates an object that subrices to MQTT topics and triggers
    a costum method upon receiving  message.
    ### Args:
    - host: The adress of the host.
    - username: username credential.
    - password: password credential.
    - topicSubList (list [str]): List of topics to subsrcibe to.
    - messageHandler (function): Triggered when a message is received.
    """

    def __init__(
        self,
        host,
        username: str,
        password: str,
        topicSubList: list[str],
        messageHandler: callable,
    ) -> None:
        self.client = paho.Client(2)
        self.client.username_pw_set(username, password)
        self.client.on_message = messageHandler
        if self.client.connect(host=host) != 0:
            raise RuntimeError("Couldn't connect to the mQTT host")
        try:
            # Check for a succesful subscription
            for topic in topicSubList:
                if self.client.subscribe(topic)[0] != paho.MQTT_ERR_SUCCESS:
                    print(f"Couldn't subscribe to the topic: {topic}")
            # Let an unexperienced user know how to kill the process
            print("Press CTRL+C to exit...")
            self.client.loop_start()
        except Exception as error:
            print(error)
        # finally:
        #     print ("Disconnecting from the MQTT broker")
        #     self.client.disconnect()
