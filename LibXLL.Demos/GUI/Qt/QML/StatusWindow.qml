import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    title: "XLL Status (QML)"
    width: 340
    height: 170

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            Label { text: "Your name:" }
            TextField {
                id: nameField
                text: "World"
                Layout.fillWidth: true
                onAccepted: sendBtn.clicked()
            }
        }

        Label {
            text: bridge.statusText
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }

        Button {
            id: sendBtn
            text: "Greet Active Cell"
            Layout.alignment: Qt.AlignHCenter
            highlighted: true
            onClicked: bridge.writeToCell("Hello, " + nameField.text + "!")
        }

        Item { Layout.fillHeight: true }
    }

    // Hide instead of destroy when the user closes the window.
    onClosing: function(close) {
        close.accepted = false
        root.hide()
    }
}

