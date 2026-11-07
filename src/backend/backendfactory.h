#pragma once
class QObject;
class ZapretBackend;
namespace BackendFactory {
ZapretBackend *create(QObject *parent = nullptr);
}
