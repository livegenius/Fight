# Fighting Game Engine #

A fighting game engine inspired by airdashes using C++.
- Online multiplayer uses rollback netcode thanks to GGPO, although it still needs to be tested.
- Offline multiplayer is also supported.
- Both, keyboard and controllers, can be used in any combination.
- Lua scripting is used to define the behaviour of characters, among other things.
- A character framedata editor is still in progress.

![screenshot](https://user-images.githubusercontent.com/39018575/134964719-4d5d5d1b-7689-4565-83db-55375132d0b8.png)

## How to play ##

You can download the released binaries for Windows if you don't want to compile the game.

To configure your input, do the following while in the game:

For keyboard, press F9 to configure player 1 controls or F10 for player 2 controls.
For controller, press F7 to configure player 1 control or F8 for player 2 controls.
Then input the key, button, etc. you want in the follower order: 
Up Down Left Right A B C D. Yes, this is a four button fighter.
You can swap the controllers by pressing F6.

H displays the hitboxes and P pauses the game. While the game is paused you can press
space to advance one frame. F5 Changes the framerate. Escape quits the game.
Sorry, these keys are hardcoded for now.

Whenever you play, a replay is saved. To play this replay, enter playdemo in the command line.
You can host a game by entering the port in the command line, and you can join a game by
entering an IP address and the port, separated by spaces, not a colon.

### Example ###

> AFGEngine.exe playdemo
> 
> AFGEngine.exe 7000
> 
> AFGEngine.exe 127.0.0.1 7000

## Compiling ##

Clone the repo, init the submodules, run cmake with your preferred settings and build.
Set AFGE_BUILD_TOOLS to true if you want to build the developer tools.
The software is meant to be cross platform, but I haven't tried compiling it for linux yet.
It may work with some changes.
