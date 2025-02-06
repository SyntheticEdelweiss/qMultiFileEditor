#include "MultiFileEditor.h"

#include <functional>

#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>


// #ifdef Q_OS_WIN
// #include "aclapi.h" // https://stackoverflow.com/questions/5021645/qt-setpermissions-not-setting-permisions
// #endif

#include <QtWidgets/QTextEdit>

MultiFileEditor::MultiFileEditor(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::MultiFileEditor)
{
    QApplication::setApplicationName("qMultiFileEditor");
    ui->setupUi(this);

    ui->comboBox_actionType->clear();
    ui->comboBox_actionType->addItem("Remove", static_cast<int>(ActionType::Remove));
    ui->comboBox_actionType->addItem("Replace", static_cast<int>(ActionType::Replace));

    ui->comboBox_actionTarget->clear();
    ui->comboBox_actionTarget->addItem("Files", static_cast<int>(ActionTarget::Files));
    ui->comboBox_actionTarget->addItem("Directories", static_cast<int>(ActionTarget::Dirs));
    ui->comboBox_actionTarget->addItem("Files & Directories", static_cast<int>(ActionTarget::FilesDirs));
    ui->comboBox_actionTarget->addItem("File contents", static_cast<int>(ActionTarget::FileContents));

    loadAllPresets();
    ui->comboBox_presets->setCurrentIndex(-1);

    connect(ui->comboBox_presets, &QComboBox::textActivated, this, &MultiFileEditor::fillPreset);
    connect(ui->pushButton_savePreset,   &QPushButton::clicked, this, &MultiFileEditor::savePreset);
    connect(ui->pushButton_removePreset, &QPushButton::clicked, this, &MultiFileEditor::removePreset);
    connect(ui->comboBox_actionType,    qOverload<int>(&QComboBox::activated), this, &MultiFileEditor::onActionCombosActivated);
    connect(ui->comboBox_actionTarget,  qOverload<int>(&QComboBox::activated), this, &MultiFileEditor::onActionCombosActivated);
    connect(ui->pushButton_browseDirectory, &QPushButton::clicked, this, &MultiFileEditor::getExistingDirectory);
    connect(ui->pushButton_reset,           &QPushButton::clicked, this, &MultiFileEditor::reset);
    connect(ui->pushButton_execute,         &QPushButton::clicked, this, &MultiFileEditor::execute);
    // TODO: optimize to omit excessive rechecking?
    connect(ui->lineEdit_dirPath,       &QLineEdit::editingFinished, this, &MultiFileEditor::checkAllValidity);
    connect(ui->lineEdit_filePattern,   &QLineEdit::editingFinished, this, &MultiFileEditor::checkAllValidity);
    connect(ui->lineEdit_searchFor,     &QLineEdit::editingFinished, this, &MultiFileEditor::checkAllValidity);
    connect(ui->lineEdit_replaceWith,   &QLineEdit::editingFinished, this, &MultiFileEditor::checkAllValidity);
    connect(ui->checkBox_isRegExpFilePattern,   &QCheckBox::clicked, this, &MultiFileEditor::checkAllValidity);
    connect(ui->checkBox_isRegExpSearchReplace, &QCheckBox::clicked, this, &MultiFileEditor::checkAllValidity);

    onActionCombosActivated();
}

MultiFileEditor::~MultiFileEditor()
{
    delete ui;
}

void MultiFileEditor::onActionCombosActivated()
{
    ActionType actionType = static_cast<ActionType>(ui->comboBox_actionType->currentData(Qt::UserRole).toInt());
    ActionTarget actionTarget = static_cast<ActionTarget>(ui->comboBox_actionTarget->currentData(Qt::UserRole).toInt());
    if (actionType == ActionType::Remove)
    {
        if (actionTarget == ActionTarget::FileContents)
        {
            ui->checkBox_isRegExpFilePattern->setEnabled(true);
            ui->checkBox_isRegExpSearchReplace->setEnabled(true);
            ui->lineEdit_filePattern->setEnabled(true);
            ui->lineEdit_searchFor->setEnabled(true);
            ui->lineEdit_replaceWith->setEnabled(false);
        }
        else if ((actionTarget == ActionTarget::Files) || (actionTarget == ActionTarget::Dirs) || (actionTarget == ActionTarget::FilesDirs))
        {
            ui->checkBox_isRegExpFilePattern->setEnabled(true);
            ui->checkBox_isRegExpSearchReplace->setEnabled(false);
            ui->lineEdit_filePattern->setEnabled(true);
            ui->lineEdit_searchFor->setEnabled(false);
            ui->lineEdit_replaceWith->setEnabled(false);

            ui->checkBox_isRegExpSearchReplace->setChecked(false);
        }
    }
    else if (actionType == ActionType::Replace)
    {
        if (actionTarget == ActionTarget::FileContents)
        {
            ui->checkBox_isRegExpFilePattern->setEnabled(true);
            ui->checkBox_isRegExpSearchReplace->setEnabled(true);
            ui->lineEdit_filePattern->setEnabled(true);
            ui->lineEdit_searchFor->setEnabled(true);
            ui->lineEdit_replaceWith->setEnabled(true);
        }
        else if ((actionTarget == ActionTarget::Files) || (actionTarget == ActionTarget::Dirs) || (actionTarget == ActionTarget::FilesDirs))
        {
            ui->checkBox_isRegExpFilePattern->setEnabled(false);
            ui->checkBox_isRegExpSearchReplace->setEnabled(false);
            ui->lineEdit_filePattern->setEnabled(false);
            ui->lineEdit_searchFor->setEnabled(true);
            ui->lineEdit_replaceWith->setEnabled(true);

            ui->checkBox_isRegExpFilePattern->setChecked(false);
            ui->checkBox_isRegExpSearchReplace->setChecked(true);
        }
    }
    checkAllValidity();
    return;
}

