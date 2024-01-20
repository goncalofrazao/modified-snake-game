# Lizards & Roaches & Wasps

Lizards and roaches and wasps is a modified version of the snake game.

Lizards are human controlled clients and are equivalent to snakes.

Roaches and wasps are automatic clients but they must be launch by a user.

# Game Rules

There are 3 types of things in the game: lizards, roaches and wasps.

Lizards are the players and the goal is to reach 50 points eating roaches.

The roaches have a number between 1 and 5 that is the points you earn if you eat the roach.

Wasps are characterized by a # and if you bump into them you lose 10 points.

The game can be played with many players at the same time.

Good luck!

# Build

## To build all

```bash
    make all
```

or

```bash
    make
```

## To build server

```bash
    make lizardsnroaches-server
```

## To build lizard client

```bash
    make lizard-client
```

## To build wasps client

```bash
    make wasps-client
```

## To build roaches client

```bash
    make roaches-client
```

# Play

## Launch server

```bash
./lizardsnroaches-server <lizards-port> <wasps-and-roaches-port> <display-port>
```

## Launch wasps and roaches

You can launch the roaches and wasps clients you want.
Carefull that your wasps and roaches can only occupy 1/3 of the field.
Each client of wasps/roaches you launch will generate between 1 and 10 wasps/roaches randomly.

Note: If you try to launch more than allowed the server will not accept and the roaches/wasps client will auto exit.

### Launch roaches

```bash
./roaches-client <server-ip> <wasps-and-roaches-port>
```

### Launch wasps

```bash
./wasps-client <server-ip> <wasps-and-roaches-port>
```

## Launch player

```bash
./lizard-client <server-ip> <lizards-port> <display-port>
```
