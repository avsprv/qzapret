# QML and Kirigami migration

## Goals

- Replace the Qt Widgets presentation layer with Qt Quick and KDE Kirigami 6.
- Preserve the existing C++ service, helper, configuration, profile, blockcheck, and update logic.
- Keep privileged operations behind the existing Polkit-protected helper.
- Follow KDE HIG and use the active KDE/Qt platform theme rather than hard-coded widget palettes.
- Keep the current application buildable while the replacement UI is developed incrementally.

## Migration strategy

The migration starts with a separately built, non-installed QML preview target. The existing
Qt Widgets executable remains the production target until feature parity and regression tests
are complete. This avoids shipping an incomplete replacement or temporarily removing working
features from release packages.

The QML preview currently demonstrates the Kirigami application shell and the first C++ bridge
for service status and start/stop operations. Navigation entries for the remaining sections are
explicit placeholders, not implemented functionality.

## Planned stages

1. Establish the Qt Quick/Kirigami build and a minimal C++/QML application shell.
2. Extract UI-independent operations from QWidget page classes into QObject controllers and
   QAbstractItemModel models.
3. Migrate configuration and profile management.
4. Migrate lists and journal monitoring.
5. Migrate blockcheck2 orchestration and strategy results.
6. Migrate update/install workflows and application settings.
7. Reimplement tray integration and desktop lifecycle for the QML application.
8. Remove the legacy Widgets UI only after feature parity, tests, and package validation.

## Architectural boundaries

- QML owns presentation, navigation, layouts, and user interaction.
- C++ QObject controllers expose typed properties, invokable commands, and signals.
- Existing backend classes remain responsible for systemd, helper invocation, blockcheck2,
  update checking, and privileged operations.
- QML must never execute shell commands or invoke pkexec directly.
- Existing settings keys and user data formats must remain backward-compatible.

## Build notes

The preview target is intentionally not installed or packaged as the application executable.
It is compiled in CI to validate the QML module and C++ bridge. Production packaging continues
to ship the proven Widgets executable until the migration is complete.

## Quality baseline

The migration follows the repository-wide [quality and engineering standards](QUALITY_STANDARDS.md),
which use the official KDE Kirigami documentation and KDE Human Interface Guidelines as their
implementation baseline.

In particular, each migrated page must have:
- a C++ controller/model with a typed, testable API;
- native Kirigami interaction patterns and platform theme integration;
- complete Russian and English localization;
- accessible keyboard and focus behavior;
- loading, empty, success, cancellation, and failure states where applicable;
- isolated regression tests for the controller and critical workflows;
- documentation and CI coverage.

The QML preview is not considered production-ready while navigation placeholders remain, settings
do not affect runtime behavior, or the About/localization/desktop integration work is incomplete.
The final replacement gate is defined in QUALITY_STANDARDS.md.
