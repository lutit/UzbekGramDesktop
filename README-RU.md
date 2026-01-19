# UzbekGram Desktop ✅
![UzbekGram Logo](.github/uzbekgram.png) ![UzbekChan](.github/uzbekchan.png)

[ [English](README.md)  | Русский ]

## Функции и Фишки

- shalava mod✅
- shalama mega mod✅
- mega ultra super shalava mod✅
- uzbekchan✅✅✅

## Сборка

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

**[macOS](docs/building-mac.md)**

## Использованные материалы

### Телеграм клиенты

- [Telegram Desktop](https://github.com/telegramdesktop/tdesktop)
- [AyuGram Desktop](https://github.com/AyuGram/AyuGramDesktop)
- [Kotatogram](https://github.com/kotatogram/kotatogram-desktop)
- [64Gram](https://github.com/TDesktop-x64/tdesktop)
- [Forkgram](https://github.com/forkgram/tdesktop)

### Использованные библиотеки

- [JSON for Modern C++](https://github.com/nlohmann/json)
- [SQLite](https://github.com/sqlite/sqlite)
- [sqlite_orm](https://github.com/fnc12/sqlite_orm)

### Иконки

- [Solar Icon Set](https://www.figma.com/community/file/1166831539721848736)

### Боты

- [TelegramDB](https://t.me/tgdatabase) для получения юзернейма по ID
