// qtDemo.cpp
//
// Minimal Qt GUI executable — verifies that the library links and works.

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Qt inside LibXLL");
    window.resize(340, 160);

    auto* layout = new QVBoxLayout(&window);
    auto* label  = new QLabel("Hello from Qt!");
    label->setAlignment(Qt::AlignCenter);
    QFont font = label->font();
    font.setBold(true);
    font.setPointSize(14);
    label->setFont(font);

    auto* button = new QPushButton("Greet");
    QObject::connect(button, &QPushButton::clicked, [&window]() {
        QMessageBox::information(&window, "Greeting", "Hello from Qt!");
    });

    layout->addWidget(label);
    layout->addWidget(button);

    window.show();
    return app.exec();
}

