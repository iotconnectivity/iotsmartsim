# IoT Sm@rtSIM for Arduino

An Arduino library to interface with **IoT Sm@rtSIM** Applications. Use IoT Sm@rtSIM to delegate the SIM card some IoT Device functions:

* **Reduce IoT device Manufacturing costs**: The SIM will encrypt your device telemetry data. You can opt for cheaper hardware now.
* **Reduce IoT device On Boarding costs**: The SIM will download device settings from the cloud, the first time device is switch on.

This Arduino Library contains real case examples using `Arduino MKR GSM 1400` and `Arduino Nano Every`, in order to let you learn how to use IoT Sm@rtSIM embedded capabilities.

## Table of Contents

- [Description](#description)
- [Installation](#installation)
- [Usage](#usage)
- [Support](#support)
- [Contributing](#contributing)

## Description

Use the SIM-embedded applications interacts with IoT Platform. IoT Sm@rtSIM card includes the following embedded apps:

* **Zero Touch Provisioning**: The SIM will download your device configuration from cloud.
* **SIM-to-Cloud Encryption**: The SIM will upload your device telemetry to cloud.

The SIM-embedded apps uses TLSv1.3 (PresharedKey) for a secure interaction with IoT Platform.

Do you want to see real case scenarios? Check here 👇

👉 **Arduino Nano IoT Board**:

Turning Arduino Nano into an IoT board with IoT SIM and **SIM2Cloud Encryption** application

👉 **On Boarding an Arduino MKR GSM 1400**: 

Auto-configuring Arduino GSM boards with IoT SIM and **Zero Touch Provisioning**

## Installation

Download a .ZIP package from GitHub and Import via `Sketch -> Include Library -> Add .ZIP Library`

## Usage

Once installed, go `File -> Examples` to test the built-in examples. 

This Arduino Library contains Examples for `Arduino MKR GSM 1400` and `Arduino Nano Every`

## Support

Please [open an issue](https://github.com/podgroupconnectivity/PodEnoSim/issues/new) for support.

## Contributing

Please contribute using [Github Flow](https://guides.github.com/introduction/flow/). Create a branch, add commits, and open a pull request.
