# RAGE IntelliBuild Patcher
- #### Rockstar Advanced Game Engine IntelliBuild Patcher

## Purpose
This program will extend your free trial of IntelliBuild 4.0, the standard one for building any RAGE Assets like scripts or shaders so you can fully utilize your CPU without having to deal with the crappy standalone mode when your trial runs out.

This isn't really a big patch, as if you forget to run this patcher before your trial runs out, IntelliBuild will refuse to listen to you and you will have to manually remove the registry and reinstall. More information on how to do that down

## Features
* Allow to run on startup feature - A optional feature that will run the patch everytime Windows starts, pretty much killing the need to manually patch it yourself, however if you don't often restart your device, this might be futile.

## To-Do List
* Add a daily rountine system feature which will allow you to give the patcher a daily routine to follow, this would mean the patcher will have to be on always to keep up on schedule.
* Figure out a way to fix expired instances.

## How to remove/reset expired date
1. Go to registry
2. Go to `Computer\HKEY_CLASSES_ROOT\WOW6432Node\Interface\{23DE9F4B-25F9-4163-BF69-01639BB2B8BA}` and delete the whole directory.
3. Reinstall IntelliBuild 4.0

And you'll see it will be completely reset and you can now run the patcher continously without any problems as long as it doesn't go past.