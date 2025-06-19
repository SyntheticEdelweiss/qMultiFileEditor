#include "MulticolorDelegate.h"

#include <QtGui/QPainter>





#define HORIZONTAL_MARGIN style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, nullptr) + 1

const int minimumLineNumberDigits = 6;
const int minRowHeight = 18;

MulticolorDelegate::MulticolorDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

void MulticolorDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!index.data(Qt::UserRole).isValid() || !index.data(Qt::UserRole).canConvert<ColoredText>())
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();

    const ColoredText coloredText = index.data(Qt::UserRole).value<ColoredText>();
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    QStyle* style = opt.widget->style();

    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    const int horizontalMargin = HORIZONTAL_MARGIN;

    // Avoid overlapping with checkbox
    if (opt.features & QStyleOptionViewItem::HasCheckIndicator)
        opt.rect.setLeft(opt.rect.left() + style->pixelMetric(QStyle::PM_IndicatorWidth) + 2 * horizontalMargin);
    // Avoid overlapping with icon
    if (opt.features & QStyleOptionViewItem::HasDecoration)
        opt.rect.setLeft(opt.rect.left() + opt.decorationSize.width() + 2 * horizontalMargin);

    const QRect focusRect = opt.rect;
    const int lineNumberWidth = drawLineNumber(painter, opt, opt.rect, index, coloredText);
    const QRect textRect = opt.rect.adjusted(lineNumberWidth, 0, 0, 0);
    drawText(painter, opt, textRect, index, coloredText);
    drawFocus(painter, option, focusRect);
    painter->restore();
}

// TODO: there's probably a better way to calculate required size other than manually adding presumed horizontalMargin
QSize MulticolorDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!index.data(Qt::UserRole).isValid() || !index.data(Qt::UserRole).canConvert<ColoredText>())
    {
        QSize result = QStyledItemDelegate::sizeHint(option, index);
        result.setHeight(std::max(result.height(), minRowHeight));
        return result;
    }

    QVariant value = index.data(Qt::SizeHintRole);
    if (value.isValid())
        return qvariant_cast<QSize>(value);

    const ColoredText coloredText = index.data(Qt::UserRole).value<ColoredText>();
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    QStyle* style = opt.widget->style();
    const int horizontalMargin = HORIZONTAL_MARGIN;
    QSize result(0, std::max(minRowHeight, opt.fontMetrics.height()));
    if (opt.features & QStyleOptionViewItem::HasCheckIndicator)
    {
        result.setWidth(result.width() + style->pixelMetric(QStyle::PM_IndicatorWidth) + 2 * horizontalMargin);
        result.setHeight(std::max(style->pixelMetric(QStyle::PM_IndicatorHeight), result.height()));
    }
    if (opt.features & QStyleOptionViewItem::HasDecoration)
    {
        result.setWidth(result.width() + opt.decorationSize.width() + 2 * horizontalMargin);
        result.setHeight(std::max(opt.decorationSize.height(), result.height()));
    }
    const int numberWidth = getLineNumberInfo(option, index, coloredText).first;
    const int textWidth = opt.fontMetrics.horizontalAdvance(coloredText.text);
    result.setWidth(result.width() + numberWidth + textWidth + 2 * horizontalMargin);
    return result;
}

std::pair<uint, QString> MulticolorDelegate::getLineNumberInfo(const QStyleOptionViewItem& option, const QModelIndex& index, const ColoredText& coloredText) const
{
    Q_UNUSED(index)
    if (coloredText.lineNumber < 1)
        return { 0, {} };
    const QString lineNumberText = QString::number(coloredText.lineNumber);
    const int lineNumberDigits = qMax(minimumLineNumberDigits, lineNumberText.size());
    const int fontWidth = option.fontMetrics.horizontalAdvance(QString(lineNumberDigits, QLatin1Char('0')));
    const QStyle* style = option.widget->style();
    const int horizontalMargin = HORIZONTAL_MARGIN;
    return { horizontalMargin + fontWidth + horizontalMargin, lineNumberText };
}

