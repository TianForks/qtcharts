/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Enterprise Charts Add-on.
**
** $QT_BEGIN_LICENSE$
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
** $QT_END_LICENSE$
**
****************************************************************************/

#include "abstractdomain_p.h"
#include "qabstractaxis_p.h"
#include <qmath.h>

QTCOMMERCIALCHART_BEGIN_NAMESPACE

AbstractDomain::AbstractDomain(QObject *parent)
    : QObject(parent),
      m_minX(0),
      m_maxX(0),
      m_minY(0),
      m_maxY(0),
      m_signalsBlocked(false),
      m_zoomed(false),
      m_zoomResetMinX(0),
      m_zoomResetMaxX(0),
      m_zoomResetMinY(0),
      m_zoomResetMaxY(0)

{
}

AbstractDomain::~AbstractDomain()
{
}

void AbstractDomain::setSize(const QSizeF &size)
{
	if(m_size!=size)
	{
		m_size=size;
		emit updated();
	}
}

QSizeF AbstractDomain::size() const
{
	return m_size;
}

void AbstractDomain::setRangeX(qreal min, qreal max)
{
    setRange(min, max, m_minY, m_maxY);
}

void AbstractDomain::setRangeY(qreal min, qreal max)
{
    setRange(m_minX, m_maxX, min, max);
}

void AbstractDomain::setMinX(qreal min)
{
    setRange(min, m_maxX, m_minY, m_maxY);
}

void AbstractDomain::setMaxX(qreal max)
{
    setRange(m_minX, max, m_minY, m_maxY);
}

void AbstractDomain::setMinY(qreal min)
{
    setRange(m_minX, m_maxX, min, m_maxY);
}

void AbstractDomain::setMaxY(qreal max)
{
    setRange(m_minX, m_maxX, m_minY, max);
}

qreal AbstractDomain::spanX() const
{
    Q_ASSERT(m_maxX >= m_minX);
    return m_maxX - m_minX;
}

qreal AbstractDomain::spanY() const
{
    Q_ASSERT(m_maxY >= m_minY);
    return m_maxY - m_minY;
}

bool AbstractDomain::isEmpty() const
{
    return qFuzzyCompare(spanX(), 0) || qFuzzyCompare(spanY(), 0) || m_size.isEmpty();
}

QPointF AbstractDomain::calculateDomainPoint(const QPointF &point) const
{
    const qreal deltaX = m_size.width() / (m_maxX - m_minX);
    const qreal deltaY = m_size.height() / (m_maxY - m_minY);
    qreal x = point.x() / deltaX + m_minX;
    qreal y = (point.y() - m_size.height()) / (-deltaY) + m_minY;
    return QPointF(x, y);
}

// handlers

void AbstractDomain::handleVerticalAxisRangeChanged(qreal min, qreal max)
{
    setRangeY(min, max);
}

void AbstractDomain::handleHorizontalAxisRangeChanged(qreal min, qreal max)
{
    setRangeX(min, max);
}

void AbstractDomain::blockRangeSignals(bool block)
{
    if (m_signalsBlocked!=block) {
        m_signalsBlocked=block;
        if (!block) {
            emit rangeHorizontalChanged(m_minX,m_maxX);
            emit rangeVerticalChanged(m_minY,m_maxY);
        }
    }
}

void AbstractDomain::zoomReset()
{
    if (m_zoomed) {
        setRange(m_zoomResetMinX,
                 m_zoomResetMaxX,
                 m_zoomResetMinY,
                 m_zoomResetMaxY);
        m_zoomed = false;
    }
}

void AbstractDomain::storeZoomReset()
{
    if (!m_zoomed) {
        m_zoomed = true;
        m_zoomResetMinX = m_minX;
        m_zoomResetMaxX = m_maxX;
        m_zoomResetMinY = m_minY;
        m_zoomResetMaxY = m_maxY;
    }
}

//algorithm defined by Paul S.Heckbert GraphicalGems I

void AbstractDomain::looseNiceNumbers(qreal &min, qreal &max, int &ticksCount)
{
    qreal range = niceNumber(max - min, true); //range with ceiling
    qreal step = niceNumber(range / (ticksCount - 1), false);
    min = qFloor(min / step);
    max = qCeil(max / step);
    ticksCount = int(max - min) + 1;
    min *= step;
    max *= step;
}

