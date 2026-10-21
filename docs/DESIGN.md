# Design

## Goal

Drive three Somfy RTS blinds from a battery board that lasts months, with no
wall plug. The board sits in the bedroom on the second floor; the home server
is in the basement, too far for 433 MHz.

## Split of work

The **server** keeps the brain. It already computes sunrise and sunset and
runs the schedule. It publishes two retained MQTT messages for the board:

- `plan`: the next event per blind (epoch time + direction), plus the
  current time so the board can correct its clock.
- `cmd`: a manual command with an id, for "close the bay now" from the
  phone or laptop. The server clears it once the board acks that id.

The **board** is a sleepy actuator. Each wake:

1. Connect to Wi-Fi (static IP, cached channel and BSSID in RTC memory).
2. Read `plan` and `cmd`, sync the clock.
3. Send any due frames. Wi-Fi goes off before transmitting.
4. Publish state, battery voltage and the acked command id.
5. Put the CC1101 to sleep, deep sleep until the next event or the poll
   interval, whichever is sooner.

If Wi-Fi fails it runs the last plan it has, on its own clock.

## Rules that came out of review

- **Rolling codes live in NVS**, committed before every transmit. RTC memory
  alone loses them on a reset, and a blind ignores codes it has seen.
- **The last acked command id also lives in NVS**, so a reset can't replay it.
- **Cap lateness**: an event more than ~30 min overdue is skipped, not run.
- **Hold CSN high in deep sleep** (GPIO 5 is not an RTC pin, needs
  `gpio_deep_sleep_hold_en` or a pull-up). Low CSN wakes the CC1101 to
  about 1.7 mA.
- **Low-battery lockout** near 3.0 V: stop using Wi-Fi and sleep, instead of
  brownout reboot loops that over-drain the cell.
- **The internal clock drifts by percent**, so every successful wake resyncs.
  Offline it gets worse by the hour.
- **Transmit through the RMT peripheral**, not a bit-banged pin with
  interrupts off.
- **Weak receivers get repeats**: Corner Win 2 needs about 8 frames.
- **No credentials or rolling codes in the repo** (`secrets.h` is ignored).

## Power

Open question until sleep current is measured on a spare board. The dev
board's USB chip, regulator and power LED likely idle around 0.5 to 1 mA,
which alone sets the battery life. Options, in order of preference:

1. 18650 Li-ion + a low-quiescent LDO rated 500 mA or more (RT9080 class).
   Real voltage curve, so the battery level means something.
2. LiFePO4 straight to 3V3. Simple, but the voltage is flat until empty and a
   fresh cell sits at the chip's 3.6 V limit.

Either way, a 470 µF bulk cap for Wi-Fi spikes, and the power LED comes off.

## Tradeoffs accepted

- Manual commands wait until the next wake (poll interval, 5 to 30 min, his
  pick).
- Physical remote presses are not tracked: the board's receiver is off.

## Steps

1. RTS encoder + tests against Somfy_Remote_Lib. **Done.**
2. CC1101 setup + RMT transmit.
3. Sleep/wake loop, NVS, MQTT plan/cmd/ack, lockout.
4. Flash a spare ESP32, measure sleep current.
5. Pick and buy the battery and regulator.
6. Move the CC1101, re-pair the three blinds as new remotes.
7. Server side: publish `plan` and `cmd` from the existing `blinds` scheduler.
8. Photos, video, write-up.