void MultiFileEditor::getExistingDirectory()
{
    QString startingPath(ui->lineEdit_dirPath->text());
    if (QDir(startingPath).exists() == false)
        startingPath = QDir::currentPath();
    QString dirPath = QFileDialog::getExistingDirectory(
        this, "Pick a directory", startingPath, QFileDialog::ShowDirsOnly);
    if (QDir(dirPath).exists())
        ui->lineEdit_dirPath->setText(dirPath);
    emit ui->lineEdit_dirPath->editingFinished();
    return;
}

bool MultiFileEditor::checkDirectoryValidity()
{
    bool isValid = true;
    if ((ui->lineEdit_dirPath->text().isEmpty() == false) && QDir(ui->lineEdit_dirPath->text()).exists())
    {
        isValid = true;
        ui->label_directoryCheckMark->setPixmap(QPixmap(":/Icons/checkmark_ok_16x16.png"));
        ui->label_directoryCheckMark->setToolTip("Directory is valid");
    }
    else
    {
        isValid = false;
        ui->label_directoryCheckMark->setPixmap(QPixmap(":/Icons/checkmark_error_16x16.png"));
        ui->label_directoryCheckMark->setToolTip("Directory doesn't exist");
    }
    return isValid;
}

bool MultiFileEditor::checkFilePatternValidity()
{
    bool isValid = true;
    if (ui->lineEdit_filePattern->isEnabled())
    {
        QRegularExpression regExp;
        QString pattern;
        if (ui->checkBox_isRegExpFilePattern->isChecked())
            pattern = ui->lineEdit_filePattern->text();
        else
            pattern = regExpFromWildcardFilters(ui->lineEdit_filePattern->text());
        regExp.setPattern(pattern);
        if (pattern.isEmpty() || !regExp.isValid())
        {
            isValid = false;
            ui->label_filePatternCheckMark->setPixmap(QPixmap(":/Icons/checkmark_error_16x16.png"));
            ui->label_filePatternCheckMark->setToolTip("File pattern is invalid");
        }
        else
        {
            isValid = true;
            ui->label_filePatternCheckMark->setPixmap(QPixmap(":/Icons/checkmark_ok_16x16.png"));
            ui->label_filePatternCheckMark->setToolTip("File pattern is valid");
        }
    }
    return isValid;
}

bool MultiFileEditor::checkSearchReplaceValidity()
{
    bool isValid = true;
    if (ui->lineEdit_searchFor->isEnabled())
    {
        if (ui->lineEdit_searchFor->text().isEmpty() ||
            (ui->checkBox_isRegExpSearchReplace->isChecked() && (QRegularExpression(ui->lineEdit_searchFor->text()).isValid() == false)))
        {
            isValid &= false;
            ui->label_searchForCheckMark->setPixmap(QPixmap(":/Icons/checkmark_error_16x16.png"));
            ui->label_searchForCheckMark->setToolTip("Search pattern is invalid");
        }
        else
        {
            isValid &= true;
            ui->label_searchForCheckMark->setPixmap(QPixmap(":/Icons/checkmark_ok_16x16.png"));
            ui->label_searchForCheckMark->setToolTip("Search pattern is valid");
        }
    }
    if (ui->lineEdit_replaceWith->isEnabled())
    {
        // Assume all replace patterns are valid. It's impossible to rename file\dir to something illegal and such attempt will be indicated anyway.
        isValid &= true;
        ui->label_replaceWithCheckMark->setPixmap(QPixmap(":/Icons/checkmark_ok_16x16.png"));
        ui->label_replaceWithCheckMark->setToolTip("Repalce pattern is valid");
    }
    return isValid;
}

void MultiFileEditor::checkAllValidity()
{
    bool isAllValid = true;
    isAllValid &= checkDirectoryValidity();
    isAllValid &= checkFilePatternValidity();
    isAllValid &= checkSearchReplaceValidity();
    if (isAllValid)
    {
        ui->pushButton_execute->setEnabled(true);
    }
    else
    {
        ui->pushButton_execute->setEnabled(false);
    }
    return;
}

void MultiFileEditor::savePreset()
{
    if (ui->comboBox_presets->currentText().isEmpty())
    {
        QMessageBox::critical(this, "Error", "Preset name can't be empty");
        return;
    }
    QSettings presetsFile(g_presetsPath, QSettings::IniFormat, this);
    presetsFile.beginGroup(ui->comboBox_presets->currentText());
    presetsFile.setValue("action_type", ui->comboBox_actionType->currentData(Qt::UserRole));
    presetsFile.setValue("action_target", ui->comboBox_actionTarget->currentData(Qt::UserRole));
    presetsFile.setValue("recursive", ui->checkBox_isRecursive->isChecked());
    presetsFile.setValue("case_sensitive", ui->checkBox_isCaseSensitive->isChecked());
    presetsFile.setValue("autoconfirm_execute", ui->checkBox_isAutoconfirmExecute->isChecked());
    presetsFile.setValue("re_file_pattern", ui->checkBox_isRegExpFilePattern->isChecked());
    presetsFile.setValue("re_search_replace", ui->checkBox_isRegExpSearchReplace->isChecked());
    presetsFile.setValue("highlight_match", ui->checkBox_isHighlightMatch->isChecked());
    presetsFile.setValue("dir_path", ui->lineEdit_dirPath->text());
    presetsFile.setValue("file_pattern", ui->lineEdit_filePattern->text());
    presetsFile.setValue("search_for", ui->lineEdit_searchFor->text());
    presetsFile.setValue("replace_with", ui->lineEdit_replaceWith->text());
    presetsFile.endGroup();

    MFEPreset& preset = m_presetMap[ui->comboBox_presets->currentText()];
    preset.actionType = ui->comboBox_actionType->currentData(Qt::UserRole).toInt();
    preset.actionTarget = ui->comboBox_actionTarget->currentData(Qt::UserRole).toInt();
    preset.isRecursive = ui->checkBox_isRecursive->isChecked();
    preset.isCaseSensitive = ui->checkBox_isCaseSensitive->isChecked();
    preset.isAutoconfirmExecute = ui->checkBox_isAutoconfirmExecute->isChecked();
    preset.isRegExpFilePattern = ui->checkBox_isRegExpFilePattern->isChecked();
    preset.isRegExpSearchReplace = ui->checkBox_isRegExpSearchReplace->isChecked();
    preset.isHighlightMatch = ui->checkBox_isHighlightMatch->isChecked();
    preset.dirPath = ui->lineEdit_dirPath->text();
    preset.filePattern = ui->lineEdit_filePattern->text();
    preset.searchFor = ui->lineEdit_searchFor->text();
    preset.replaceWith = ui->lineEdit_replaceWith->text();
    preset.presetName = ui->comboBox_presets->currentText();

    QMessageBox::information(this, "Saved", "Preset successfully saved");
    return;
}

