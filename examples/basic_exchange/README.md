# PeerDrop – Basic Exchange Example

This example demonstrates the simplest PeerDrop interaction between two ESP32 devices.

---

## 1. Setup

### Hardware
- 2 × ESP32 boards
- BLE enabled
- No additional peripherals required

---

## 2. Flashing

1. Open `peerdrop.ino` in Arduino IDE
2. Configure device identity in `config.h`
3. Flash the same firmware to both ESP32 devices

---

## 3. How It Works

- Both devices advertise the PeerDrop BLE service
- Both devices scan for nearby peers
- When proximity conditions are met:
  - A one-time data exchange occurs
  - Peer MAC is stored in NVS
- Further exchanges with the same device are blocked

---

## 4. Expected Behavior

- Exchange happens only once per peer
- Restarting the device does NOT reset exchange history
- Clearing NVS resets exchange state

---

## 5. Notes

- This example uses unencrypted payloads
- Intended for learning and testing
- See `docs/security.md` for limitations

---

## 6. Next Steps

- Add encryption
- Add user confirmation
- Implement dynamic UUIDs
