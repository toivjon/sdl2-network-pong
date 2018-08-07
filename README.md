# sdl2-network-pong

A SDL2 implementation of the classic Pong game with a simple network support.

Contains a single mode where two human players can play againts each other.

Contains a single scene.

## Features
This Pong implementation contains the following features.
* A support for TCP and UDP transport protocol.
* Each launched game instance acts either as a server or client.
* Each game lasts until either player receives the 10th point.
* Both paddles are controlled by human players.
* Ball velocity is increased on each hit with a paddle.
* Ball movement is being stopped for ~1 second after each reset.
* Ball direction is randomized from four different direction after each reset.
* Paddles are returned to their default position after each reset.

## External Dependencies
This implementation has external dependencies on the following libraries:
* SDL2
* SDL2net

## Compilation
Easiest way to compile the code is to use the provided Makefile.

Makefile may require some modifications based on the compilation environment.

## Usage
Game startup syntax is as following.

**pong.exe [transport-protocol] [host]**

An example to start a TCP server.

**$ pong.exe tcp**

An example to start a TCP client and connect to a localhost.

**$ pong.exe tcp localhost**

## Notes
There are some major notes and bugs in the current implementation:
* Implementation uses remote lag as the latency compensation mechanism.
* Implementation has some minor timing problems (shown as jittering) at the start.
* Implementation does not include reliability control for UDP.

## Screenshots
![alt text](https://github.com/toivjon/sdl2-network-pong/blob/master/pong.png "Pong")