void MultiFileEditor::removePreset()
{
    if (ui->comboBox_presets->currentText().isEmpty())
    {
        QMessageBox::critical(this, "Error", "Preset name can't be empty");
        return;
    }
    QComboBox* pBox = ui->comboBox_presets;
    QSettings presetsFile(g_presetsPath, QSettings::IniFormat, this);
    presetsFile.remove(pBox->currentText());
    pBox->removeItem(pBox->findText(pBox->currentText()));
    QMessageBox::information(this, "Removed", "Preset successfully removed");
    return;
}

void MultiFileEditor::fillPreset(const QString& presetName)
{
    MFEPreset& preset = m_presetMap[presetName]; // TODO: safer to use .find() ?
    ui->comboBox_actionType->setCurrentIndex(ui->comboBox_actionType->findData(preset.actionType, Qt::UserRole));
    ui->comboBox_actionTarget->setCurrentIndex(ui->comboBox_actionTarget->findData(preset.actionTarget, Qt::UserRole));
    ui->checkBox_isRecursive->setChecked(preset.isRecursive);
    ui->checkBox_isCaseSensitive->setChecked(preset.isCaseSensitive);
    ui->checkBox_isAutoconfirmExecute->setChecked(preset.isAutoconfirmExecute);
    ui->checkBox_isRegExpFilePattern->setChecked(preset.isRegExpFilePattern);
    ui->checkBox_isRegExpSearchReplace->setChecked(preset.isRegExpSearchReplace);
    ui->checkBox_isHighlightMatch->setChecked(preset.isHighlightMatch);
    ui->lineEdit_dirPath->setText(preset.dirPath);
    ui->lineEdit_filePattern->setText(preset.filePattern);
    ui->lineEdit_searchFor->setText(preset.searchFor);
    ui->lineEdit_replaceWith->setText(preset.replaceWith);
    onActionCombosActivated();
    return;
}

void MultiFileEditor::loadAllPresets()
{
    QSettings presetsFile(g_presetsPath, QSettings::IniFormat, this);
    QStringList presets = presetsFile.childGroups();
    std::sort(presets.begin(), presets.end());
    for (const QString& curPresetName : qAsConst(presets))
    {
        presetsFile.beginGroup(curPresetName);
        MFEPreset& curPreset = m_presetMap[curPresetName];
        curPreset.actionType = presetsFile.value("action_type").toUInt();
        curPreset.actionTarget = presetsFile.value("action_target").toInt();
        curPreset.isRecursive = presetsFile.value("recursive").toBool();
        curPreset.isCaseSensitive = presetsFile.value("case_sensitive").toBool();
        curPreset.isAutoconfirmExecute = presetsFile.value("autoconfirm_execute").toBool();
        curPreset.isRegExpFilePattern = presetsFile.value("re_file_pattern").toBool();
        curPreset.isRegExpSearchReplace = presetsFile.value("re_search_replace").toBool();
        curPreset.isHighlightMatch = presetsFile.value("highlight_match").toBool();
        curPreset.dirPath = presetsFile.value("dir_path").toString();
        curPreset.filePattern = presetsFile.value("file_pattern").toString();
        curPreset.searchFor = presetsFile.value("search_for").toString();
        curPreset.replaceWith = presetsFile.value("replace_with").toString();
        curPreset.presetName = curPresetName;
        presetsFile.endGroup();
    }
    ui->comboBox_presets->addItems(presets);
    return;
}

void MultiFileEditor::reset()
{
    ui->label_resultsText->clear();
    ui->treeWidget_results->clear();
    m_fileDirEntryMap.clear();
    m_fileContentsEntryMap.clear();
    m_isSearchDone = false;
    ui->pushButton_execute->setText("Search");
    ui->frame_settings->setEnabled(true);
    return;
}