// returns the width of the line number area
int MulticolorDelegate::drawLineNumber(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, const QModelIndex& index, const ColoredText& coloredText) const
{
    const bool isSelected = option.state & QStyle::State_Selected;
    const std::pair<int, QString> numberInfo = getLineNumberInfo(option, index, coloredText);
    if (numberInfo.first == 0)
        return 0;
    QRect lineNumberAreaRect(rect);
    lineNumberAreaRect.setWidth(numberInfo.first);

    QPalette::ColorGroup cg = QPalette::Normal;
    if (!(option.state & QStyle::State_Active))
        cg = QPalette::Inactive;
    else if (!(option.state & QStyle::State_Enabled))
        cg = QPalette::Disabled;

    painter->fillRect(lineNumberAreaRect, QBrush(isSelected ?
                                                 option.palette.brush(cg, QPalette::Highlight) :
                                                 option.palette.color(cg, QPalette::Base).darker(111)));

    QStyleOptionViewItem opt = option;
    opt.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
    opt.palette.setColor(QPalette::Text, Qt::darkGray);

    const QWidget* w = option.widget;
    const QStyle* style = w->style();
    const int horizontalMargin = HORIZONTAL_MARGIN;

    const QRect textRect = lineNumberAreaRect.adjusted(horizontalMargin, 0, -horizontalMargin, 0);
    style->drawItemText(painter, textRect, opt.displayAlignment, opt.palette, w->isEnabled(), numberInfo.second, (option.state & QStyle::State_Selected) ? QPalette::HighlightedText : QPalette::Text);

    return numberInfo.first;
}

void MulticolorDelegate::drawText(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, const QModelIndex& index, const ColoredText& coloredText) const
{
    Q_UNUSED(index)
    QFontMetrics fontMetrics = painter->fontMetrics();
    const QStyle* style = option.widget->style();
    const int horizontalMargin = HORIZONTAL_MARGIN;

    QRect textRect = rect;
    textRect.adjust(horizontalMargin, 0, -horizontalMargin, 0);
    QString resultText = fontMetrics.elidedText(coloredText.text, Qt::ElideRight, textRect.width());
    QRect curRect;
    const QColor defaultTextColor = option.palette.color((option.state & QStyle::State_Selected) ? QPalette::HighlightedText : QPalette::Text);
    for (const ColoredSegment& segment : coloredText.segments)
    {
        if (segment.indexStart >= resultText.length())
            break;

        QString curText = resultText.mid(segment.indexStart, segment.length);
        int curTextWidth = fontMetrics.horizontalAdvance(curText);
        curRect = QRect(textRect.topLeft(), QSize(curTextWidth, textRect.height()));
        painter->setPen(segment.textColor.isValid() ? segment.textColor : defaultTextColor);
        if (segment.backgroundColor.isValid())
            painter->fillRect(curRect, segment.backgroundColor);
        style->drawItemText(painter, curRect, option.displayAlignment, option.palette, option.widget->isEnabled(), curText);
        textRect.setLeft(textRect.left() + curTextWidth);
    }
}

void MulticolorDelegate::drawFocus(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect) const
{
    if ((option.state & QStyle::State_HasFocus) == 0 || !rect.isValid())
        return;
    QStyleOptionFocusRect o;
    o.QStyleOption::operator=(option);
    o.rect = rect;
    o.state |= QStyle::State_KeyboardFocusChange;
    o.state |= QStyle::State_Item;
    QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled)
                            ? QPalette::Normal : QPalette::Disabled;
    o.backgroundColor = option.palette.color(cg, (option.state & QStyle::State_Selected)
                                                 ? QPalette::Highlight : QPalette::Window);
    const QWidget* widget = option.widget;
    QStyle* style = widget->style();
    style->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter, widget);
}




MulticolorDelegateV2::MulticolorDelegateV2(QObject* parent)
    : QItemDelegate(parent)
{}

QStyleOptionViewItem MulticolorDelegateV2::setOptions(const QModelIndex& index, const QStyleOptionViewItem& option) const
{
    QStyleOptionViewItem opt = QItemDelegate::setOptions(index, option);

    QVariant value;
    opt.index = index;
    value = index.data(Qt::CheckStateRole);
    if (value.isValid() && !value.isNull()) {
        opt.features |= QStyleOptionViewItem::HasCheckIndicator;
        opt.checkState = static_cast<Qt::CheckState>(value.toInt());
    }
    value = index.data(Qt::DecorationRole);
    if (value.isValid() && !value.isNull()) {
        opt.features |= QStyleOptionViewItem::HasDecoration;
        switch (value.userType()) {
        case QMetaType::QIcon: {
            opt.icon = qvariant_cast<QIcon>(value);
            QIcon::Mode mode;
            if (!(opt.state & QStyle::State_Enabled))
                mode = QIcon::Disabled;
            else if (opt.state & QStyle::State_Selected)
                mode = QIcon::Selected;
            else
                mode = QIcon::Normal;
            QIcon::State state = opt.state & QStyle::State_Open ? QIcon::On : QIcon::Off;
            QSize actualSize = opt.icon.actualSize(opt.decorationSize, mode, state);
            // For highdpi icons actualSize might be larger than decorationSize, which we don't want. Clamp it to decorationSize.
            opt.decorationSize = QSize(qMin(opt.decorationSize.width(), actualSize.width()),
                                       qMin(opt.decorationSize.height(), actualSize.height()));
            break;
        }
        case QMetaType::QColor: {
            QPixmap pixmap(opt.decorationSize);
            pixmap.fill(qvariant_cast<QColor>(value));
            opt.icon = QIcon(pixmap);
            break;
        }
        case QMetaType::QImage: {
            QImage image = qvariant_cast<QImage>(value);
            opt.icon = QIcon(QPixmap::fromImage(image));
            opt.decorationSize = image.size() / image.devicePixelRatio();
            break;
        }
        case QMetaType::QPixmap: {
            QPixmap pixmap = qvariant_cast<QPixmap>(value);
            opt.icon = QIcon(pixmap);
            opt.decorationSize = pixmap.size() / pixmap.devicePixelRatio();
            break;
        }
        default:
            break;
        }
    }
    value = index.data(Qt::DisplayRole);
    if (value.isValid() && !value.isNull()) {
        opt.features |= QStyleOptionViewItem::HasDisplay;
        //opt.text = displayText(value, opt.locale);
        opt.text = value.toString();
    }
    opt.backgroundBrush = qvariant_cast<QBrush>(index.data(Qt::BackgroundRole));

    return opt;
}

