# Crescent-Vine

Crescent-Vine is a fork of Gecko 1.8.1 for improved compatibility on the modern web, with Windows 95 and Windows NT 3.51 in mind. Right now, Crescent-Vine's rendering capabilities are pretty similar to RetroZilla 2.2, and Firefox 2.0's, but as Crescent-Vine progresses, so will its capabilities. 

Winternight-Classic (Firefox based) is currently the primary target of Crescent-Vine.

## Building

I currently do my builds on Windows XP SP3 with Visual Studio 6.0 SP5 and MozillaBuild 1.2. Building should also work on Windows NT4, and possibly Vista and above, but don't take my word on it. Building on Windows 2000 works, but seems to be limited to Visual Studio 6.0. Building will also work with Visual Studio 2003 with Windows XP and possibly Vista+ if that is desirable (which it isn't for me)... See issue #25.

1. You're going to need to install VC6, [MozillaBuild 1.2](https://ftp.mozilla.org/pub/mozilla/libraries/win32/MozillaBuildSetup-1.2.exe), [VC6 SP5](https://github.com/rn10950/RetroZillaWeb/releases/download/0/vs6sp5.exe) (not SP6) and [VC6 Processor Pack](https://github.com/rn10950/RetroZillaWeb/releases/download/0/vcpp5.exe).

2. Place your source somewhere in a directory without spaces if it's not already. I recommend something like C:\Browser-Source\Crescent-Vine-VC6\. 

3. Start "start-l10n.bat" in C:\mozilla-build\. This will open a UNIX-type shell window. navigate to your source directory. It uses UNIX-style file paths with the Windows drive letters as the first child directory (e.g. C:\WINDOWS\System32 will be /c/WINDOWS/System32 in MSYS shell) 

4. Copy mozconfig-browser.txt to mozconfig (no extension). Open up your newly created mozconfig in a text editor. You're going to want to change the object directory, I recommend changing it to the parent directory of the source. Using my example for a source directory above, change
`mk_add_options MOZ_OBJDIR=@TOPSRCDIR@/obj-sm95-release`
to 
`mk_add_options MOZ_OBJDIR=/c/Browser-Source/Crescent-Vine-VC6/moz95/obj-fx95-release`

4. Now just run `make -f client.mk configure build` from the MSYS shell and wait. On a VM running on a modern host, building should take 20-40 minutes. On XP-era desktops expect building to take about 1 hour and 20 minutes to 2 hours. Note: When the compiler is building in the "security" directory, the build is nearly complete. 

If start-msvc6.bat can't find your VC6 installation, add the following line to start-msvc6.bat, after "SET MOZILLABUILD=..."
`SET VC6DIR=C:\Program Files\Microsoft Visual Studio\VC98`. If that doesn't work, IME, "start-l10n" works well (and possibly only) with Visual Studio 6.0.

## Incremental Builds
If you have already built Cresent-Vine and you would like to save time by building only a small subset of the program to test a change you made, run make from the corresponding folder in your object directory. Depending on what you changed, building should only take a few minutes.

EX: If you made a change to `Crescent-Vine-VC6/xpfe/browser/resources/content/navigator.xul`, cd into `{OBJDIR}/xpfe/browser/resources/content` using MSYS shell and run `make`. Note: This only partially works... If you get lots of errors in a specific directory, then it is very wise to do this to save on time. If you have build success, the compiler will sometimes skip over all the files, even if they have been changed.

## Creating a compressed installer
I don't know how to create the official installer, so instead, I go into "C:\mozilla-build\Version1.2\nsis-2.22\Bin", and run "zip2exe.exe", and create a fake, 1/2 functional compressed installer that way.

Update: It is possible to create the installer by BUILDING USING STATIC LIBRARIES (mozconfig-browser-static.txt), then change directory to the object directory, then run "make -C browser/installer installer". 