void MultiFileEditor::execute()
{
    ActionType actionType = static_cast<ActionType>(ui->comboBox_actionType->currentData(Qt::UserRole).toInt());
    ActionTarget actionTarget = static_cast<ActionTarget>(ui->comboBox_actionTarget->currentData(Qt::UserRole).toInt());
    if (m_isSearchDone) // execute action
    {
        if (ui->checkBox_isAutoconfirmExecute->isChecked() == false)
        {
            int ret = QMessageBox::question(this, "Perform execute?",
                                            "Are you sure you want to execute selected action?");
            if (ret != QMessageBox::Yes)
                return;
        }

        if (actionTarget == ActionTarget::FileContents)
        {
            // Remove is just Replace with empty replaceWith string
            if ((actionType == ActionType::Remove) || (actionType == ActionType::Replace))
            {
                uint fileSuccessCount = 0;
                uint lineSuccessCount = 0;
                uint fileFailCount = 0;
                uint lineFailCount = 0;

                QTreeWidgetItemIterator fileItemIter(ui->treeWidget_results, QTreeWidgetItemIterator::HasChildren);
                for (; *fileItemIter != nullptr; ++fileItemIter)
                {
                    if ((*fileItemIter)->checkState(0) == Qt::Unchecked)
                        continue;
                    auto entryIter = m_fileContentsEntryMap.find(reinterpret_cast<uintptr_t>(*fileItemIter));
                    QFile file(entryIter->fileInfo.canonicalFilePath());
                    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                    {
                        (*fileItemIter)->setData(2, Qt::DisplayRole, "Failed to open file");
                        (*fileItemIter)->setIcon(2, QIcon(":/Icons/checkmark_error_16x16.png"));
                        ++fileFailCount;
                        lineFailCount += (*fileItemIter)->childCount();
                        continue;
                    }

                    QStringList& linesList = entryIter.value().lines;
                    QTreeWidgetItemIterator lineItemIter((*fileItemIter)->child(0));
                    for (int i = 0; i < (*fileItemIter)->childCount(); ++i, ++lineItemIter)
                    {
                        (*lineItemIter)->setIcon(2, QIcon(":/Icons/checkmark_ok_16x16.png"));
                        linesList[(*lineItemIter)->data(0, Qt::UserRole).toUInt()] = (*lineItemIter)->data(1, Qt::DisplayRole).toString();
                        ++lineSuccessCount;
                    }
                    ++fileSuccessCount;

                    QTextStream fileStream(&file);
                    for (auto& line : entryIter->lines)
                        fileStream << line << '\n';
                    file.close();
                }
                QString resultMessage(QString("Edited entries: %1 lines in %2 files")
                                      .arg(fileSuccessCount)
                                      .arg(lineSuccessCount));
                if (fileFailCount > 0)
                    resultMessage.append(QString(". Failed to edit: %3 lines in %4 files")
                                         .arg(fileFailCount)
                                         .arg(lineFailCount));
                ui->label_resultsText->setText(resultMessage);
            }
            else
            {
                QMessageBox::critical(this, "MultiFileEditor error",
                                      "Encountered unsupported Action Type."
                                      "\nIf it's caused by incorrect preset - fix the preset."
                                      "\nIf it's caused by something else - it requires code fixing."
                                      "\nThe program will now be terminated.");
                throw std::runtime_error("Encountered unsupported Action Type which shouldn't even be possible."
                                         "This requires code fixing. The program will now be terminated.");
            }
        }
        else if (under_cast(actionTarget & ActionTarget::FilesDirs) != 0)
        {
            if (actionType == ActionType::Remove)
            {
                uint dirSuccessCount = 0;
                uint fileSuccessCount = 0;
                uint dirFailCount = 0;
                uint fileFailCount = 0;
                QTreeWidgetItemIterator treeIter(ui->treeWidget_results, QTreeWidgetItemIterator::NoChildren);
                for (; *treeIter != nullptr; ++treeIter)
                {
                    if ((*treeIter)->checkState(0) == Qt::Unchecked)
                        continue;
                    auto mapIter = m_fileDirEntryMap.find(reinterpret_cast<uintptr_t>(*treeIter));
                    if ((mapIter != m_fileDirEntryMap.end()) && (mapIter.value().isExecutableTarget == true))
                    {
                        auto entryFileInfo = mapIter.value().fileInfo;
                        if (entryFileInfo.isDir())
                        {
                            bool isOk = removeDirRecursively(QDir(entryFileInfo.canonicalFilePath()));
                            if (isOk)
                            {
                                ++dirSuccessCount;
                                (*treeIter)->setIcon(2, QIcon(":/Icons/checkmark_ok_16x16.png"));
                            }
                            else
                            {
                                ++dirFailCount;
                                (*treeIter)->setIcon(2, QIcon(":/Icons/checkmark_error_16x16.png"));
                            }
                        }
                        else
                        {
                            QFile fileToRemove = entryFileInfo.canonicalFilePath();
                            fileToRemove.setPermissions(QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther);
                            const bool isOk = fileToRemove.remove();
                            if (isOk)
                            {
                                ++fileSuccessCount;
                                (*treeIter)->setIcon(2, QIcon(":/Icons/checkmark_ok_16x16.png"));
                            }
                            else
                            {
                                ++fileFailCount;
                                (*treeIter)->setIcon(2, QIcon(":/Icons/checkmark_error_16x16.png"));
                            }
                        }
                    }
                }
                QString resultMessage(QString("Removed entries: %1 directories and %2 files")
                                      .arg(dirSuccessCount)
                                      .arg(fileSuccessCount));
                if ((dirFailCount > 0) || (fileFailCount > 0))
                    resultMessage.append(QString(". Failed to remove: %3 directories and %4 files")
                                         .arg(dirFailCount)
                                         .arg(fileFailCount));
                ui->label_resultsText->setText(resultMessage);
            }
            else if (actionType == ActionType::Replace)
            {
                uint dirSuccessCount = 0;
                uint fileSuccessCount = 0;
                uint dirFailCount = 0;
                uint fileFailCount = 0;

                QTreeWidgetItemIterator treeIter(ui->treeWidget_results);
                QTreeWidgetItemIterator iterNext(ui->treeWidget_results);
                while (*(++iterNext) != nullptr)
                    ++treeIter;
                for (; *treeIter != nullptr; --treeIter)
                {
                    if ((*treeIter)->checkState(0) == Qt::Unchecked)
                        continue;
                    auto entryIter = m_fileDirEntryMap.find(reinterpret_cast<uintptr_t>(*treeIter));
                    if ((entryIter != m_fileDirEntryMap.end()) && (entryIter.value().isExecutableTarget == true))
                    {
                        auto entryFileInfo = entryIter.value().fileInfo;
                        if (entryFileInfo.isDir())
                        {
                            bool isOk = QDir(entryFileInfo.canonicalPath())
                                        .rename(entryFileInfo.fileName(),
                                                (*treeIter)->text(1));
                            if (isOk)
                            {
                                ++dirSuccessCount;
                                (*treeIter)->setIcon(2, QIcon(":/Icons/checkmark_ok_16x16.png"));
                            }
                            else
                            {
                                ++dirFailCount;
                                (*treeIter)->setIcon(2, QIcon(":/Icons/checkmark_error_16x16.png"));
                            }
                        }
                        else
                        {
                            bool isOk = QDir(entryFileInfo.canonicalPath())
                                        .rename(entryFileInfo.fileName(),
                                                (*treeIter)->text(1));
                            if (isOk)
                            {
                                ++fileSuccessCount;
                                (*treeIter)->setIcon(2, QIcon(":/Icons/checkmark_ok_16x16.png"));
                            }
                            else
                            {
                                ++fileFailCount;
                                (*treeIter)->setIcon(2, QIcon(":/Icons/checkmark_error_16x16.png"));
                            }
                        }
                    }
                }
                QString resultMessage(QString("Renamed entries: %1 directories and %2 files")
                                      .arg(dirSuccessCount)
                                      .arg(fileSuccessCount));
                if ((dirFailCount > 0) || (fileFailCount > 0))
                    resultMessage.append(QString(". Failed to rename: %3 directories and %4 files")
                                         .arg(dirFailCount)
                                         .arg(fileFailCount));
                ui->label_resultsText->setText(resultMessage);
            }
            else
            {
                QMessageBox::critical(this, "MultiFileEditor error",
                                      "Encountered unsupported Action Type."
                                      "\nIf it's caused by incorrect preset - fix the preset."
                                      "\nIf it's caused by something else - it requires code fixing."
                                      "\nThe program will now be terminated.");
                throw std::runtime_error("Encountered unsupported Action Type which shouldn't even be possible."
                                         "This requires code fixing. The program will now be terminated.");
            }
        }
        else
        {
            QMessageBox::critical(this, "MultiFileEditor error",
                                  "Encountered unsupported Action Target."
                                  "\nIf it's caused by incorrect preset - fix the preset."
                                  "\nIf it's caused by something else - it requires code fixing."
                                  "\nThe program will now be terminated.");
            throw std::runtime_error("Encountered unsupported Action Target which shouldn't even be possible."
                                     "This requires code fixing. The program will now be terminated.");
        }
    }
    else // perform search
    {
        ui->label_resultsText->clear();
        ui->treeWidget_results->clear();
        m_fileDirEntryMap.clear();
        m_fileContentsEntryMap.clear();

        if (actionTarget == ActionTarget::FileContents)
        {
            if (actionType == ActionType::Remove)
            {
                QDir targetDir(ui->lineEdit_dirPath->text());
                bool isRecursive = ui->checkBox_isRecursive->isChecked();
                Qt::CaseSensitivity caseSensitivity = ui->checkBox_isCaseSensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;

                QRegularExpression filePatternRegExp;
                QString filePatternString = ui->lineEdit_filePattern->text();
                if (ui->checkBox_isRegExpFilePattern->isChecked() == false)
                    filePatternString = regExpFromWildcardFilters(filePatternString);
                filePatternRegExp.setPattern(filePatternString);
                if (ui->checkBox_isCaseSensitive->isChecked() == false)
                    filePatternRegExp.setPatternOptions(filePatternRegExp.patternOptions() | QRegularExpression::CaseInsensitiveOption);

                if (ui->checkBox_isRegExpSearchReplace->isChecked())
                {
                    QRegularExpression searchRegExp(ui->lineEdit_searchFor->text());
                    if (ui->checkBox_isCaseSensitive->isChecked() == false)
                        searchRegExp.setPatternOptions(searchRegExp.patternOptions() | QRegularExpression::CaseInsensitiveOption);
                    ui->treeWidget_results->addTopLevelItems(searchFileContentsToReplace(targetDir, filePatternRegExp, searchRegExp, QString(), isRecursive));
                }
                else
                    ui->treeWidget_results->addTopLevelItems(searchFileContentsToReplace(targetDir, filePatternRegExp, ui->lineEdit_searchFor->text(), QString(), isRecursive, caseSensitivity));
            }
            else if (actionType == ActionType::Replace)
            {
                QDir targetDir(ui->lineEdit_dirPath->text());
                bool isRecursive = ui->checkBox_isRecursive->isChecked();
                Qt::CaseSensitivity caseSensitivity = ui->checkBox_isCaseSensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;

                QRegularExpression filePatternRegExp;
                QString filePatternString = ui->lineEdit_filePattern->text();
                if (ui->checkBox_isRegExpFilePattern->isChecked() == false)
                    filePatternString = regExpFromWildcardFilters(filePatternString);
                filePatternRegExp.setPattern(filePatternString);
                if (ui->checkBox_isCaseSensitive->isChecked() == false)
                    filePatternRegExp.setPatternOptions(filePatternRegExp.patternOptions() | QRegularExpression::CaseInsensitiveOption);

                if (ui->checkBox_isRegExpSearchReplace->isChecked())
                {
                    QRegularExpression searchRegExp(ui->lineEdit_searchFor->text());
                    if (ui->checkBox_isCaseSensitive->isChecked() == false)
                        searchRegExp.setPatternOptions(searchRegExp.patternOptions() | QRegularExpression::CaseInsensitiveOption);
                    ui->treeWidget_results->addTopLevelItems(searchFileContentsToReplace(targetDir, filePatternRegExp, searchRegExp, ui->lineEdit_replaceWith->text(), isRecursive));
                }
                else
                    ui->treeWidget_results->addTopLevelItems(searchFileContentsToReplace(targetDir, filePatternRegExp, ui->lineEdit_searchFor->text(), ui->lineEdit_replaceWith->text(), isRecursive, caseSensitivity));
            }
            else
            {
                QMessageBox::critical(this, "MultiFileEditor error",
                                      "Encountered unsupported Action Type."
                                      "\nIf it's caused by incorrect preset - fix the preset."
                                      "\nIf it's caused by something else - it requires code fixing."
                                      "\nThe program will now be terminated.");
                throw std::runtime_error("Encountered unsupported Action Type which shouldn't even be possible."
                                         "This requires code fixing. The program will now be terminated.");
            }
        }
        else if (under_cast(actionTarget & ActionTarget::FilesDirs) != 0)
        {
            if (actionType == ActionType::Remove)
            {
                QDir targetDir(ui->lineEdit_dirPath->text());
                QDir::Filters filters = QDir::NoDotAndDotDot | static_cast<QDir::Filters>(static_cast<int>(actionTarget));
                bool isRecursive = ui->checkBox_isRecursive->isChecked();

                QTreeWidgetItem* pRootItem = nullptr;
                QRegularExpression regExp;
                QString regExpPattern = ui->lineEdit_filePattern->text();
                if (ui->checkBox_isRegExpFilePattern->isChecked() == false)
                    regExpPattern = regExpFromWildcardFilters(regExpPattern);
                regExp.setPattern(regExpPattern);
                regExp.setPatternOptions(regExp.patternOptions() | QRegularExpression::DontCaptureOption);
                if (ui->checkBox_isCaseSensitive->isChecked() == false)
                    regExp.setPatternOptions(regExp.patternOptions() | QRegularExpression::CaseInsensitiveOption);

                if (regExpPattern.isEmpty())
                    pRootItem = new QTreeWidgetItem;
                else
                    pRootItem = searchFileDirToRemove(targetDir, filters, regExp, isRecursive);
                pRootItem->setData(0, Qt::DisplayRole, targetDir.canonicalPath());
                pRootItem->setIcon(0, QIcon(":/Icons/folder_15x15.png"));
                m_fileDirEntryMap.insert(reinterpret_cast<uintptr_t>(pRootItem), {false, QFileInfo(targetDir.canonicalPath())});
                ui->treeWidget_results->addTopLevelItem(pRootItem);
            }
            else if (actionType == ActionType::Replace)
            {
                QDir targetDir(ui->lineEdit_dirPath->text());
                QDir::Filters filters = QDir::NoDotAndDotDot | static_cast<QDir::Filters>(static_cast<int>(actionTarget));
                bool isRecursive = ui->checkBox_isRecursive->isChecked();

                QTreeWidgetItem* pRootItem = nullptr;
                QRegularExpression regExp;
                QString regExpPattern = ui->lineEdit_searchFor->text();
                regExp.setPattern(regExpPattern);
                if (ui->checkBox_isCaseSensitive->isChecked() == false)
                    regExp.setPatternOptions(regExp.patternOptions() | QRegularExpression::CaseInsensitiveOption);

                if (regExpPattern.isEmpty())
                    pRootItem = new QTreeWidgetItem;
                else
                    pRootItem = searchFileDirToReplace(targetDir, filters, regExp, ui->lineEdit_replaceWith->text(), isRecursive);
                pRootItem->setData(0, Qt::DisplayRole, targetDir.canonicalPath());
                pRootItem->setIcon(0, QIcon(":/Icons/folder_15x15.png"));
                m_fileDirEntryMap.insert(reinterpret_cast<uintptr_t>(pRootItem), {false, QFileInfo(targetDir.canonicalPath())});
                ui->treeWidget_results->addTopLevelItem(pRootItem);
            }
            else
            {
                QMessageBox::critical(this, "MultiFileEditor error",
                                      "Encountered unsupported Action Type."
                                      "\nIf it's caused by incorrect preset - fix the preset."
                                      "\nIf it's caused by something else - it requires code fixing."
                                      "\nThe program will now be terminated.");
                throw std::runtime_error("Encountered unsupported Action Type which shouldn't even be possible."
                                         "This requires code fixing. The program will now be terminated.");
            }
        }
        else
        {
            QMessageBox::critical(this, "MultiFileEditor error",
                                  "Encountered unsupported Action Target."
                                  "\nIf it's caused by incorrect preset - fix the preset."
                                  "\nIf it's caused by something else - it requires code fixing."
                                  "\nThe program will now be terminated.");
            throw std::runtime_error("Encountered unsupported Action Target which shouldn't even be possible."
                                     "This requires code fixing. The program will now be terminated.");
        }
    }
    ui->treeWidget_results->expandAll();
    ui->treeWidget_results->resizeColumnToContents(0);
    ui->treeWidget_results->resizeColumnToContents(1);
    m_isSearchDone = !m_isSearchDone;
    ui->pushButton_execute->setText(m_isSearchDone ? "Execute" : "Search");
    ui->frame_settings->setEnabled(!m_isSearchDone);
    return;
}

