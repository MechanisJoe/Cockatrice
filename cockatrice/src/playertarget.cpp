#include "playertarget.h"
#include "player.h"
#include "protocol_datastructures.h"
#include "pixmapgenerator.h"
#include <QPainter>
#include <QPixmapCache>
#include <QDebug>
#include <math.h>

PlayerCounter::PlayerCounter(Player *_player, int _id, const QString &_name, int _value, QGraphicsItem *parent)
	: AbstractCounter(_player, _id, _name, false, _value, parent)
{
}

QRectF PlayerCounter::boundingRect() const
{
	return QRectF(0, 0, 50, 30);
}

void PlayerCounter::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
	const int radius = 8;
	const qreal border = 1;
	QPainterPath path(QPointF(50 - border / 2, border / 2));
	path.lineTo(radius, border / 2);
	path.arcTo(border / 2, border / 2, 2 * radius, 2 * radius, 90, 90);
	path.lineTo(border / 2, 30 - border / 2);
	path.lineTo(50 - border / 2, 30 - border / 2);
	path.closeSubpath();
	
	QPen pen(QColor(100, 100, 100));
	pen.setWidth(border);
	painter->setPen(pen);
	painter->setBrush(hovered ? QColor(50, 50, 50, 160) : QColor(0, 0, 0, 160));
	
	painter->drawPath(path);

	QRectF translatedRect = painter->combinedTransform().mapRect(boundingRect());
	QSize translatedSize = translatedRect.size().toSize();
	painter->resetTransform();
	QFont font("Serif");
	font.setWeight(QFont::Bold);
	font.setPixelSize(qMax((int) round(translatedSize.height() / 1.3), 9));
	painter->setFont(font);
	painter->setPen(Qt::white);
	painter->drawText(translatedRect, Qt::AlignCenter, QString::number(value));
}

PlayerTarget::PlayerTarget(Player *_owner)
	: ArrowTarget(_owner, _owner), playerCounter(0)
{
	setCacheMode(DeviceCoordinateCache);

	if (!fullPixmap.loadFromData(_owner->getUserInfo()->getAvatarBmp()))
		fullPixmap = QPixmap();
}

QRectF PlayerTarget::boundingRect() const
{
	return QRectF(0, 0, 160, 64);
}

void PlayerTarget::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
	ServerInfo_User *info = owner->getUserInfo();

	const qreal border = 2;

	QRectF avatarBoundingRect = boundingRect().adjusted(border, border, -border, -border);
	QRectF translatedRect = painter->combinedTransform().mapRect(avatarBoundingRect);
	QSize translatedSize = translatedRect.size().toSize();
	QPixmap cachedPixmap;
	const QString cacheKey = "avatar" + QString::number(translatedSize.width()) + "_" + QString::number(info->getUserLevel()) + "_" + QString::number(fullPixmap.cacheKey());
#if QT_VERSION >= 0x040600
	if (!QPixmapCache::find(cacheKey, &cachedPixmap)) {
#else
	if (!QPixmapCache::find(cacheKey, cachedPixmap)) {
#endif
		cachedPixmap = QPixmap(translatedSize.width(), translatedSize.height());
		
		QPainter tempPainter(&cachedPixmap);
		QRadialGradient grad(translatedRect.center(), sqrt(translatedSize.width() * translatedSize.width() + translatedSize.height() * translatedSize.height()) / 2);
		grad.setColorAt(1, Qt::black);
		grad.setColorAt(0, QColor(180, 180, 180));
		tempPainter.fillRect(QRectF(0, 0, translatedSize.width(), translatedSize.height()), grad);
		
		QPixmap tempPixmap;
		if (fullPixmap.isNull())
			tempPixmap = UserLevelPixmapGenerator::generatePixmap(translatedSize.height(), info->getUserLevel());
		else
			tempPixmap = fullPixmap.scaled(translatedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		
		tempPainter.drawPixmap((translatedSize.width() - tempPixmap.width()) / 2, (translatedSize.height() - tempPixmap.height()) / 2, tempPixmap);
		QPixmapCache::insert(cacheKey, cachedPixmap);
	}
	
	painter->save();
	painter->resetTransform();
	painter->translate((translatedSize.width() - cachedPixmap.width()) / 2.0, 0);
	painter->drawPixmap(translatedRect, cachedPixmap, cachedPixmap.rect());
	painter->restore();

	QRectF nameRect = QRectF(0, boundingRect().height() - 20, 110, 20);
	painter->fillRect(nameRect, QColor(0, 0, 0, 160));
	QRectF translatedNameRect = painter->combinedTransform().mapRect(nameRect);
	
	painter->save();
	painter->resetTransform();
	
	QString name = info->getName();
	if (name.size() > 13)
		name = name.mid(0, 10) + "...";
	
	QFont font;
	font.setPixelSize(qMax((int) round(translatedNameRect.height() / 1.5), 9));
	painter->setFont(font);
	painter->setPen(Qt::white);
	painter->drawText(translatedNameRect, Qt::AlignVCenter | Qt::AlignLeft, "  " + name);
	painter->restore();

	QPen pen(QColor(100, 100, 100));
	pen.setWidth(border);
	pen.setJoinStyle(Qt::RoundJoin);
	painter->setPen(pen);
	painter->drawRect(boundingRect().adjusted(border / 2, border / 2, -border / 2, -border / 2));
	
	if (getBeingPointedAt())
		painter->fillRect(boundingRect(), QBrush(QColor(255, 0, 0, 100)));
}

AbstractCounter *PlayerTarget::addCounter(int _counterId, const QString &_name, int _value)
{
	if (playerCounter)
		return 0;
	
	playerCounter = new PlayerCounter(owner, _counterId, _name, _value, this);
	playerCounter->setPos(boundingRect().width() - playerCounter->boundingRect().width(), boundingRect().height() - playerCounter->boundingRect().height());
	connect(playerCounter, SIGNAL(destroyed()), this, SLOT(delCounter()));
	
	return playerCounter;
}

void PlayerTarget::delCounter()
{
	playerCounter = 0;
}