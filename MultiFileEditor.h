#ifndef MULTIFILEEDITOR_H
#define MULTIFILEEDITOR_H

#include "Utils.h"
#include "ui_MultiFileEditor.h"

#include <functional>
#include <stdexcept>
#include <array>

#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

struct FileDirEntry
{
    bool isExecutableTarget = false;
    QFileInfo fileInfo;
};

struct FileContentsEntry
{
    QFileInfo fileInfo;
    QStringList lines;
};

class MultiFileEditor : public QWidget
{
    Q_OBJECT
public:
    explicit MultiFileEditor(QWidget *parent = 0);
    ~MultiFileEditor();

private:
    bool m_isSearchDone = false;
    QHash<uintptr_t, FileDirEntry> m_fileDirEntryMap;
    QHash<uintptr_t, FileContentsEntry> m_fileContentsEntryMap;
    std::array<int, 3> m_resultsColumnWidth;

    QHash<QString, MFEPreset> m_presetMap;

private:
    QTreeWidgetItem* searchFileDirToRemove(QDir targetDir, QDir::Filters filters, QRegularExpression regExp, bool isRecursive);
    QTreeWidgetItem* searchFileDirToReplace(QDir targetDir, QDir::Filters filters, QRegularExpression regExp, QString replaceString, bool isRecursive);
    QList<QTreeWidgetItem*> searchFileContentsToReplace(QDir targetDir, QRegularExpression filePatternRegExp, QRegularExpression searchRegExp, QString replaceString, bool isRecursive);
    QList<QTreeWidgetItem*> searchFileContentsToReplace(QDir targetDir, QRegularExpression filePatternRegExp, QString searchString, QString replaceString, bool isRecursive, Qt::CaseSensitivity caseSensitivity);
    bool removeDirRecursively(QDir targetDir);

private slots:
    void onActionCombosActivated();
    void getExistingDirectory();

    bool checkDirectoryValidity();
    bool checkFilePatternValidity();
    bool checkSearchReplaceValidity();
    void checkAllValidity();

    void savePreset();
    void removePreset();
    void fillPreset(const QString& presetName);
    void loadAllPresets();

    void reset();
    void execute();

private:
    Ui::MultiFileEditor *ui;
};

#endif // MULTIFILEEDITOR_H
