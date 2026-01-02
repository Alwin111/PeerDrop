# PeerDrop – Security Overview

PeerDrop is designed as a **privacy-preserving, offline system**.
Security decisions are enforced locally on each ESP32 device.

---

## 1. Threat Model

PeerDrop considers the following threats:

- Unauthorized BLE scanners
- Long-range BLE relay attacks
- Repeated spam exchange attempts
- Passive tracking via advertisements

---

## 2. Security Mechanisms

### 2.1 Minimal BLE Advertisements
- No personal data in advertisements
- Only service UUIDs are broadcast
- Prevents passive data harvesting

---

### 2.2 Proximity-Based Access Control
- RSSI is used to approximate physical distance
- Exchanges are allowed only below a defined distance threshold
- Reduces relay and amplification attacks

---

### 2.3 One-Time Exchange Enforcement
- Each peer MAC address is recorded after exchange
- Repeat exchanges are blocked
- Prevents spam and replay-style abuse

---

### 2.4 Local-Only Storage
- All peer data stored in ESP32 NVS
- No cloud, no external logging
- Data remains under user control

---

## 3. Known Limitations

- RSSI-based distance estimation is approximate
- MAC randomization may reduce long-term deduplication
- BLE is inherently observable at RF level

These are acknowledged design trade-offs.

---

## 4. Responsible Disclosure

If you discover a vulnerability:
- Do NOT open a public issue
- Contact the maintainer privately

See `SECURITY.md` in the root directory.

---

## 5. Security Philosophy

PeerDrop does not aim for absolute cryptographic secrecy.
Instead, it enforces **physical presence + minimal exposure** as its primary defense.
