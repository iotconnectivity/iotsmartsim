# Pod ENO SIM for Arduino

An Arduino library to interface with **Pod ENO SIM** Applications. Use Pod ENO SIM to delegate the SIM card some IoT Device functions:

* **Reduce IoT device Manufacturing costs**: The SIM will encrypt your device telemetry data. You can opt for cheaper hardware now.
* **Reduce IoT device On Boarding costs**: The SIM will download device settings from the cloud, the first time device is switch on.

This Arduino Library contains real case examples using `Arduino MKR GSM 1400` and `Arduino Nano Every`, in order to let you learn how to use Pod ENO SIM embedded capabilities.

Do you want a trial? **Contact us to order your SIM trial**: <a href="mailto:innovations@podgroup.com?subject=Order Pod ENO SIM trial kit">innovations@podgroup.com</a>

## Table of Contents

- [Description](#description)
- [Installation](#installation)
- [Usage](#usage)
- [Support](#support)
- [Contributing](#contributing)

## Description

Use the SIM-embedded applications interacts with the PodGroup's IoT Platform. Pod ENO SIM card includes the following embedded apps:

* **Zero Touch Provisioning**: The SIM will download your device configuration from cloud.
* **SIM-to-Cloud Encryption**: The SIM will upload your device telemetry to cloud.

The SIM-embedded apps uses TLSv1.3 (PresharedKey) for a secure interaction with PodGroup's IoT Platform.

Do you want to see real case scenarios? Check here 👇

👉 **Arduino Nano IoT Board**:

Turning Arduino Nano into an IoT board with PodGroup's IoT SIM and **SIM2Cloud Encryption** application

https://create.arduino.cc/projecthub/kostiantynchertov/tls-1-3-for-arduino-nano-649610

<a href="https://create.arduino.cc/projecthub/kostiantynchertov/tls-1-3-for-arduino-nano-649610"><img width="600" alt="arduino-nano-iot-board" src="https://hackster.imgix.net/uploads/attachments/1196958/_fsJWio6Efg.blob?auto=compress%2Cformat&w=900&h=675&fit=min"></a>

👉 **On Boarding an Arduino MKR GSM 1400**: 

Auto-configuring Arduino GSM boards with PodGroup's IoT SIM and **Zero Touch Provisioning**

https://create.arduino.cc/projecthub/kostiantynchertov/zero-touch-provisioning-based-on-tls-1-3-a07359

<a href="https://create.arduino.cc/projecthub/kostiantynchertov/zero-touch-provisioning-based-on-tls-1-3-a07359"><img width="600" alt="arduino-mkr-gsm-140-on-boarding" src="https://hackster.imgix.net/uploads/attachments/1294347/_GEOk3hWCaO.blob?auto=compress%2Cformat&w=900&h=675&fit=min"></a>

## Installation

Download a .ZIP package from GitHub and Import via `Sketch -> Include Library -> Add .ZIP Library`

## Usage

Once installed, go `File -> Examples -> PodEnoSim` to test the built-in examples. 

This Arduino Library contains Examples for `Arduino MKR GSM 1400` and `Arduino Nano Every`

## Support

Please [open an issue](https://github.com/podgroupconnectivity/PodEnoSim/issues/new) for support.

## Contributing

Please contribute using [Github Flow](https://guides.github.com/introduction/flow/). Create a branch, add commits, and open a pull request.