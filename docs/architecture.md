# PeerDrop – Architecture Overview

PeerDrop is a decentralized, offline peer-to-peer data exchange system built on ESP32 using Bluetooth Low Energy (BLE).

The system is designed with **privacy, proximity, and simplicity** as first principles.

---

## 1. High-Level Architecture

PeerDrop operates without:
- Internet
- Cloud servers
- Central authority

Each ESP32 device acts as both:
- **Advertiser**
- **Scanner**
- **Data provider**

This creates a fully peer-based architecture.

---

## 2. Core Components

### 2.1 BLE Advertising
- Devices broadcast a minimal BLE advertisement
- Contains:
  - Service UUID
  - Device role identifier
- No personal data is exposed in advertisements

---

### 2.2 BLE Scanning
- Devices continuously scan for nearby PeerDrop nodes
- RSSI values are collected for proximity estimation
- Only devices within a defined RSSI threshold are considered valid peers

---

### 2.3 Proximity Validation
- RSSI is converted into approximate distance
- Exchange is allowed only if:
  - Distance < configured threshold
  - Signal is stable (anti-spoofing measure)

This prevents long-range or relay-based attacks.

---

### 2.4 Secure Data Exchange
- Data is exchanged using BLE GATT characteristics
- Exchange rules:
  - One-time exchange per device
  - MAC-based deduplication
  - No continuous tracking

---

### 2.5 Local Storage (NVS)
- Exchanged peer identifiers are stored in ESP32 NVS
- Prevents repeated or spam exchanges
- Data persists across reboots

---

## 3. Security Design Principles

- Privacy-first (no broadcasted personal data)
- Offline-by-default
- Minimal attack surface
- No dependency on third-party services

---

## 4. Future Enhancements

- Encrypted payload exchange
- Dynamic UUID rotation
- Trust scoring based on signal behavior
- Optional user confirmation layer

---

## 5. Summary

PeerDrop is intentionally simple, offline, and proximity-bound.
Security is achieved through **physical presence**, not cloud trust.
