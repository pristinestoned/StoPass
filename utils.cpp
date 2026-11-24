#include "utils.h"
#include <QRandomGenerator>

QString generatePassword(int length) {
    const QString chars =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "!@#$%^&*()-_=+[]{}";

    QString out;
    for (int i = 0; i < length; i++) {
        int idx = QRandomGenerator::global()->bounded(chars.size());
        out += chars[idx];
    }
    return out;
}
