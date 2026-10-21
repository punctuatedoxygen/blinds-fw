# blinds-fw

Battery firmware for an ESP32 + CC1101 board that drives Somfy RTS blinds.
The board sleeps, wakes for scheduled moves or to pick up commands, sends
the radio frames, and sleeps again. A home server keeps the schedule.

## Status

- `lib/rts`: Somfy RTS frame builder and pulse encoder (no Arduino, runs anywhere).
- Next: CC1101 setup, transmit through the RMT peripheral, sleep/wake loop, MQTT.

## Tests

`make test` builds the encoder on a laptop and checks it, waveform for waveform,
against [Legion2/Somfy_Remote_Lib](https://github.com/Legion2/Somfy_Remote_Lib)
(Apache 2.0). That library is cloned at test time, not copied into this repo.
It runs about 21,000 cases: sample addresses, every command, edge
rolling codes and random ones.

Credit for the protocol work goes to the pushstack write-up and
Legion2/Somfy_Remote_Lib.
