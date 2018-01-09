# Fugitap display manager

[![Build Status](https://travis-ci.org/tunebird/fugitap.svg?branch=master)](https://travis-ci.org/tunebird/fugitap)

A tap display designed for the Fûgıdaıre kegerator. Runs on an esp8266 with the
arduino framework connected to a cheap 2.4in colour TFT display.

The device grabs beer names, style, abv and other info from a JSON source hosted
on the web and displays it to the user. The device can optionally read a flow
meter to display the fill status of the keg that's being monitored, and send
temperature and humidity readings on an MQTT network.

## Getting Started

Copy the `settings_template.h` file to `settings.h` and enter your own user
settings. The file is set to be ignored by git so you don't accidentally commit
your WiFi or MQTT credentials to github :)

OTA updates are enabled. After the first build, simply add the following line
to the `platformio.ini` file to enable remote updates :

```
upload_port = 192.168.0.17
```

(relace by the device's IP, obviously. It will display briefly during boot up)

If you want to upload to several boards without changing the configuration every
time, you can also call the following command from a terminal, or add it as a
build task to your editor :

```
pio run -t upload --upload-port 192.168.0.26
```

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/your/project/tags). 

## Authors

* **Andrew Watson** - *Initial work* - https://github.com/tunebird


## License

MIT License

## Acknowledgments