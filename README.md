# InfraRedChat
A "chat" application to send messages between Wii U <-> 3DS over InfraRed.  

## Disclaimer
I made this within a few hours only to test IR connections on the Wii U, after I spent some time reverse engineering it.  
The message transmission by this app is by no means a good practice (messages might not get transmitted for unstable connections, the app might get stuck in false states, ...).

## Building
If you do want to test it, you will need to build this with a [libctru](https://github.com/devkitPro/libctru) build including [PR #513](https://github.com/devkitPro/libctru/pull/513) and a [wut](https://github.com/devkitPro/wut) build including [PR #288](https://github.com/devkitPro/wut/pull/288).
