import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    title: "QML inside an XLL"
    width: 360
    height: 150

    property string inputText: ""
    property bool accepted: false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            Label { text: "Enter your name:" }
            TextField {
                id: nameField
                Layout.fillWidth: true
                Layout.minimumWidth: 200
                onAccepted: okBtn.clicked()
            }
        }

        Item { Layout.preferredHeight: 10 }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8

            Button {
                id: okBtn
                text: "OK"
                onClicked: {
                    root.inputText = nameField.text
                    root.accepted = true
                    root.close()
                }
            }

            Button {
                text: "Cancel"
                onClicked: {
                    root.accepted = false
                    root.close()
                }
            }
        }

        Item { Layout.fillHeight: true }
    }

    // Title-bar X button → treat as Cancel.
    onClosing: function(close) {
        if (!root.accepted) {
            root.inputText = ""
        }
    }
}