//nice numbers can be expressed as form of 1*10^n, 2* 10^n or 5*10^n

qreal AbstractDomain::niceNumber(qreal x, bool ceiling)
{
    qreal z = qPow(10, qFloor(log10(x))); //find corresponding number of the form of 10^n than is smaller than x
    qreal q = x / z; //q<10 && q>=1;

    if (ceiling) {
        if (q <= 1.0) q = 1;
        else if (q <= 2.0) q = 2;
        else if (q <= 5.0) q = 5;
        else q = 10;
    } else {
        if (q < 1.5) q = 1;
        else if (q < 3.0) q = 2;
        else if (q < 7.0) q = 5;
        else q = 10;
    }
    return q * z;
}

bool AbstractDomain::attachAxis(QAbstractAxis *axis)
{
    if (axis->orientation() == Qt::Vertical) {
		QObject::connect(axis->d_ptr.data(), SIGNAL(rangeChanged(qreal,qreal)), this, SLOT(handleVerticalAxisRangeChanged(qreal,qreal)));
		QObject::connect(this, SIGNAL(rangeVerticalChanged(qreal,qreal)), axis->d_ptr.data(), SLOT(handleRangeChanged(qreal,qreal)));
	}

    if (axis->orientation() == Qt::Horizontal) {
		QObject::connect(axis->d_ptr.data(), SIGNAL(rangeChanged(qreal,qreal)), this, SLOT(handleHorizontalAxisRangeChanged(qreal,qreal)));
		QObject::connect(this, SIGNAL(rangeHorizontalChanged(qreal,qreal)), axis->d_ptr.data(), SLOT(handleRangeChanged(qreal,qreal)));
	}

	return true;
}

bool AbstractDomain::detachAxis(QAbstractAxis *axis)
{
    if (axis->orientation() == Qt::Vertical) {
		QObject::disconnect(axis->d_ptr.data(), SIGNAL(rangeChanged(qreal,qreal)), this, SLOT(handleVerticalAxisRangeChanged(qreal,qreal)));
		QObject::disconnect(this, SIGNAL(rangeVerticalChanged(qreal,qreal)), axis->d_ptr.data(), SLOT(handleRangeChanged(qreal,qreal)));
	}

    if (axis->orientation() == Qt::Horizontal) {
		QObject::disconnect(axis->d_ptr.data(), SIGNAL(rangeChanged(qreal,qreal)), this, SLOT(handleHorizontalAxisRangeChanged(qreal,qreal)));
		QObject::disconnect(this, SIGNAL(rangeHorizontalChanged(qreal,qreal)), axis->d_ptr.data(), SLOT(handleRangeChanged(qreal,qreal)));
	}

	return true;
}

// operators

bool QTCOMMERCIALCHART_AUTOTEST_EXPORT operator== (const AbstractDomain &domain1, const AbstractDomain &domain2)
{
    return (qFuzzyIsNull(domain1.m_maxX - domain2.m_maxX)
            && qFuzzyIsNull(domain1.m_maxY - domain2.m_maxY)
            && qFuzzyIsNull(domain1.m_minX - domain2.m_minX)
            && qFuzzyIsNull(domain1.m_minY - domain2.m_minY));
}


bool QTCOMMERCIALCHART_AUTOTEST_EXPORT operator!= (const AbstractDomain &domain1, const AbstractDomain &domain2)
{
    return !(domain1 == domain2);
}


QDebug QTCOMMERCIALCHART_AUTOTEST_EXPORT operator<<(QDebug dbg, const AbstractDomain &domain)
{
#ifdef QT_NO_TEXTSTREAM
    Q_UNUSED(domain)
#else
    dbg.nospace() << "AbstractDomain(" << domain.m_minX << ',' << domain.m_maxX << ',' << domain.m_minY << ',' << domain.m_maxY << ')' << domain.m_size;
#endif
    return dbg.maybeSpace();
}

// This function adjusts min/max ranges to failsafe values if negative/zero values are attempted.
void AbstractDomain::adjustLogDomainRanges(qreal &min, qreal &max)
{
    if (min <= 0) {
       min = 1.0;
       if (max <= min)
           max = min + 1.0;
    }
}


#include "moc_abstractdomain_p.cpp"

QTCOMMERCIALCHART_END_NAMESPACE