QTreeWidgetItem* MultiFileEditor::searchFileDirToRemove(QDir targetDir, QDir::Filters filters, QRegularExpression regExp, bool isRecursive)
{
    QTreeWidgetItem* retItem = new QTreeWidgetItem;
    bool isDeleteDirs = ((filters & QDir::Dirs) == QDir::Dirs);
    bool isDeleteFiles = ((filters & QDir::Files) == QDir::Files);

    QStringList nameFilters({"*"});
    QDir::SortFlags sortFlags = QDir::Name | QDir::DirsFirst;
    QList<QFileInfo> allFileDirs = targetDir.entryInfoList(nameFilters, filters, sortFlags);

    auto iter = allFileDirs.begin();
    // entryInfoList is guaranteed to only contain dirs and\or files (as set by filters)
    // and it's guaranteed that all dirs will be placed before files (as set by sortFlags)
    if (isDeleteDirs || isRecursive)
        for (; (iter != allFileDirs.end()) && iter->isDir(); ++iter)
        {
            if (isDeleteDirs && regExp.match(iter->fileName()).hasMatch())
            {
                QTreeWidgetItem* pChildItem = new QTreeWidgetItem;
                pChildItem->setData(0, Qt::DisplayRole, iter->fileName());
                pChildItem->setIcon(0, QIcon(":/Icons/folder_15x15.png"));
                pChildItem->setCheckState(0, Qt::Checked);
                retItem->addChild(pChildItem);
                m_fileDirEntryMap.insert(reinterpret_cast<uintptr_t>(pChildItem), {true, *iter});
            }
            else if (isRecursive)
            {
                QTreeWidgetItem* pChildItem = searchFileDirToRemove(QDir(iter->canonicalFilePath()), filters, regExp, isRecursive);
                if (pChildItem->childCount() != 0)
                {
                    pChildItem->setData(0, Qt::DisplayRole, iter->fileName());
                    pChildItem->setIcon(0, QIcon(":/Icons/folder_15x15.png"));
                    retItem->addChild(pChildItem);
                }
                else
                    delete pChildItem;
            }
        }

    // and it's guaranteed that only files will be from here on out
    if (isDeleteFiles)
        for (; iter != allFileDirs.end(); ++iter)
            if (regExp.match(iter->fileName()).hasMatch())
            {
                QTreeWidgetItem* pChildItem = new QTreeWidgetItem;
                pChildItem->setData(0, Qt::DisplayRole, iter->fileName());
                pChildItem->setIcon(0, QIcon(":/Icons/file_12x15.png"));
                pChildItem->setCheckState(0, Qt::Checked);
                retItem->addChild(pChildItem);
                m_fileDirEntryMap.insert(reinterpret_cast<uintptr_t>(pChildItem), {true, *iter});
            }
    return retItem;
}

