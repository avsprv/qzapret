Unofficial Qt6 GUI for [bol-van/zapret2](https://github.com/bol-van/zapret2).

> Статус: ранний скелет (v0.1.0). Умеет показывать статус `zapret2.service`
> и запускать/останавливать его через polkit. Всё остальное — в планах.

## Принципы

- GUI работает **без root**. Привилегированные действия идут через `pkexec`/polkit.
- Вся работа с апстримом изолирована в `src/backend/` (интерфейс `ZapretBackend`).
- Апстрим подключён как submodule и **не правится**; версия закреплена тегом.

## Сборка (Fedora)

```bash
sudo dnf install cmake ninja-build gcc-c++ qt6-qtbase-devel
git clone --recurse-submodules https://github.com/avsprv/qzapret.git
cd qzapret
cmake -S . -B build -G Ninja
cmake --build build
./build/qzapret
```

Если забыл `--recurse-submodules`: `git submodule update --init --recursive`.

## Требования на машине

zapret2 должен быть установлен отдельно (по документации апстрима, по умолчанию в
`/opt/zapret2`) и иметь юнит `zapret2.service`. qzapret его не устанавливает.

## Структура

```
src/                 Qt6-код GUI
src/backend/         адаптер к zapret2 (systemd)
packaging/fedora/    spec для COPR (заготовка)
third_party/zapret2/ апстрим, submodule
```

## Обновление апстрима

```bash
git -C third_party/zapret2 fetch --tags
git -C third_party/zapret2 checkout <тег>
git add third_party/zapret2 && git commit -m "Bump zapret2 to <тег>"
```

## Лицензия

MIT (см. `LICENSE`). Лицензия и авторство zapret2 — в `NOTICE.md`.
