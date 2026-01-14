#pragma once

#ifdef USE_NIMBLE
#include <NimBLEDevice.h>
using BleDevice = NimBLEDevice;
using BleScan = NimBLEScan;
using BleAdvertisedDevice = NimBLEAdvertisedDevice;
using BleScanResults = NimBLEScanResults;
#else
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
using BleDevice = BLEDevice;
using BleScan = BLEScan;
using BleAdvertisedDevice = BLEAdvertisedDevice;
using BleScanResults = BLEScanResults;
#endif
