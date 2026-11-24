#include <QApplication>
#include <QMainWindow>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QByteArray>
#include <QCloseEvent>
#include <QPushButton>
#include <QClipboard>
#include <QGuiApplication>
#include <QLabel>
#include <utils.h>

static QByteArray xorEncryptDecrypt(const QByteArray &data, const QByteArray &key)
{
    if (key.isEmpty()) return data;
    QByteArray out = data;
    for (int i = 0; i < out.size(); ++i) {
        out[i] = out[i] ^ key[i % key.size()];
    }
    return out;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    QLineEdit *passwordEdit = new QLineEdit;
    QLineEdit *passwordLength = new QLineEdit;

public:
    explicit MainWindow(const QString &filePath, const QString &masterKey, QWidget *parent = nullptr)
        : QMainWindow(parent),
          m_filePath(filePath),
          m_masterKey(masterKey.toUtf8())
    {
        setWindowTitle("Local Password Manager");

        m_table = new QTableWidget(this);
        m_table->setColumnCount(3);
        QStringList headers;
        headers << "Service" << "Username" << "Password";
        m_table->setHorizontalHeaderLabels(headers);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);
        m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

        QWidget *central = new QWidget(this);
        QVBoxLayout *layout = new QVBoxLayout(central);
        layout->addWidget(m_table);
        setCentralWidget(central);

        QToolBar *toolbar = addToolBar("Main");
        QAction *addAct = toolbar->addAction("Add");
        QAction *editAct = toolbar->addAction("Edit");
        QAction *delAct = toolbar->addAction("Delete");
        toolbar->addSeparator();
        QAction *saveAct = toolbar->addAction("Save");
        QAction *loadAct = toolbar->addAction("Load");
        
        QHBoxLayout *row = new QHBoxLayout();

        QPushButton *genBtn = new QPushButton("Generate Password");
        layout->addWidget(genBtn);

        row->addWidget(new QLabel("Length:"));
        row->addWidget(passwordLength);
        row->addWidget(passwordEdit);

        layout->addLayout(row);


        passwordLength->setText("16"); // Default length'

        connect(addAct, &QAction::triggered, this, &MainWindow::addEntry);
        connect(editAct, &QAction::triggered, this, &MainWindow::editEntry);
        connect(delAct, &QAction::triggered, this, &MainWindow::deleteEntry);
        connect(saveAct, &QAction::triggered, this, &MainWindow::saveToFile);
        connect(loadAct, &QAction::triggered, this, &MainWindow::loadFromFile);
        connect(genBtn, &QPushButton::clicked, this, [this] {

        QString pwd = generatePassword(passwordLength->text().toInt());
        
        // Copy to clipboard
        QClipboard *cb = QGuiApplication::clipboard();
        cb->setText(pwd);
        passwordEdit->setText(pwd);


        QMessageBox::information(this,
                             "Generated Password",
                             "Your new password has been copied to the clipboard:\n\n" + pwd);
});


        if (!m_filePath.isEmpty()) {
            loadFromFile();
        }
    }

