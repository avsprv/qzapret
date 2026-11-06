# zapret2applet

**A graphical frontend for [zapret2](https://github.com/bol-van/zapret2) on Linux.**

Manage the zapret2 service, test strategies, and update an existing installation through a graphical interface — without having to perform these tasks in a terminal.

> [!IMPORTANT]
> **Unofficial project.** `zapret2applet` is not affiliated with the author or maintainers of [zapret2](https://github.com/bol-van/zapret2).
>
> This is an early version of the application. Strategies found by the app must be transferred to the zapret2 configuration manually.

## Features

- **zapret2 management.** View the service status, start it, or stop it with one click.
- **Strategy testing.** Test specified websites over HTTP, HTTPS, and QUIC using IPv4 and IPv6. The app reports which strategies work with your ISP.
- **Copy results.** Copy a detected strategy with one click.
- **System tray.** Closing the window leaves the application running in the system tray. Start and stop controls are available from the tray icon menu.
- **Autostart.** Configure the zapret2 service to start at system boot and the app to start when you log in. When launched automatically, the app starts minimized to the tray.
- **Update checks.** The app checks GitHub for new releases once a day. You can update the installed zapret2 version to the latest release; downloaded files are verified using checksums.
- **Two interface languages.** Russian and English; the language is selected automatically based on the system language.

## Compatibility

Supported distributions:

| Distribution | Minimum version |
|---|---:|
| Fedora | 41 |
| Debian | 13 |
| Ubuntu | 25.04 |

**zapret2 must be installed before using this application.** The app manages and updates an existing installation but does not perform the initial zapret2 installation.

## Installation

### 1. Install zapret2

Install zapret2 by following the [official project instructions](https://github.com/bol-van/zapret2). The usual installation directory is `/opt/zapret2`.

### 2. Download and install the package

Open the [Releases](https://github.com/avsprv/zapret2applet/releases) page, download the package for your distribution, and install it.

**Fedora (`.rpm`):**

```bash
sudo dnf install ./zapret2applet-*.rpm
```

**Debian / Ubuntu (`.deb`):**

```bash
sudo apt install ./zapret2applet_*.deb
```

### 3. Launch the application

Find **zapret2applet** in your application menu and launch it.

> If a package for your distribution is not available yet, follow the [build from source instructions](#building-from-source).

## Usage

### Service management

On the **Status** tab, you can view the zapret2 service status, start or stop the service, and configure autostart for both the service and the application.

### Strategy testing

1. Make sure the websites you want to test are listed in the application.
2. Click **Start testing**.
3. If zapret2 is running, the app will ask you to stop it for the duration of the test.
4. Wait for the tests to finish. This may take several minutes.
5. Review the results and copy a suitable strategy.

Testing covers HTTP, HTTPS, and QUIC over both IPv4 and IPv6. A detected strategy must be transferred to the zapret2 configuration manually.

### Updates

The **Updates** tab provides **Check for updates** and **Update** actions. Automatic daily update checks can be disabled.

## Why does the application ask for an administrator password?

The main application runs as a regular user. Some operations require elevated privileges and are performed by a small helper module. The system requests the administrator password through its authorization mechanism.

Authorization may be requested when you:

- start or stop the service;
- test strategies;
- update zapret2.

An authorization prompt appears every time you update: the operation replaces files that are later executed with `root` privileges.

## Building from source

### Dependencies

**Fedora:**

```bash
sudo dnf install cmake ninja-build gcc-c++ qt6-qtbase-devel qt6-qttools-devel polkit
```

**Debian / Ubuntu:**

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-tools-dev qt6-l10n-tools pkexec
```

### Build and install

```bash
git clone https://github.com/avsprv/zapret2applet.git
cd zapret2applet

cmake -S . -B build -G Ninja
cmake --build build

sudo cmake --install build --prefix /usr
```

You can also run the application directly from the build directory without installing it:

```bash
./build/zapret2applet
```

When launched from the build directory, the authorization dialog does not include a detailed description of the operation. When the application is installed on the system, the dialog displays a clearer description.

### Building packages

Instructions for building `.deb` and `.rpm` packages are available in [`packaging/README.md`](packaging/README.md).

## Report a bug or suggest an improvement

If you find a bug or have an idea for an improvement, [open an issue](https://github.com/avsprv/zapret2applet/issues).

Developer documentation is available in [`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md).

## License and third-party projects

This project is licensed under the **MIT License**. See [`LICENSE`](LICENSE) for the license text.

`zapret2` is a separate project by [bol-van](https://github.com/bol-van/zapret2), also licensed under the MIT License. See [`NOTICE.md`](NOTICE.md) for additional information.