std::pair<uint, QString> MulticolorDelegateV2::getLineNumberInfo(const QStyleOptionViewItem& option, const QModelIndex& index, const ColoredText& coloredText) const
{
    Q_UNUSED(index)
    if (coloredText.lineNumber < 1)
        return { 0, {} };
    const QString lineNumberText = QString::number(coloredText.lineNumber);
    const int lineNumberDigits = qMax(minimumLineNumberDigits, lineNumberText.size());
    const int fontWidth = option.fontMetrics.horizontalAdvance(QString(lineNumberDigits, QLatin1Char('0')));
    const QStyle* style = option.widget->style();
    const int horizontalMargin = HORIZONTAL_MARGIN;
    return { horizontalMargin + fontWidth + horizontalMargin, lineNumberText };
}

LayoutInfo MulticolorDelegateV2::getLayoutInfo(const QStyleOptionViewItem& option, const QModelIndex& index, const ColoredText& coloredText) const
{
    LayoutInfo info;
    info.option = setOptions(index, option);

    // check mark
    const bool checkable = (index.model()->flags(index) & Qt::ItemIsUserCheckable);
    info.checkState = Qt::Unchecked;
    if (checkable)
    {
        QVariant checkStateData = index.data(Qt::CheckStateRole);
        info.checkState = static_cast<Qt::CheckState>(checkStateData.toInt());
        info.checkRect = doCheck(info.option, info.option.rect, checkStateData);
    }

    // icon
    info.icon = index.data(Qt::DecorationRole).value<QIcon>();
    if (!info.icon.isNull())
    {
        const QSize size = info.option.decorationSize;
        info.pixmapRect = QRect(0, 0, size.width(), size.height());
    }

    // text
    info.textRect = info.option.rect.adjusted(0, 0, info.checkRect.width() + info.pixmapRect.width(), 0);

    // do basic layout
    doLayout(info.option, &info.checkRect, &info.pixmapRect, &info.textRect, false);

    // adapt for line numbers
    const int lineNumberWidth = getLineNumberInfo(info.option, index, coloredText).first;
    info.lineNumberRect = info.textRect;
    info.lineNumberRect.setWidth(lineNumberWidth);
    info.textRect.adjust(lineNumberWidth, 0, 0, 0);
    return info;
}

void MulticolorDelegateV2::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!index.data(Qt::UserRole).isValid() || !index.data(Qt::UserRole).canConvert<ColoredText>())
    {
        QItemDelegate::paint(painter, option, index);
        return;
    }

    const ColoredText coloredText = index.data(Qt::UserRole).value<ColoredText>();
    const LayoutInfo info = getLayoutInfo(option, index, coloredText);
    const auto& opt = info.option;

    painter->save();
    painter->setFont(opt.font);
    if (opt.state & QStyle::State_Selected) {
        painter->setPen(opt.palette.color(QPalette::HighlightedText));
        painter->setBrush(opt.palette.highlight());
    }

    QItemDelegate::drawBackground(painter, opt, index);
    QItemDelegate::drawCheck(painter, opt, info.checkRect, info.checkState); // internally checks whether it's valid
    QItemDelegate::drawDecoration(painter, opt, info.pixmapRect, QItemDelegate::decoration(opt, index.data(Qt::DecorationRole)));
    drawLineNumber(painter, opt, info.lineNumberRect, index, coloredText);
    drawText(painter, opt, info.textRect, index, coloredText);
    QItemDelegate::drawFocus(painter, opt, (info.lineNumberRect | info.textRect));

    painter->restore();
}

