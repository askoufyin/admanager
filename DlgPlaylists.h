#pragma once
#include <QDialog>
#include <ui_DlgPlaylists.h>


class DlgPlaylists :
    public QDialog
{
    Q_OBJECT
public:
    DlgPlaylists();
protected:
    void setupSignals();
    void newPlayList(void);
    void delPlayList(void);
    void renPlayList(void);
    void copyPlayList(void);
    void playListSelChanged(int);
    void playListAddItems(void);
protected:
    Ui::DlgPlaylists ui;
};