protected:
    void closeEvent(QCloseEvent *event) override
    {
        auto ret = QMessageBox::question(this, "Exit",
                                         "Do you want to save before exiting?",
                                         QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
        if (ret == QMessageBox::Yes) {
            saveToFile();
        }
        event->accept();
    }

private slots:
    void addEntry()
    {
        bool ok;
        QString service = QInputDialog::getText(this, "Add Entry", "Service:", QLineEdit::Normal, "", &ok);
        if (!ok || service.isEmpty()) return;

        QString username = QInputDialog::getText(this, "Add Entry", "Username:", QLineEdit::Normal, "", &ok);
        if (!ok) return;

        QString password = QInputDialog::getText(this, "Add Entry", "Password:", QLineEdit::Normal, "", &ok);
        if (!ok) return;

        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(service));
        m_table->setItem(row, 1, new QTableWidgetItem(username));
        m_table->setItem(row, 2, new QTableWidgetItem(password));
    }

    void editEntry()
    {
        int row = m_table->currentRow();
        if (row < 0) {
            QMessageBox::warning(this, "Edit Entry", "No row selected.");
            return;
        }

        bool ok;
        QString service = QInputDialog::getText(this, "Edit Entry", "Service:",
                                                QLineEdit::Normal,
                                                m_table->item(row, 0)->text(), &ok);
        if (!ok || service.isEmpty()) return;

        QString username = QInputDialog::getText(this, "Edit Entry", "Username:",
                                                 QLineEdit::Normal,
                                                 m_table->item(row, 1)->text(), &ok);
        if (!ok) return;

        QString password = QInputDialog::getText(this, "Edit Entry", "Password:",
                                                 QLineEdit::Normal,
                                                 m_table->item(row, 2)->text(), &ok);
        if (!ok) return;

        m_table->item(row, 0)->setText(service);
        m_table->item(row, 1)->setText(username);
        m_table->item(row, 2)->setText(password);
    }

    void deleteEntry()
    {
        int row = m_table->currentRow();
        if (row < 0) {
            QMessageBox::warning(this, "Delete Entry", "No row selected.");
            return;
        }
        m_table->removeRow(row);
    }

    void saveToFile()
    {
        QString path = m_filePath;
        if (path.isEmpty()) {
            path = QFileDialog::getSaveFileName(this, "Save Passwords", "", "Password DB (*.pwdb);;All Files (*)");
            if (path.isEmpty()) return;
            m_filePath = path;
        }

        QByteArray plain;
        for (int row = 0; row < m_table->rowCount(); ++row) {
            QString service = m_table->item(row, 0)->text();
            QString username = m_table->item(row, 1)->text();
            QString password = m_table->item(row, 2)->text();
            QString line = service + "\t" + username + "\t" + password + "\n";
            plain.append(line.toUtf8());
        }

        QByteArray encrypted = xorEncryptDecrypt(plain, m_masterKey);

        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "Error", "Failed to open file for writing.");
            return;
        }
        f.write(encrypted);
        f.close();
        QMessageBox::information(this, "Saved", "Passwords saved successfully.");
    }

    void loadFromFile()
    {
        QString path = m_filePath;
        if (path.isEmpty()) {
            path = QFileDialog::getOpenFileName(this, "Open Passwords", "", "Password DB (*.pwdb);;All Files (*)");
            if (path.isEmpty()) return;
            m_filePath = path;
        }

        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Error", "Failed to open file for reading.");
            return;
        }
        QByteArray encrypted = f.readAll();
        f.close();

        QByteArray plain = xorEncryptDecrypt(encrypted, m_masterKey);
        QList<QByteArray> lines = plain.split('\n');

        m_table->setRowCount(0);
        for (const QByteArray &line : lines) {
            if (line.trimmed().isEmpty()) continue;
            QList<QByteArray> parts = line.split('\t');
            if (parts.size() < 3) continue;
            int row = m_table->rowCount();
            m_table->insertRow(row);
            m_table->setItem(row, 0, new QTableWidgetItem(QString::fromUtf8(parts[0])));
            m_table->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8(parts[1])));
            m_table->setItem(row, 2, new QTableWidgetItem(QString::fromUtf8(parts[2])));
        }
        QMessageBox::information(this, "Loaded", "Passwords loaded successfully.");
    }

private:
    QTableWidget *m_table;
    QString m_filePath;
    QByteArray m_masterKey;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Ask for master password
    bool ok;
    QString master = QInputDialog::getText(nullptr,
                                           "Master Password",
                                           "Enter master password:",
                                           QLineEdit::Password,
                                           "",
                                           &ok);
    if (!ok || master.isEmpty()) {
        return 0;
    }

    QString filePath;
    if (argc > 1) {
        filePath = QString::fromLocal8Bit(argv[1]);
    }

    MainWindow w(filePath, master);
    w.resize(700, 400);
    w.show();

    return app.exec();
}

#include "main.moc"
