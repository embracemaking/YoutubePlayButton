# YoutubePlayButton

This is the repository for the YouTube play button award with live subscriber and view count + RGB effect on the play button model.

![Render1](https://user-images.githubusercontent.com/109498075/179452693-4a783aa7-ede0-47d0-9301-9efcbc31fdde.JPG)

Video instructions here:
>

3D models and digital resources here:
> https://www.printables.com/model/243806-rgb-youtube-play-button-award-with-live-subview-co

You will need the following hardware:
- WEMOS D1 MINI
- LOLIN 7 RGB D1 MINI SHIELD
- MAX7219 8x8 4 in 1 PANEL

You will also need:
- 3D printer to print some parts.
- 8 x 10 picture frame
- Hobby Knife
- Paper printer

Big shoutout to HackMakeMod for the original video and original source code that I forked this from. I've modified their auto generated code from socialgenius.io to use the 4 panel MAX7219 instead of the 8 panel so it can fit in the picture frame. I also modified the code to run the LOLIN RGB shield and changed the formatting of text / numbers to fit the 4 panel matrix. 

You will need to visit socialgenius.io and sign up for an account. Follow the instructions in my YouTube video to see which part of the code you need to copy from their auto-generated code so that their server can fetch your stats with the proper project name. 

You can find their original video here:
> https://www.youtube.com/watch?v=16gZ2CX7qB4
They also have a nice written guide here with details on installing the required libraries 
> https://hackmakemod.com/blogs/projects/diy-kit-guide-building-your-own-subscription-clock

In addition to the ones listed in their writeup, you will also need the Adafruit NeoPixel library.


