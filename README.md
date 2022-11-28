# Building
All building and testing is currently handled with `build.sh`, which is designed to be run on a Linux system with clang. It's not very flexible, so if it doesn't work on your system, do edit it. You can pass it these options, which can be combined:

- `win` - cross-compile for Windows  
- `android` - cross-compile for Android x86, x86_64, arm, - and arm64 (requires SDK in /usr/lib/android-sdk and NDK in /opt/android-ndk)  
- `psp` - cross-compile for PSP (must have pspdev installed)  
- `wiiu` - cross-compile for Wii U (must have devkitpro, devkitppc and wut installed)  
- `debug` - compile for debugging (has its own build directory)  
- `debugger` - run with LLDB for linux build, for other platforms this is impossible  
- `clean` - don't build, remove object files in this build directory  
- `link` - link even if output seems to be up to date  
- 'buildlua' - rebuild lua even if it seems up to date  
- 'buildsoloud' - rebuild soloud even if it seems up to date  
- `test` - run after building (for windows, uses wine, for android, connects to an android device with ADB)  
- `verbose` - print every compiler command run

Example: `./build.sh test` - normally testing after making a change in the code  
`./build.sh debug debugger test` - looking into a mysterious segfault

# Dependencies

The project uses several required libraries in the `ext` folder. These are included in the project:
- a modified version of SoLoud with backends added for PSP and Switch

These are downloaded by the build script:
- [ffmpeg](https://github.com/FFmpeg/FFmpeg)
- LuaJIT
- Lua 5.4 for platforms that LuaJIT can't build for (PSP)
- [texture_atlas](https://github.com/JohnnyonFlame/texture-atlas)