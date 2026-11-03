Unofficial Qt6 GUI for [bol-van/zapret2](https://github.com/bol-van/zapret2).

> Статус: ранняя версия (v0.2.0). Применение найденных стратегий в конфиг zapret2 пока не реализовано.

## Возможности

- Статус, запуск и остановка `zapret2.service`.
- **Подбор стратегий:** вкладка-обёртка над `blockcheck2.sh` (домены, IPv4/IPv6, HTTP/TLS1.2/TLS1.3/QUIC,
  уровень сканирования), найденные стратегии собираются списком, их можно скопировать.
- **Системный апплет:** иконка в трее (Plasma/Wayland через StatusNotifierItem), меню запуска/остановки;
  крестик окна сворачивает в трей, выход — через меню трея.
- **Автозапуск:** сервис zapret2 при загрузке системы (`systemctl enable/disable`) и сам qzapret при входе
  в сеанс (`~/.config/autostart/qzapret.desktop`, запуск с `--minimized`).

## Принципы

- GUI работает **без root**. Привилегированные действия выполняет `qzapret-helper`
  (`/usr/libexec/qzapret/`) через `pkexec` и polkit-действие `org.qzapret.helper`.
- Хелпер принимает только фиксированный набор команд (`unit start|stop|enable|disable`, `blockcheck`,
  `blockcheck-cancel`), строго валидирует аргументы и запускает blockcheck2 с чистым окружением.
- Вся работа с апстримом изолирована в `src/backend/` (интерфейс `ZapretBackend`).
- Апстрим подключён как submodule и **не правится**; версия закреплена тегом.

## Сборка (Fedora)

```bash
sudo dnf install cmake ninja-build gcc-c++ qt6-qtbase-devel polkit
git clone --recurse-submodules https://github.com/avsprv/qzapret.git
cd qzapret
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure   # парсер, конвейер runner, валидация хелпера
./build/qzapret
```

Для запуска из каталога сборки (без установки) хелпер находится по `../packaging/helper/qzapret-helper`;
путь можно переопределить переменной `QZAPRET_HELPER`. Для установленной версии с нормальным
polkit-диалогом: `sudo cmake --install build --prefix /usr`.

Если забыл `--recurse-submodules`: `git submodule update --init --recursive`.

## Требования на машине

zapret2 должен быть установлен отдельно (по документации апстрима, по умолчанию в
`/opt/zapret2`) и иметь юнит `zapret2.service`. qzapret его не устанавливает.

## Структура

```
src/                 Qt6-код GUI
src/backend/         адаптер к zapret2 (systemd), запуск blockcheck2
packaging/helper/    qzapret-helper — привилегированная часть (polkit)
data/                .desktop и шаблон polkit-политики
tests/               QtTest и shell-тест хелпера
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
