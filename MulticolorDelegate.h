#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtWidgets/QItemDelegate>
#include <QtWidgets/QStyledItemDelegate>

struct ColoredSegment
{
    ColoredSegment() = default;
    ColoredSegment(int a_start, int a_end, QColor a_backgroundColor = QColor(), QColor a_textColor = QColor())
        : indexStart(a_start)
        , indexEnd(a_end)
        , length(indexEnd - indexStart)
        , textColor(a_textColor)
        , backgroundColor(a_backgroundColor)
    {}

    int indexStart = 0;
    int indexEnd = 0;
    int length = 0;
    QColor textColor;
    QColor backgroundColor;
};
Q_DECLARE_METATYPE(ColoredSegment)

class ColoredText
{
public:
    QString text;
    QList<ColoredSegment> segments;
    uint lineNumber = 0;

    ColoredText() = default;
    ColoredText(QString a_text, int a_lineNumber, QRegularExpression const& regExp, int a_reOffset = 0, QColor a_bgColor = Qt::yellow, QColor a_fgColor = Qt::black)
    {
        QRegularExpressionMatchIterator matchIter = regExp.globalMatch(a_text, a_reOffset);
        if (matchIter.isValid() && matchIter.hasNext())
        {
            text = a_text;
            lineNumber = a_lineNumber;
            while (matchIter.hasNext())
            {
                QRegularExpressionMatch match = matchIter.next();
                ColoredSegment segment(match.capturedStart(0), match.capturedEnd(0), a_bgColor, a_fgColor);
                this->segments.append(segment);
            }
            normalize();
        }
    }

    void normalize()
    {
        if (text.isEmpty())
        {
            segments.clear();
            lineNumber = 0;
            return;
        }

        if (segments.size() == 0)
        {
            segments.prepend(ColoredSegment(0, text.length()));
            return;
        }

        for (auto& e : segments)
        {
            e.length = e.indexEnd - e.indexStart;
        }

        {
            ColoredSegment const& firstSeg = segments.first();
            if (firstSeg.indexStart != 0)
            {
                segments.prepend(ColoredSegment(0, firstSeg.indexStart));
            }
            ColoredSegment const& lastSeg = segments.last();
            if (lastSeg.indexEnd != text.length())
            {
                segments.append(ColoredSegment(lastSeg.indexEnd, text.length()));
            }
        }

        for (int i = 1; i < segments.size(); ++i)
        {
            ColoredSegment& curSeg = segments[i];
            ColoredSegment& prevSeg = segments[i - 1];
            if (prevSeg.indexEnd == curSeg.indexStart)
            {
                continue;
            }
            else if (prevSeg.indexEnd < curSeg.indexStart)
            {
                ColoredSegment newSeg(prevSeg.indexEnd, curSeg.indexStart);
                segments.insert(i, newSeg);
                ++i;
            }
            else if (prevSeg.indexEnd >= curSeg.indexEnd)
            {
                segments.removeAt(i);
                --i;
            }
            else if (prevSeg.indexEnd > curSeg.indexStart)
            {
                curSeg.indexStart = prevSeg.indexEnd;
                curSeg.length = curSeg.indexEnd - curSeg.indexStart;
            }
        }
    }
};
Q_DECLARE_METATYPE(ColoredText)


class MulticolorDelegate : public QStyledItemDelegate
{
public:
    MulticolorDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    std::pair<uint, QString> getLineNumberInfo(const QStyleOptionViewItem& option, const QModelIndex& index, const ColoredText& coloredText) const;
    int drawLineNumber(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, const QModelIndex& index, const ColoredText& coloredText) const;
    void drawText(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, const QModelIndex& index, const ColoredText& coloredText) const;
    void drawFocus(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect) const;
};


struct LayoutInfo
{
    QRect checkRect;
    QRect pixmapRect;
    QRect textRect;
    QRect lineNumberRect;
    QIcon icon;
    Qt::CheckState checkState;
    QStyleOptionViewItem option;
};

// NOTE: Might consider processing tabs as does QtCreator with SearchResultTreeItemDelegate::m_tabString
class MulticolorDelegateV2 : public QItemDelegate
{
public:
    MulticolorDelegateV2(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    // Why is QItemDelegate::setOptions() a castrated version of QStyledItemDelegate::initStyleOption()? Because fuck Qt
    QStyleOptionViewItem setOptions(const QModelIndex& index, const QStyleOptionViewItem& option) const;
    std::pair<uint, QString> getLineNumberInfo(const QStyleOptionViewItem& option, const QModelIndex& index, const ColoredText& coloredText) const;
    LayoutInfo getLayoutInfo(const QStyleOptionViewItem& option, const QModelIndex& index, const ColoredText& coloredText) const;
    int drawLineNumber(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, const QModelIndex& index, const ColoredText& coloredText) const;
    void drawText(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, const QModelIndex& index, const ColoredText& coloredText) const;
};

// Class implementation is based on SearchResultTreeItemDelegate from QtCreator source code
/* Useful links:
 * https://stackoverflow.com/questions/55923137/custom-qstyleditemdelegate-to-draw-text-with-multiple-colors
 * https://stackoverflow.com/questions/15368343/how-to-set-qtableview-as-a-cell-of-qtableview
 * https://stackoverflow.com/questions/1956542/how-to-make-item-view-render-rich-html-text-in-qt
 * https://stackoverflow.com/questions/8274123/position-of-icon-in-qtreewidgetitem/8274944#8274944
 * https://stackoverflow.com/questions/27723749/how-do-i-draw-a-styled-focus-rectangle-in-a-qstyleditemdelegate
*/
