#pragma once

#include <QQuickImageProvider>
#include <QString>
#include <QPainter>
#include <QSize>


class QRImageProvider : public QQuickImageProvider {
public:
    QRImageProvider(): QQuickImageProvider(QQuickImageProvider::Pixmap){}
    // Overriden method of base class; should return cropped image
    virtual QPixmap requestPixmap ( const QString &id, QSize *size, const QSize &requestedSize );
    void paintQR(QPainter &painter, const QSize sz, const QString &data, QColor fg);
    QString url = "initial";
    QPainter paint;
};

