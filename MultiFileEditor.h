#pragma once

#include <array>

#include <QtCore/QDir>

#include "Utils.h"
#include "ui_MultiFileEditor.h"


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

    bool isRecursive = false;
    bool isHighlight = false;
    Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;

private:
    QTreeWidgetItem* searchFileDirToRemove(QDir targetDir, QDir::Filters filters, QRegularExpression regExp);
    QTreeWidgetItem* searchFileDirToReplace(QDir targetDir, QDir::Filters filters, QRegularExpression regExp, QString replaceString);
    QList<QTreeWidgetItem*> searchFileContentsToReplace(QDir targetDir, QRegularExpression filePatternRegExp, QRegularExpression searchRegExp, QString replaceString);
    QList<QTreeWidgetItem*> searchFileContentsToReplace(QDir targetDir, QRegularExpression filePatternRegExp, QString searchString, QString replaceString);
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
    void loadSettings();
    void loadAllPresets();

    void reset();
    void execute();

    void closeEvent(QCloseEvent* event) final;

private:
    Ui::MultiFileEditor *ui;
};