QTreeWidgetItem* MultiFileEditor::searchFileDirToReplace(QDir targetDir, QDir::Filters filters, QRegularExpression regExp, QString replaceWith, bool isRecursive)
{
    QTreeWidgetItem* retItem = new QTreeWidgetItem;
    bool isRenameDirs = ((filters & QDir::Dirs) == QDir::Dirs);
    bool isRenameFiles = ((filters & QDir::Files) == QDir::Files);

    QStringList nameFilters({"*"});
    QDir::SortFlags sortFlags = QDir::Name | QDir::DirsFirst;
    QList<QFileInfo> allFileDirs = targetDir.entryInfoList(nameFilters, filters, sortFlags);

    auto iter = allFileDirs.begin();
    // entryInfoList is guaranteed to only contain dirs and\or files (as set by filters)
    // and it's guaranteed that all dirs will be placed before files (as set by sortFlags)
    if (isRenameDirs || isRecursive)
        for (; (iter != allFileDirs.end()) && iter->isDir(); ++iter)
        {
            QTreeWidgetItem* pChildItem = nullptr;
            if (isRecursive)
            {
                pChildItem = searchFileDirToReplace(QDir(iter->canonicalFilePath()), filters, regExp, replaceWith, isRecursive);
                if (pChildItem->childCount() == 0)
                {
                    delete pChildItem;
                    pChildItem = nullptr;
                }
                else
                {
                    pChildItem->setData(0, Qt::DisplayRole, iter->fileName());
                    pChildItem->setIcon(0, QIcon(":/Icons/folder_15x15.png"));
                    retItem->addChild(pChildItem);
                }
            }
            if (isRenameDirs && regExp.match(iter->fileName()).hasMatch())
            {
                if (pChildItem == nullptr)
                {
                    pChildItem = new QTreeWidgetItem;
                    pChildItem->setData(0, Qt::DisplayRole, iter->fileName());
                    pChildItem->setIcon(0, QIcon(":/Icons/folder_15x15.png"));
                    retItem->addChild(pChildItem);
                }
                pChildItem->setData(1, Qt::DisplayRole, iter->fileName().replace(regExp, replaceWith));
                pChildItem->setCheckState(0, Qt::Checked);
                m_fileDirEntryMap.insert(reinterpret_cast<uintptr_t>(pChildItem), {true, *iter});
            }
        }

    // and it's guaranteed that only files will be from here on out
    if (isRenameFiles)
        for (; iter != allFileDirs.end(); ++iter)
            if (regExp.match(iter->fileName()).hasMatch())
            {
                QTreeWidgetItem* pChildItem = new QTreeWidgetItem;
                pChildItem->setData(0, Qt::DisplayRole, iter->fileName());
                pChildItem->setData(1, Qt::DisplayRole, iter->fileName().replace(regExp, replaceWith));
                pChildItem->setIcon(0, QIcon(":/Icons/file_12x15.png"));
                pChildItem->setCheckState(0, Qt::Checked);
                retItem->addChild(pChildItem);
                m_fileDirEntryMap.insert(reinterpret_cast<uintptr_t>(pChildItem), {true, *iter});
            }
    return retItem;
}

