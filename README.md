# MidiControl

Version 1.0

MidiControl is a small background service that connects to a MIDI device, accepts sounds and maps these
sounds to configurable actions. It is written in C99 using Visual Studio and the Windows API.

#### Table of contents
* [Introduction](#midicontrol)
* [Configuration](#configuration)
* [Building](#building)

## Configuration

The configuration of MidiControl is done via a simple INI configuration file. A sample config.ini is
already included.

#### Adding actions
Currently there are only three predefined actions:
| Index | Action         |
|-------|----------------|
|   0   | Previous track |
|   2   | Skip track     |
|   1   | Play / Pause   |

To add more actions, simply add more values to the switch-case statement in line 190 and then you can do
whatever you want.

## Building

The building should be relatively easy thanks to the supplied Visual Studio solution file.
Since MidiControl uses several elements from the Windows API, it is currently only compatible
with Windows 8.1+.
