# UzbekGram Desktop ✅

![UzbekGram Logo](.github/uzbekgram.png) ![UzbekChan](.github/uzbekchan.png)

[ English  |   [Русский](README-RU.md) ]

## Features

- shalava mod✅
- shalama mega mod✅
- mega ultra super shalava mod✅
- uzbekchan✅✅✅

## Build

### Linux (Docker)

```bash
BuildPath=~/TBuild
mkdir -p "$BuildPath"
cd "$BuildPath" || exit 1
git clone --recursive https://github.com/lutit/UzbekGramDesktop.git tdesktop
cd "$BuildPath/tdesktop" || exit 1
docker run --rm -it \
  -u "$(id -u)" \
  -v "$PWD:/usr/src/tdesktop" \
  ghcr.io/telegramdesktop/tdesktop/centos_env:latest \
  /usr/src/tdesktop/Telegram/build/docker/centos_env/build.sh \
    -D TDESKTOP_API_ID=2040 \
    -D TDESKTOP_API_HASH=b18441a1ff607e10a989891a5462e627
```

**[Windows](docs/building-win-x64.md)**
