Name:           qzapret
Version:        0.2.0
Release:        1%{?dist}
Summary:        Unofficial Qt6 GUI for zapret2
License:        MIT
URL:            https://github.com/avsprv/qzapret
Source0:        %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.21
BuildRequires:  ninja-build
BuildRequires:  gcc-c++
BuildRequires:  cmake(Qt6Widgets)
BuildRequires:  cmake(Qt6Test)
BuildRequires:  desktop-file-utils
Requires:       polkit

%description
Qt6 graphical front-end for zapret2: service control, strategy selection
(blockcheck2), system tray applet and autostart. Privileged operations go
through a small polkit-protected helper. zapret2 itself is not bundled and must be installed separately.

%prep
%autosetup

%build
%cmake -G Ninja
%cmake_build

%install
%cmake_install

%check
%ctest
desktop-file-validate %{buildroot}%{_datadir}/applications/qzapret.desktop

%files
%license LICENSE NOTICE.md
%doc README.md
%{_bindir}/qzapret
%{_libexecdir}/qzapret/qzapret-helper
%{_datadir}/polkit-1/actions/org.qzapret.policy
%{_datadir}/applications/qzapret.desktop

%changelog
* Tue Nov 03 2026 avsprv - 0.2.0-1
- Strategy selection via blockcheck2, tray applet, boot and login autostart
- Privileged helper with polkit policy

* Tue Nov 03 2026 avsprv - 0.1.0-1
- Initial skeleton
