# MCP C SDK

## Transport Protocols

- MQTT v5

## Dependencies

[MQTT C Library](https://github.com/eclipse-paho/paho.mqtt.c)
```shell
$ git clone https://github.com/eclipse-paho/paho.mqtt.c
$ cd paho.mqtt.c && mkdir build && cd build
$ cmake .. && make && sudo make install
```

[cjson](https://github.com/DaveGamble/cJSON)
```shell
$ git clone https://github.com/DaveGamble/cJSON
$ cd cJSON && mkdir build && cd build
$ cmake ..&& make && sudo make install
```