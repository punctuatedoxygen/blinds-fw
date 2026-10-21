# Design

## Goal

Drive three Somfy RTS blinds from a battery board that lasts months, with no
wall plug. The board sits near the blinds; the home server is too far away
for 433 MHz.

## Split of work

The **server** keeps the brain. It already computes sunrise and sunset and
runs the schedule. It publishes two retained MQTT messages for the board:

- `plan`: the next event per blind (epoch time + direction), plus the
  current time so the board can correct its clock.
- `cmd`: a manual command with an id, for "close blind 1 now" from the
  `blinds` CLI on a laptop. The server clears it once the board acks that
  id, so a retained command never runs twice.

The **board** is a sleepy actuator. Each wake:

1. Connect to Wi-Fi (static IP, cached channel and BSSID in RTC memory).
2. Read `plan` and `cmd`, sync the clock.
3. Send any due frames. Wi-Fi goes off before transmitting.
4. Publish state, battery voltage and the acked command id.
5. Put the CC1101 to sleep, deep sleep until the next event or the poll
   interval, whichever is sooner.

If Wi-Fi fails it runs the last plan it has, on its own clock.

## Manual commands from the CLI

The CLI can't wake the board. In deep sleep the Wi-Fi radio is off, so no
packet reaches it. A command is queued instead: `blinds 1 down` publishes
`cmd`, prints "queued, runs by 9:42 pm", and with `--wait` blocks until the
board acks. Latency is the poll interval (5 min) plus a few seconds to
reconnect.

Two ways to get instant response were ruled out:

- **Staying on Wi-Fi in light sleep.** It answers in about a second, but on
  this hardware it costs about as much as the board's idle overhead, and
  Arduino builds don't enable automatic light sleep by default.
  `WiFi.setSleep(true)` is modem sleep, with the CPU awake at 20-30 mA. Worth
  an experiment once sleep current is measured, not the default.
- **Wake-on-radio** through the CC1101. It would need a 433 MHz transmitter
  at the laptop.

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
- **Weak receivers get repeats**: one of the three needs about 8 frames.
- **No credentials or rolling codes in the repo** (`secrets.h` is ignored).

## Power

The dev board as shipped can't do this. Its AMS1117 regulator draws about
5-10 mA doing nothing and the power LED a few more, which drains an 18650 in
2-4 weeks whatever the firmware does. The AMS1117 also needs about 1 V of
headroom, so on a sagging Li-ion cell it browns out during Wi-Fi bursts.
**The regulator gets bypassed and the LED comes off. That's required.**
Options, in order of preference:

1. 18650 Li-ion + a low-quiescent LDO rated 500 mA or more (RT9080,
   ME6211). Real voltage curve, so the battery level means something. Not
   the HT7333 or MCP1700: 250 mA is under Wi-Fi peaks.
2. LiFePO4 straight to 3V3. Simple, but the voltage is flat until empty and a
   fresh cell sits at the chip's 3.6 V limit.

Either way, a 470 µF bulk cap for Wi-Fi spikes. Measure sleep current on a
spare board first, then again after the changes.

## Tradeoffs accepted

- Manual commands from the CLI wait for the next wake, up to 5 min.
- Physical remote presses are not tracked: the board's receiver is off.

## Steps

1. RTS encoder + tests against Somfy_Remote_Lib. **Done.**
2. CC1101 setup + RMT transmit.
3. Sleep/wake loop, NVS, MQTT plan/cmd/ack, lockout.
4. Flash a spare ESP32, measure sleep current.
5. Buy the battery and regulator, bypass the AMS1117, remove the LED,
   measure again.
6. Move the CC1101, re-pair the three blinds as new remotes.
7. Server side: the scheduler publishes `plan`; the laptop `blinds` CLI
   switches from talking HTTP to ESPSomfy to queuing `cmd` over MQTT.
8. Photos, video, write-up.
