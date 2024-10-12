#pragma once

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtWidgets/QItemDelegate>
#include <QtWidgets/QStyledItemDelegate>

class MulticolorDelegate : public QStyledItemDelegate
{
public:
    struct TextSegmentParameters
    {
        int index;
        int length;
        QColor textColor;
        QColor backgroundColor;
    };
    struct ColoredText
    {
        QString text;
        QVector<TextSegmentParameters> segments;
        uint lineNumber;
    };

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
Q_DECLARE_METATYPE(MulticolorDelegate::TextSegmentParameters)
Q_DECLARE_METATYPE(MulticolorDelegate::ColoredText)


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
    struct TextSegmentParameters
    {
        int index;
        int length;
        QColor textColor;
        QColor backgroundColor;
    };
    struct ColoredText
    {
        QString text;
        QVector<TextSegmentParameters> segments;
        uint lineNumber;
    };

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
Q_DECLARE_METATYPE(MulticolorDelegateV2::TextSegmentParameters)
Q_DECLARE_METATYPE(MulticolorDelegateV2::ColoredText)

// Class implementation is based on SearchResultTreeItemDelegate from QtCreator source code
/* Useful links:
 * https://stackoverflow.com/questions/55923137/custom-qstyleditemdelegate-to-draw-text-with-multiple-colors
 * https://stackoverflow.com/questions/15368343/how-to-set-qtableview-as-a-cell-of-qtableview
 * https://stackoverflow.com/questions/1956542/how-to-make-item-view-render-rich-html-text-in-qt
 * https://stackoverflow.com/questions/8274123/position-of-icon-in-qtreewidgetitem/8274944#8274944
 * https://stackoverflow.com/questions/27723749/how-do-i-draw-a-styled-focus-rectangle-in-a-qstyleditemdelegate
*/
