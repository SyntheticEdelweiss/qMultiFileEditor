#include <QtCore/QTimer>
#include <QtWidgets/QApplication>

#include "MultiFileEditor.h"

int main(int argc, char* argv[])
{
    QApplication::setStyle("Fusion");
    QApplication app(argc, argv);
    QDir::setCurrent(qApp->applicationDirPath());

    QTimer::singleShot(0, &app, [](){
        MultiFileEditor* w = new MultiFileEditor;
        w->show();
    });
    return app.exec();
}
