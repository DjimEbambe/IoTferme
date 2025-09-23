# Pairing & Enrollment Workflow

The edge CLI (Pi5) drives pairing over the USB CDC link to the gateway. Example flow:

1. **Open serial console** on the Pi5:
   ```bash
   picocom -b 115200 /dev/ttyACM0
   ```
2. **Start pairing window** (120 s default):
   ```json
   {"act":"pair_begin","window_ms":120000}
   ```
3. **Put device into pairing mode.**
   - For S3/C6 nodes, connect via USB Serial and issue `pair <gateway-mac>` (custom script) or push the physical pairing button if available.
   - When discovered, send the device MAC + asset mapping to the gateway:
     ```json
     {"act":"pair_commit","mac":"A4:B5:C6:D7:E8:F9","asset":"A-PP-03"}
     ```
4. **Verify** the whitelist:
   ```json
   {"act":"pair_list"}
   ```
   Gateway replies with `{ "type": "pair_list", "peers": [...] }`.
5. **Stop pairing window** (optional timeout):
   ```json
   {"act":"pair_remove","mac":"..."}
   ```

For automated commissioning, wrap the JSON frames with COBS (or rely on the provided Pi5 agent which already handles framing and MessagePack encoding).