QList<QTreeWidgetItem*> MultiFileEditor::searchFileContentsToReplace(QDir targetDir, QRegularExpression filePatternRegExp, QRegularExpression searchRegExp, QString replaceString, bool isRecursive)
{
    QList<QTreeWidgetItem*> retList;
    QStringList nameFilters({"*"});
    QDir::Filters filters = QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files;
    QDir::SortFlags sortFlags = QDir::Name | QDir::DirsFirst;
    QList<QFileInfo> allFileDirs = targetDir.entryInfoList(nameFilters, filters, sortFlags);

    auto iter = allFileDirs.begin();
    // entryInfoList is guaranteed to only contain dirs and files (as set by filters)
    // and it's guaranteed that all dirs will be placed before files (as set by sortFlags)
    if (isRecursive)
        for (; (iter != allFileDirs.end()) && iter->isDir(); ++iter)
            retList.append(searchFileContentsToReplace(iter->canonicalFilePath(), filePatternRegExp, searchRegExp, replaceString, isRecursive));
    else
        while ((iter != allFileDirs.end()) && iter->isDir())
            ++iter;

    // and it's guaranteed that only files will be from here on out
    for (; iter != allFileDirs.end(); ++iter)
        if (filePatternRegExp.match(iter->fileName()).hasMatch())
        {
            QTreeWidgetItem* pFileItem = new QTreeWidgetItem;
            pFileItem->setData(0, Qt::DisplayRole, iter->canonicalFilePath());
            pFileItem->setIcon(0, QIcon(":/Icons/file_12x15.png"));
            QFile file(iter->canonicalFilePath());
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                pFileItem->setData(2, Qt::DisplayRole, "Failed to open file");
                pFileItem->setIcon(2, QIcon(":/Icons/checkmark_error_16x16.png"));
                continue;
            }
            pFileItem->setFlags(pFileItem->flags() | Qt::ItemIsAutoTristate);
            auto entry = m_fileContentsEntryMap.insert(reinterpret_cast<uintptr_t>(pFileItem), {*iter, QStringList()});
            QStringList& fileLines = entry.value().lines;

            uint lineIdx = 0;
            QTextStream fileStream(&file);
            while (!fileStream.atEnd())
            {
                QString line = fileStream.readLine();
                if (searchRegExp.match(line).hasMatch())
                {
                    QTreeWidgetItem* pLineItem = new QTreeWidgetItem;
                    QString postReplaceLine = line;
                    postReplaceLine.replace(searchRegExp, replaceString);
                    pLineItem->setData(0, Qt::DisplayRole, line);
                    pLineItem->setData(0, Qt::UserRole, lineIdx);
                    pLineItem->setData(1, Qt::DisplayRole, postReplaceLine);
                    pLineItem->setCheckState(0, Qt::Checked);
                    pFileItem->addChild(pLineItem);
                }
                fileLines.append(line);
                ++lineIdx;
            }

            if (pFileItem->childCount() != 0)
            {
                retList.append(pFileItem);
                while (fileLines.back().isEmpty())
                    fileLines.removeLast();
            }
            else
            {
                m_fileContentsEntryMap.erase(entry);
                delete pFileItem;
            }
        }
    return retList;
}

