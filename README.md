# qalculate-helper

a lightweight wrapper around [libqaclulate](https://github.com/Qalculate/libqalculate/tree/master)

### attribution

based on [qalculate-helper](https://git.zajc.eu.org/libot/qalculate-helper.git/) by [Marko Zajc](https://zajc.eu.org/).

### added features

since forks are supposed to be useful, here are some useful features I added:

- LICENSE.md file
- docker file (published on ghcr.io)
- now with cmake
- works cleanly with clangd
- better removal of `plot` and `uptime`

todo:

- includes the LHS text
- absolutely horrid "should calculation be exact" check

### building

see the Dockerfile. known working build targets:

- darwin arm64
- linux arm64

literally the freest PR if you find other targets

### notes for your brain

if you want to keep a cache of the currency files in your docker stuff they are in:

- `~/.local/share/qalculate/btc.json`
- `~/.local/share/qalculate/eurofxref-daily.xml`
- `~/.local/share/qalculate/rates.json`
