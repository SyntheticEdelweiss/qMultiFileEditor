#include "MultiFileEditor.h"

#include <QtCore/QTimer>
#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
    QApplication::setStyle("Fusion");
    QApplication app(argc, argv);

    QTimer::singleShot(0, &app, [](){
        MultiFileEditor* w = new MultiFileEditor;
        w->show();
    });
    return app.exec();
}