QList<QTreeWidgetItem*> MultiFileEditor::searchFileContentsToReplace(QDir targetDir, QRegularExpression filePatternRegExp, QString searchString, QString replaceString, bool isRecursive, Qt::CaseSensitivity caseSensitivity)
{
    QList<QTreeWidgetItem*> retList;
    QStringList nameFilters({"*"});
    QDir::Filters filters = QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files;
    QDir::SortFlags sortFlags = QDir::Name | QDir::DirsFirst;
    QList<QFileInfo> allFileDirs = targetDir.entryInfoList(nameFilters, filters, sortFlags);

    auto iter = allFileDirs.begin();
    // entryInfoList is guaranteed to only contain dirs and files (as set by filters)
    // and it's guaranteed that all dirs will be placed before files (as set by sortFlags)
    if (isRecursive)
        for (; (iter != allFileDirs.end()) && iter->isDir(); ++iter)
            retList.append(searchFileContentsToReplace(iter->canonicalFilePath(), filePatternRegExp, searchString, replaceString, isRecursive, caseSensitivity));
    else
        while ((iter != allFileDirs.end()) && iter->isDir())
            ++iter;

    // and it's guaranteed that only files will be from here on out
    for (; iter != allFileDirs.end(); ++iter)
        if (filePatternRegExp.match(iter->fileName()).hasMatch())
        {
            QTreeWidgetItem* pFileItem = new QTreeWidgetItem;
            pFileItem->setData(0, Qt::DisplayRole, iter->canonicalFilePath());
            pFileItem->setIcon(0, QIcon(":/Icons/file_12x15.png"));
            QFile file(iter->canonicalFilePath());
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                pFileItem->setData(2, Qt::DisplayRole, "Failed to open file");
                pFileItem->setIcon(2, QIcon(":/Icons/checkmark_error_16x16.png"));
                continue;
            }
            pFileItem->setFlags(pFileItem->flags() | Qt::ItemIsAutoTristate);
            auto entry = m_fileContentsEntryMap.insert(reinterpret_cast<uintptr_t>(pFileItem), {*iter, QStringList()});
            QStringList& fileLines = entry.value().lines;

            uint lineIdx = 0;
            QTextStream fileStream(&file);
            while (!fileStream.atEnd())
            {
                QString line = fileStream.readLine();
                if (line.contains(searchString, caseSensitivity))
                {
                    QTreeWidgetItem* pLineItem = new QTreeWidgetItem;
                    QString postReplaceLine = line;
                    postReplaceLine.replace(searchString, replaceString, caseSensitivity);
                    pLineItem->setData(0, Qt::DisplayRole, line);
                    pLineItem->setData(0, Qt::UserRole, lineIdx);
                    pLineItem->setData(1, Qt::DisplayRole, postReplaceLine);
                    pLineItem->setCheckState(0, Qt::Checked);
                    pFileItem->addChild(pLineItem);
                }
                fileLines.append(line);
            }

            if (pFileItem->childCount() != 0)
            {
                retList.append(pFileItem);
                while (fileLines.back().isEmpty())
                    fileLines.removeLast();
            }
            else
            {
                m_fileContentsEntryMap.erase(entry);
                delete pFileItem;
            }
        }
    return retList;
}

bool MultiFileEditor::removeDirRecursively(QDir targetDir)
{
    if (targetDir.exists() == false)
        return false;
    QStringList nameFilters({"*"});
    QDir::Filters filters = QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files | QDir::Hidden;
    QDir::SortFlags sortFlags = QDir::DirsFirst;
    QList<QFileInfo> entriesToRemove = targetDir.entryInfoList(nameFilters, filters, sortFlags);
    for (auto iter = entriesToRemove.begin(); iter != entriesToRemove.end(); ++iter)
    {
        if (iter->isDir())
        {
            removeDirRecursively(iter->canonicalFilePath());
        }
        else
        {
#ifdef Q_OS_WIN // https://learn.microsoft.com/en-us/windows/win32/fileio/file-security-and-access-rights
            // SetNamedSecurityInfoA(entry.absoluteFilePath(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL);
#endif
            QFile fileToRemove(iter->canonicalFilePath());
            // If fails on Windows, check https://doc.qt.io/qt-5.15/qfileinfo.html#ntfs-permissions
            fileToRemove.setPermissions(QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther);
            fileToRemove.remove();
        }
    }    
    return targetDir.rmdir(".");
}
