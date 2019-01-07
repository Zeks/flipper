#include "qml/imageprovider.h"

#include <QQuickImageProvider>
#include <QPainter>
#include "third_party/qr/QrCode.hpp"

    QPixmap QRImageProvider::requestPixmap ( const QString &id, QSize *size, const QSize &requestedSize )
    {
        QSize localSize;
        localSize.setWidth(300);
        localSize.setHeight(300);
        QPixmap pix(300,300);
        pix.fill("white");
        QScopedPointer<QPainter> paint = QScopedPointer<QPainter> (new QPainter());
        paint->begin(&pix);
        paintQR(*paint.data(), localSize, url, QColor("black"));
        return pix;
    }

    void QRImageProvider::paintQR(QPainter &painter, const QSize sz, const QString &data, QColor fg)
    {
        // NOTE: At this point you will use the API to get the encoding and format you want, instead of my hardcoded stuff:
        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(data.toUtf8().constData(), qrcodegen::QrCode::Ecc::LOW);
        const int s=qr.getSize()>0?qr.getSize():1;
        const double w=sz.width();
        const double h=sz.height();
        const double aspect=w/h;
        const double size=((aspect>1.0)?h:w);
        const double scale=size/(s+2);
        // NOTE: For performance reasons my implementation only draws the foreground parts in supplied color.
        // It expects background to be prepared already (in white or whatever is preferred).
        painter.setPen(Qt::NoPen);
        painter.setBrush(fg);
        for(int y=0; y<s; y++) {
            for(int x=0; x<s; x++) {
                const int color=qr.getModule(x, y);  // 0 for white, 1 for black
                if(0!=color) {
                    const double rx1=(x+1)*scale, ry1=(y+1)*scale;
                    QRectF r(rx1, ry1, scale, scale);
                    painter.drawRects(&r,1);
                }
            }
        }
    }
  
