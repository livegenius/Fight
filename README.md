# Fighting Game Engine #

A fighting game engine inspired by airdashers.

![screenshot](https://user-images.githubusercontent.com/39018575/134964719-4d5d5d1b-7689-4565-83db-55375132d0b8.png)

## How to play ##

To change your controls, do the following in-game:

For controller, press F7 to configure player 1 control or F8 for player 2 controls.
For keyboard, press F9 or F10 to configure player 1 and player 2 controls, respectively.

After you've pressed the F key, input the key, button, etc. you want to bind to: 
Up Down Left Right A B C D. - They must be inputted in that order.
If there are two controllers, you can swap the controllers by pressing F6.

The H key displays the hitboxes and P pauses the game. While the game is paused you can press
space to advance one frame. F5 Changes the framerate. Escape quits the game.
F1 and F2 can save or load a savestate, respectively.
Sorry, but these keys are hardcoded for now. 

Whenever you play, a replay is recorded to a file. To play this replay, enter playdemo in the command line.
You can host a game by entering the port in the command line, and you can join a game by
entering an IP address and the port, separated by spaces, not a colon.

### Example ###

> Fight.exe playdemo
> 
> Fight.exe 7000
> 
> Fight.exe 127.0.0.1 7000

## Compiling ##

Clone the repo, init the submodules, run cmake with your preferred settings and build.
For example:

```
git clone --recursive [insert repo link here] fight
cd fight
mkdir build
cmake -S. -Bbuild -GNinja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The only external dependency that is required is the Vulkan SDK.
Set AFGE_BUILD_TOOLS to true if you want to build the developer tools.
~~It can be compiled for Linux~~. It hasn't been actively developed for
linux, so it may require a few changes.
