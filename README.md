# Important Notice
If you are reading this and are willing to offer relocation to european country to a developer of this application I would be very interested. 
You will get an extremely grateful and dedicated c++ developer working for you.

Due to my country's invasion of Ukraine both my future and stability of the service cannot be certain.
I've made some inroads about people maybe keeping my work alive so, if the worst comes to pass, ask the maintainer of https://www.ficlab.com/ whether the backup can be put up and how soon. 

P.S. It's been a fun run, but I feel like I am just going to vanish soon, First from the internet and from this world soon after. 
(this is not a suicide notice, the timeframe covered mostly means Russia's likely disconnection from the internet)


Update: draft is official now. I may be gone any day. Repo has dockerfile to build server version for linux but anyone interested in hosting a local server instance on windows can download this: https://drive.google.com/file/d/1nSEtLEh-2yOiB0Bj8nagLaVlRWySBF1t/view?usp=sharing

You run this with feed_server.exe. Wait till the process finishes gobbling up resources (it will write initial temprorary files so that it starts faster next time) and then redirect serverIp in flippers' settings/settings.ini to 127.0.0.1. note: this will unpack into 12 gigabytes folder and will create 1 more gigabyte of temp files.  Requires a moderately modern PC but I think anything recent~ish should run it well. After redirecting your flipper to that you will be able to access all of your tagged fics and find new stories to read out of the snapshot it has although, obviously, until I am gone, it's simpler to just work as normal 


# Flipper
An alternative take on search on fanfiction.net

Flipper is written in Qt5, partially with QML, and is supposed to be built with QBS build system.
The database format is SQLITE3.

Manual can be found at: https://docs.google.com/document/d/1RAquRcfpKMQQGEBKUeszk9o9_czIPQ7sysgqXtwcnkI/edit?usp=sharing

If you have questions, email me at ficflipper@gmail.com or ping me at https://discord.gg/AdDvX5H

UPD: I now have a patreon: https://www.patreon.com/zekses
