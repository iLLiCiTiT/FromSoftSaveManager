#pragma once

#include <QString>
#include <QWidget>

class BackupsDb;

bool hasLegacyBackups(const QString& backupRoot);
bool migrateIfAgreed(const QString& backupRoot, BackupsDb* db, QWidget* parent = nullptr);