// TODO: fuck Qt with its undocumented functions and all this geometry bullshit. This currently does produce proper result but does so in shitty-hacky way
QSize MulticolorDelegateV2::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!index.data(Qt::UserRole).isValid() || !index.data(Qt::UserRole).canConvert<ColoredText>())
    {
        QSize result = QItemDelegate::sizeHint(option, index);
        result.setHeight(std::max(result.height(), minRowHeight));
        return result;
    }

    QVariant value = index.data(Qt::SizeHintRole);
    if (value.isValid())
        return qvariant_cast<QSize>(value);

    const ColoredText coloredText = index.data(Qt::UserRole).value<ColoredText>();
    const LayoutInfo info = getLayoutInfo(option, index, coloredText);
    const auto& opt = info.option;
    const int height = qMax(minRowHeight, index.data(Qt::SizeHintRole).value<QSize>().height());
    const QRect textMaxRect(0, 0, INT_MAX / 256, height);
    const QRect textLayoutRect = textRectangle(nullptr, textMaxRect, opt.font, coloredText.text);
    const QRect textRect(info.textRect.x(), info.textRect.y(), textLayoutRect.width(), height);
    const QRect layoutRect = info.checkRect | info.pixmapRect | info.lineNumberRect | textRect;

    // layoutRect gives the wrong width because somehow frameHMargin is unaccounted for if there's checkbox or icon
    // it also gives wrong height because fuck knows how QItemDelegate::doLayout() calculates geometry
    const QWidget* widget = opt.widget;
    QStyle* style = widget->style();
    const bool hasMargin = (opt.features & (QStyleOptionViewItem::HasCheckIndicator | QStyleOptionViewItem::HasDecoration));
    const int frameHMargin = hasMargin ? HORIZONTAL_MARGIN : 0;
    QSize resultSize = layoutRect.size();
    resultSize.setWidth(resultSize.width() + frameHMargin);

    return resultSize;
}

// returns the width of the line number area
int MulticolorDelegateV2::drawLineNumber(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, const QModelIndex& index, const ColoredText& coloredText) const
{
    const bool isSelected = option.state & QStyle::State_Selected;
    const std::pair<int, QString> numberInfo = getLineNumberInfo(option, index, coloredText);
    if (numberInfo.first == 0)
        return 0;
    QRect lineNumberAreaRect(rect);
    lineNumberAreaRect.setWidth(numberInfo.first);

    QPalette::ColorGroup cg = QPalette::Normal;
    if (!(option.state & QStyle::State_Active))
        cg = QPalette::Inactive;
    else if (!(option.state & QStyle::State_Enabled))
        cg = QPalette::Disabled;

    painter->fillRect(lineNumberAreaRect, QBrush(isSelected ?
                                                 option.palette.brush(cg, QPalette::Highlight) :
                                                 option.palette.color(cg, QPalette::Base).darker(111)));

    QStyleOptionViewItem opt = option;
    opt.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
    opt.palette.setColor(cg, QPalette::Text, Qt::darkGray);

    QItemDelegate::drawDisplay(painter, opt, lineNumberAreaRect, numberInfo.second);

    return numberInfo.first;
}

void MulticolorDelegateV2::drawText(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, const QModelIndex& index, const ColoredText& coloredText) const
{
    Q_UNUSED(index)
    const QPen savedPen = painter->pen();
    const QStyle* style = option.widget->style();
    QFontMetrics fontMetrics = painter->fontMetrics();
    const int textMargin = HORIZONTAL_MARGIN;
    QRect textRect = rect;
    textRect.adjust(textMargin, 0, -textMargin, 0);
    QString resultText = fontMetrics.elidedText(coloredText.text, Qt::ElideRight, textRect.width());
    const QColor defaultTextColor = option.palette.color((option.state & QStyle::State_Selected) ? QPalette::HighlightedText : QPalette::Text);
    QRect curRect;
    for (const ColoredSegment& segment : coloredText.segments)
    {
        if (segment.indexStart >= resultText.length())
            break;

        QString curText = resultText.mid(segment.indexStart, segment.length);
        int curTextWidth = fontMetrics.horizontalAdvance(curText);
        curRect = QRect(textRect.topLeft(), QSize(curTextWidth, textRect.height()));
        painter->setPen(segment.textColor.isValid() ? segment.textColor : defaultTextColor);
        if (segment.backgroundColor.isValid())
            painter->fillRect(curRect, segment.backgroundColor);
        style->drawItemText(painter, curRect, option.displayAlignment, option.palette, option.widget->isEnabled(), curText);
        textRect.setLeft(textRect.left() + curTextWidth);
    }
    painter->setPen(savedPen);
}
