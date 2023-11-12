
This project, MagicShogi, is a user-participation project that learns Shogi handicaps from scratch using deep reinforcement learning. It simultaneously learns seven types including Kyo drop, Angle drop, Fly drop, 4 pieces drop, 6 pieces drop, and even game. The strength of the inferior hand (first hand) is automatically adjusted to keep the winning percentage at 50%. Will AI discover a new jōseki or rediscover tactics such as Tsubo pushing through two pieces or Yonta transmission?

The game record, graph of the Go strength, sample games, etc. are published [here](http://www.new-owner.com/magicshogi/) . [Differences](http://www.new-owner.com/magicshogi/diff.html) with the [AobaZero](http://www.new-owner.com/aobazero/).

For those interested in participating in game record production, you can download the Windows executable file (64-bit only) from [here](https://github.com/magicmotion/magicshogi/releases). If you're using a machine with only a CPU, download _filename-cpu-only.zip_. For those with GPU, download _filename-opencl.zip_. Extract and run the contained click_me.bat.

If you're a Linux user, extract the file _magicshogi-1.0.tar.gz_, run make command, and then execute `./bin/autousi`.

You can also have fun playing Shogi at ShogiDoko and ShogiGUI.

Aoba's game drop is also compatible with ShogiGUI. Please check the 'Send all moves in game analysis, deliberation mode' in 'Tools (T)', 'Options (O)'.

For developers interested in compiling the program themselves, please check the [compilation instructions](compile.txt).

A brief introduction about the game can be found [here](http://www.new-owner.com/magicshogi/).

This project uses several open source projects to work properly:
 - [AobaZero](https://github.com/kobanium/aobazero)
 - [Leela Zero (Go)](https://github.com/leela-zero/leela-zero)
 - [LCZero (Chess)](https://github.com/LeelaChessZero/lczero)

The USI engine aobak is GPL v3. Everything else is in the public domain.

More detailed licensing information can be found inside licenses in `magicshogi-1.0.tar.gz`. Please view the [short license](license.txt) for more details.