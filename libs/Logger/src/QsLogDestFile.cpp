// Copyright (c) 2013, Razvan Petru
// All rights reserved.

// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:

// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice, this
//   list of conditions and the following disclaimer in the documentation and/or other
//   materials provided with the distribution.
// * The name of the contributors may not be used to endorse or promote products
//   derived from this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include "QsLogDestFile.h"
#include "QsLog.h"
#include <QTextCodec>
#include <QDateTime>
#include <QtGlobal>
#include <iostream>

const int QsLogging::SizeRotationStrategy::MaxBackupCount = 10;

QsLogging::RotationStrategy::~RotationStrategy()
{
}

QsLogging::SizeRotationStrategy::SizeRotationStrategy(bool _rotateOnInit)
    : mCurrentSizeInBytes(0)
    , mMaxSizeInBytes(0)
    , mBackupsCount(0)
    , rotateOnInit(_rotateOnInit)

{
}

void QsLogging::SizeRotationStrategy::setInitialInfo(const QFile &file)
{
    mFileName = file.fileName();
    mCurrentSizeInBytes = file.size();
}

void QsLogging::SizeRotationStrategy::includeMessageInCalculation(const QString &message)
{
    mCurrentSizeInBytes += message.toUtf8().size();
}

bool QsLogging::SizeRotationStrategy::shouldRotate()
{
    return mCurrentSizeInBytes > mMaxSizeInBytes || rotateOnInit;
}

// Algorithm assumes backups will be named filename.X, where 1 <= X <= mBackupsCount.
// All X's will be shifted up.
void QsLogging::SizeRotationStrategy::rotate()
{
    if (!mBackupsCount) {
        if (!QFile::remove(mFileName))
            std::cerr << "QsLog: backup delete failed " << qPrintable(mFileName);
        return;
    }

    // 1. find the last existing backup than can be shifted up
    const QString logNamePattern = mFileName + QString::fromUtf8(".%1");
    int lastExistingBackupIndex = 0;
    for (int i = 1;i <= mBackupsCount;++i) {
        const QString backupFileName = logNamePattern.arg(i);
        if (QFile::exists(backupFileName))
            lastExistingBackupIndex = qMin(i, mBackupsCount - 1);
        else
            break;
    }

    // 2. shift up
    for (int i = lastExistingBackupIndex;i >= 1;--i) {
        const QString oldName = logNamePattern.arg(i);
        const QString newName = logNamePattern.arg(i + 1);
        QFile::remove(newName);
        const bool renamed = QFile::rename(oldName, newName);
        if (!renamed) {
            std::cerr << "QsLog: could not rename backup " << qPrintable(oldName)
                      << " to " << qPrintable(newName);
        }
    }

    // 3. rename current log file
    const QString newName = logNamePattern.arg(1);
    if (QFile::exists(newName))
        QFile::remove(newName);
    if (!QFile::rename(mFileName, newName)) {
        std::cerr << "QsLog: could not rename log " << qPrintable(mFileName)
                  << " to " << qPrintable(newName);
    }
    rotateOnInit = false;
}

QIODevice::OpenMode QsLogging::SizeRotationStrategy::recommendedOpenModeFlag()
{
    return QIODevice::Append;
}

void QsLogging::SizeRotationStrategy::setMaximumSizeInBytes(qint64 size)
{
    Q_ASSERT(size >= 0);
    mMaxSizeInBytes = size;
}

void QsLogging::SizeRotationStrategy::setBackupCount(int backups)
{
    Q_ASSERT(backups >= 0);
    mBackupsCount = qMin(backups, SizeRotationStrategy::MaxBackupCount);
}


QsLogging::FileDestination::FileDestination(const QString& filePath, RotationStrategyPtr rotationStrategy)
    : mRotationStrategy(rotationStrategy)
{
    mFile.setFileName(filePath);
    if (!mFile.open(QFile::WriteOnly | QFile::Text | mRotationStrategy->recommendedOpenModeFlag()))
        std::cerr << "QsLog: could not open log file " << qPrintable(filePath);

    mOutputStream.setDevice(&mFile);
    mOutputStream.setCodec(QTextCodec::codecForName("UTF-8"));
    mRotationStrategy->setInitialInfo(mFile);
}

void QsLogging::FileDestination::write(const QString& message, Level)
{
    mRotationStrategy->includeMessageInCalculation(message);
    if (mRotationStrategy->shouldRotate())
    {
        mOutputStream.setDevice(NULL);
        mFile.close();
        mRotationStrategy->rotate();
        if (!mFile.open(QFile::WriteOnly | QFile::Text | mRotationStrategy->recommendedOpenModeFlag()))
            std::cerr << "QsLog: could not reopen log file " << qPrintable(mFile.fileName());
        mRotationStrategy->setInitialInfo(mFile);
        mOutputStream.setDevice(&mFile);
    }

    mOutputStream << message << endl;
    mOutputStream.flush();
}

void QsLogging::FileDestination::close(const QString &message, QsLogging::Level level)
{
    Q_UNUSED(message)
    Q_UNUSED(level)
    mOutputStream.setDevice(NULL);
    mFile.close();
}

bool QsLogging::FileDestination::isValid()
{
    return mFile.isOpen();
}


void QsLogging::ErrDumpDestination::write(const QString &message, QsLogging::Level level)
{


    bool normalWrite = (level != QsLogging::ErrorLevel && level != QsLogging::FatalLevel);
    if(normalWrite)
    {

        if(level >= An<QsLogging::Logger>()->loggingLevel())
        {
            mRotationStrategy->includeMessageInCalculation(message);
            if (mRotationStrategy->shouldRotate())
            {
                mOutputStream.setDevice(NULL);
                mFile.close();
                mRotationStrategy->rotate();
                if (!mFile.open(QFile::WriteOnly | QFile::Text | mRotationStrategy->recommendedOpenModeFlag()))
                    std::cerr << "QsLog: could not reopen log file " << qPrintable(mFile.fileName());
                mRotationStrategy->setInitialInfo(mFile);
                mOutputStream.setDevice(&mFile);
            }


            mOutputStream << message << endl;
            mOutputStream.flush();
        }
        queue.push_back(message);
    }
    else
    {
        mRotationStrategy->includeMessageInCalculation(message);
        if (mRotationStrategy->shouldRotate()) {
            mOutputStream.setDevice(NULL);
            mFile.close();
            mRotationStrategy->rotate();
            if (!mFile.open(QFile::WriteOnly | QFile::Text | mRotationStrategy->recommendedOpenModeFlag()))
                std::cerr << "QsLog: could not reopen log file " << qPrintable(mFile.fileName());
            mRotationStrategy->setInitialInfo(mFile);
            mOutputStream.setDevice(&mFile);
        }
        bool queueFull = queue.size() > 0;
        if(queueFull)
            mOutputStream << "Error level triggered, dumping full cycle" << endl;
        for(auto string : queue)
        {
            mOutputStream << string << endl;
        }
        mOutputStream << message << endl;
        if(queueFull)
            mOutputStream << "Error level triggered, end of dump" << endl;
        queue.clear();
        mOutputStream.flush();
    }

}

void QsLogging::ErrDumpDestination::clearQueue()
{
    queue.clear();
}
