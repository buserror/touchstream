touchstream - mouse/keyboard network sharing utility
(C) Michel Pollet <buserror@gmail.com>

Introduction
--------
_touchstream_ is a program that allows you to share a keyboard & mouse over 
a number of machines, on a LAN, typicaly.

Imagine having a mac and a linux machine, with their own screens; you 
can use the Mac mouse and keyboard on the linux box too by moving the 
mouse pointer to a max screen edge, and it starts moving on the linux screen.

Also, the clipboard is exchanged as you switch back & forth.


Rationale
--------
The goal of _touchstream_ was to replace synergy -- at least on my desk. 
Synergy has various issues, notably on the bloat department; "powertop" 
flags it as one of the most used/busy process on the machine, even tho 
it technicaly hasn't a lot to do, furthermore the codebase is bloated 
too, and the implementation uses countless locks, mutexes and other 
distasteful things that adds a lot of overhead, and that one doesn't 
strictly need if designing the execution path properly.

The other reason is that synergy stopped working on my configuration 
recently for me, and I realised about it's code bloat when I went to 
try to fix it. 

Obviously, quite a bit of the platform specific bits of touchstream 
are copied straight out of synergy's source code, and not only deserve 
the credit, but also share their GPL licence. In many cases, the code 
has been entirely reworked tho, and I haven't taken care of copying 
the attribution and copyrights for individual snipets.

The actualy "core" of touchstream is entirely original, and thats 
why the files uses my generic GPL header bits.

Status
--------
Anyway, touchstream is C99, doesn't use a single mutex, and works for 
now on OSX as a server (read, the screen that has the keyboard and mouse) 
and as a client of a recent xorg.

It also can work without a "client" process by talking directly to 
remote xorg servers, as long as 1) the server allows tcp connections, 
and 2) you have disabled access control (xhost +).

Clearly it lacks some of the features synergy has (windows support, 
non-latin keyboard support etc) but it also a LOT simpler, and uses 
a fraction of the resources.

