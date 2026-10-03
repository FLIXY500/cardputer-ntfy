# cardputer-ntfy

A simple way to monitor and send messages to ntfy.sh topics on M5 Cardputer. Designed as a way to monitor server data leak notifications from oboi-dlp but the user can easily set the ntfy topic in the interface and use as an ntfy pager for any topic.

**SD CARD REQUIRED** — Yes, this app saves alert text to a file on SD card and it is required.  
**SD images** — The optional SD card images should be copied manually to your SD card root directory, but cardputer ntfy will use ASCII art if they are not found.

Install via M5Burner.

## Features

- **Receive alerts** — Polls an ntfy topic via SSE and displays new notifications
- **Send messages** — Press **S** to compose and send a message to the configured topic
- **Append alerts** to `/alerts.txt` on SD card, keeping the last 20
- **Beep** on new alert
- **Save/load settings** to `/oboi.cfg` on SD — no need to re-enter every time

## Fork

Forked by Flixy500

## Menu

Press keys on the main screen:

| Key | Action |
|-----|--------|
| A   | Connect WiFi |
| B   | Set Topic |
| C   | Start Monitoring |
| P   | Set Password |
| S   | Send Message |

While monitoring, press **X** to exit back to the menu.

## Send Message

Press **S** on the main screen to enter send mode. The app connects to the configured topic and lets you type a message. Press **Enter** to send, **X** to cancel. The send uses the same topic you're monitoring by default — change the topic via **B** first if you want to send elsewhere.

## Installing

1. Copy the `.bin` file to M5Burner and flash to your Cardputer
2. Insert an SD card
3. Turn on the Cardputer — set your **SSID** (A), **WiFi password** (P), and **ntfy topic** (B)
4. Press **C** to start monitoring

## File Structure on SD

- `/oboi.cfg` — saved credentials and topic
- `/alerts.txt` — alert history (last 20 entries, circular buffer)

## Credits
Fork by Flixy500
By Scot D Forshaw

Buy him a coffee :)
