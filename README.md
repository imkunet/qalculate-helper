# qalculate-helper

a lightweight wrapper around [libqaclulate](https://github.com/Qalculate/libqalculate/tree/master)

### attribution

based on [qalculate-helper](https://git.zajc.eu.org/libot/qalculate-helper.git/) by [Marko Zajc](https://zajc.eu.org/).

### added features

since forks are supposed to be useful, here are some useful features I added:

- LICENSE.md file
- docker file (published on ghcr.io)
- now with CMake (and pkg-config)
- works cleanly with clangd
- brute force removal of `command` and `plot` and `uptime` and `load`[^1]
- LHS text
- minecraft ticks & discord snowflake timestamp (likely going to be an opt-in build flag)[^2]
- true/false text on inequality evaluation

[^1]: this is done just in case and should have very minimal runtime impact
[^2]: likely going to be locked behind a build flag in the future

todo:
- decimal approximations when necessary
- built in web server to more easily integrate into apps (libuv+libh2o)?

### building

see the Dockerfile. known working build targets:

- darwin arm64
- linux arm64

this project likely DOES NOT WORK on windows. if you can fix it on windows send the fix on up.

literally the freest PR if you find other targets

### notes for your brain

if you want to keep a cache of the currency files in your docker stuff they are in:

- `~/.local/share/qalculate/btc.json`
- `~/.local/share/qalculate/eurofxref-daily.xml`
- `~/.local/share/qalculate/rates.json`

this project works seamlessly in Zed and (vs)code(+forks)!

for the vscode one make sure to install the `clangd` plugin and the `CMake Tools` plugin and all the smart intelligence should just work.
