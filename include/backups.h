#pragma once
#include "pure_sql.h"

database::puresql::DiagnosticSQLResult<database::puresql::DBVerificationResult> VerifyDatabase(QString name);
bool ProcessBackupForInvalidDbFile(QString pathToFile, QString fileName,  QStringList error);
void RemoveOlderBackups(QString fileName);;
