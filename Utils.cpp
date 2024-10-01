#include "MultiFileEditor.h"

QString regExpFromWildcardFilters(const QString& inputString)
{
    QStringList nameFilters;
    int idxEnd = 0;
    int idxBegin = inputString.indexOf("\"");
    while (idxBegin != -1)
    {
        ++idxBegin;
        idxEnd = inputString.indexOf("\"", idxBegin + 1);
        if (idxEnd == -1)
            break;
        nameFilters.push_back(inputString.mid(idxBegin, idxEnd - idxBegin));
        idxBegin = inputString.indexOf("\"", idxEnd + 1);
    }

    if (nameFilters.isEmpty())
        return QString();
    QString result = QRegularExpression::wildcardToRegularExpression(nameFilters.front());
    for (auto iter = nameFilters.begin() + 1; iter != nameFilters.end(); ++iter)
        result.append("|").append(QRegularExpression::wildcardToRegularExpression(*iter));
    return result;
}
