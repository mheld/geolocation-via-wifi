# workflow of the system (how does it work) 
we invoke the 'iwlist scan' command on the desired machine to get mac address and signal pairs
then we pipe the macaddress-signalstrength pairs over to our form*generator
then we POST that over to skyhook (or any other provider) to get the coordinates of where we are
then we save results as a file in TMP so that other apps can touch it


reference documentation about skyhook API
skyhook endpoint takes in an XML document with a bunch of embedded documents containing access-point mac addresses and signal strengths
<?xml version='1.0'?><LocationRQ xmlns='http://skyhookwireless.com/wps/2005' version='2.6' street-address-lookup='full'><authentication version='2.0'><simple><username>mheld</username><realm>marc@nu</realm></simple></authentication><access-point><mac>000B86982680</mac><signal-strength>-50</signal-strength></access-point><access-point><mac>000B86982681</mac><signal-strength>-50</signal-strength></access-point></LocationRQ>

<signal-strength>STRENGTH (does it make a difference if this is a static value or not? -- we can test this)</signal-strength>

spits back a document like this:
<?xml version="1.0" encoding="UTF-8" standalone="yes"?><LocationRS version="2.6" xmlns="http://skyhookwireless.com/wps/2005"><location nap="3"><latitude>42.33998341634523</latitude><longitude>-71.09059180229337</longitude><hpe>93</hpe><street-address distanceToPoint="26.68249548639868"><street-number>372</street-number><address-line>Huntington Ave</address-line><city>Boston</city><postal-code>02115</postal-code><county>Suffolk</county><state code="MA">Massachusetts</state><country code="US">United States</country></street-address></location></LocationRS>

## switching providers in the location program
will refactor to be switchable

## where to register for the api
http://www.skyhookwireless.com/developers/sdk.php

## is there a quota on the number of times I can request?
asked -- also, is there a way to get location of just one access point?

## how to compile for openwrt
requires libcurl (makefile contains dependency)

## cronjob to ensure data collection?
To have the location update every 24 hours at 1am as well as after boot, add this to crontab: 
@reboot /path/to/location
0 1 * * * /path/to/location

## is there a way to get HTTP requests without libcurl (and all of its dependencies)
not sure -- will do research

# extra notes
1) might make sense to just dump the XML file of the location to /tmp/location instead of parsing the xml file -- there's probably going to be some case that brakes the parser (it's really really naive right now -- will fix it very soon)
2) I added an include to regex.h -- however it is a POSIX library, so it *should* be there; I don't have a router to test on at the moment but I will do so as soon as I can
3) you will have so uncomment and change the popen command (lines 149-150) to point to the right iwlist command
4) I will add libcurl to the package to be statically loaded
5) how do you guys deal with version control? 