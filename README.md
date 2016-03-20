# PrettyPrinny
Pretty Prinny is an effort to polish Disgaea PC back to the quality standards of the original console release. It draws on my extensive experience in OpenGL to fix some underlying problems that prevent the game from running at proper performance or image quality standards. Additionally, it touches on general usability issues such as input, and includes features that will make modding easier on the community.

## Mini-FAQ <sup>(See the [Steam Guide](https://steamcommunity.com/sharedfiles/filedetails/?id=641932497) for full documentation)</sup>
### How do I install this?
Unzip it and copy it to your Disgaea PC folder. You can find your Disgaea PC folder at `%PROGRAMFILES%\Steam\steamapps\common`

If you are NOT using a Steam controller, delete the `Pad.cfg` file.

### How do I uninstall this?
Run `UninstallPrettyPrinny.bat`

### After installing this, my controller no longer works properly.
Go into `PrettyPrinny.ini` and change the key **WrapXInput**, which is located in the `[PrettyPrinny.Input]` section to **"false"**. Your controller should work again.

### I get a lot of black artifacting while in the game.
PrettyPrinny was tested using nVidia **361.75** drivers. If you are running any other drivers than that, please change to this driver version. Other driver versions are currently under testing.

### How can I be sure it is working?
You'll get a PSN trophy sound on achievement unlock. You can force this sound to occur by pressing  <kbd>CTRL</kbd> + <kbd>SHIFT</kbd> + <kbd>T</kbd>.


## Configuration
### [PrettyPrinny.Render]
#### SceneResX
Uncaps the game's horizontal resolution (FULLSCREEN MODE)  
Possible values = 640 - 4096  
Default = 1920  
The game defaults to 720p (1280)  

#### SceneResY
Uncaps the game's vertical resolution (FULLSCREEN MODE)  
Possible Values = 360 - 2160  
Default = 1080  
The game defaults to 720p (720)  

#### SwapInterval
Controls VSYNC at the application level

Values > 0 = Normal VSYNC (divide by refresh rate)  
0          = No VSYNC  
Values < 0 = Adaptive VSYNC (divide by refresh rate)

Do not use this. Leave it set to 0 and force VSYNC at the driver level. Otherwise performance will suck on almost all drivers.

#### RemoveFringing
Removes alpha fringing artifacts around sprites.
Default = True

## Important note about Steam Controllers
PPrinny v 0.1.0 adds support for Steam controllers natively (e.g. no keyboard / mouse emulation mode necessary). However, in addition to setting up the controller in the Steam Big Picture UI to work as a standard gamepad, you will need to replace pad.cfg with the file in the 0.1.0 release notes.

**Failure to comply will result in unusable menus; the cursor will constantly scroll up.**

## Troubleshooting
The project is rather special in that it is the first mod I have done that uses OpenGL (but by no means the first software I have written that uses OpenGL). I expect things to be rocky at first, and would ask that any issues encountered be reported in the development thread [here](http://steamcommunity.com/app/405900/discussions/4/412448792361194752/).

When reporting a problem, logs/PrettyPrinny.log, logs/steam_api.log and logs/OpenGL32.log are useful information for me. If you could paste those to [Pastebin](http://pastebin.com/) and then post a link in the thread, I will do my best to track the problem down.


## Contribute
Modding OpenGL software is a major chore, and nothing about this project is simple. My hope is that the time and effort spent here will pay dividends in future Disgaea releases and/or OpenGL-based games _(all two of them?)_ in general.

If you would like to donate, I have a Pledgie campaign dedicated to this project:

&nbsp;&nbsp;&nbsp;&nbsp;<a href='https://pledgie.com/campaigns/31259'><img alt='Click here to lend your support to: Pretty Prinny and make a donation at pledgie.com !' src='https://pledgie.com/campaigns/31259.png?skin_name=chrome' border='0' ></a>
