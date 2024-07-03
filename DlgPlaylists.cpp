#include "Consts.h"
#include "DlgPlaylists.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QDebug>
#include <QAbstractItemModel>
#include <QListWidget>


DlgPlaylists::DlgPlaylists()
{
	ui.setupUi(this);
	setupSignals();
}


void
DlgPlaylists::setupSignals(void)
{
	QObject::connect(ui.btnNewPlaylist, &QPushButton::pressed, this, &DlgPlaylists::newPlayList);
	QObject::connect(ui.btnDelPlaylist, &QPushButton::pressed, this, &DlgPlaylists::delPlayList);
	QObject::connect(ui.btnRenPlaylist, &QPushButton::pressed, this, &DlgPlaylists::renPlayList);
	QObject::connect(ui.btnRenPlaylist, &QPushButton::pressed, this, &DlgPlaylists::renPlayList);
	QObject::connect(ui.btnCopyPlaylist, &QPushButton::pressed, this, &DlgPlaylists::copyPlayList);

	QObject::connect(ui.btnAddItems, &QPushButton::pressed, this, &DlgPlaylists::playListAddItems);
}


void
DlgPlaylists::newPlayList(void)
{
	QString name;
	int idx;
	bool ok;

	name = QInputDialog::getText(
		this,
		tr("Новый плейлист"),
		tr("Название"),
		QLineEdit::Normal,
		"",
		&ok
		).trimmed();

	if (ok) {
		if (0 == name.length()) {
			QMessageBox::warning(this, tr("Ошибка"), tr("Имя плейлиста не должно быть пустым"), QMessageBox::Ok);
			return;
		}

		if (name.length() > MAX_PLAYLIST_NAME_LEN) {
			QMessageBox::warning(this, tr("Ошибка"), tr("Имя плейлиста слишком длинное"), QMessageBox::Ok);
			return;
		}

		idx = ui.cbPList->findText(name);
		if (idx >= 0) {
			QMessageBox::warning(this, tr("Ошибка"), tr("\"%1\"\n\nПлейлист с таким именем уже существует").arg(name), QMessageBox::Ok);
			return;
		}

		ui.cbPList->addItem(name);
		idx = ui.cbPList->findText(name);
		if (idx >= 0) {
			ui.cbPList->setCurrentIndex(idx);
		}
	}
}


void
DlgPlaylists::delPlayList(void)
{
	QString oldname;
	QMessageBox::StandardButton btn;

	oldname = ui.cbPList->currentText();

	btn = QMessageBox::question(
		this,
		tr("Удалить плейлист"),
		QString(tr("Удалить плейлист \"%1\"?")).arg(oldname),
		QMessageBox::Yes|QMessageBox::No
	);

	if (QMessageBox::Yes == btn) {
		ui.cbPList->removeItem(ui.cbPList->currentIndex());
		qDebug() << tr("Удалено %1").arg(oldname);
	}
}


void
DlgPlaylists::renPlayList(void)
{
	QString name, oldname;
	bool ok;

	oldname = ui.cbPList->itemText(ui.cbPList->currentIndex());

	name = QInputDialog::getText(
		this,
		tr("Переименовать плейлист"),
		tr("Новое название"),
		QLineEdit::Normal,
		oldname,
		&ok
	);
	
	if (ok) {
		ui.cbPList->setItemText(ui.cbPList->currentIndex(), name);
		qDebug() << tr("Переименовано %1 в %2").arg(oldname).arg(name);
	}
}


void
DlgPlaylists::copyPlayList(void)
{
	QString newName, oldname;
	bool ok;

	oldname = ui.cbPList->itemText(ui.cbPList->currentIndex());

	newName = QInputDialog::getText(
		this,
		tr("Копировать плейлист"),
		tr("Название"),
		QLineEdit::Normal,
		oldname,
		&ok
	);

	/* todo Проверка на совпадение имён! */
}


void
DlgPlaylists::playListSelChanged(int idx)
{
	QString name = ui.cbPList->currentText();
	qDebug() << tr("Выбрано %1").arg(idx);
}


void
DlgPlaylists::playListAddItems(void)
{
	QStringList addNames = QFileDialog::getOpenFileNames(this,
		tr("Добавить файл(ы)"),
		"",
		tr("Изображения (*.jpg *.jpeg *.bmp *.png);;Видео (*.avi *.mpg);;Все файлы (*.*)")
		);

	foreach(QString file, addNames) {
		ui.plContent->insertItem(0, file);
	}
}

