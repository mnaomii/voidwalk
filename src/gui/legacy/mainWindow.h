#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_mainWindow.h"

class mainWindow : public QMainWindow
{
    Q_OBJECT

public:
    mainWindow(QWidget *parent = nullptr);
    ~mainWindow();

private:
    Ui::mainWindowClass ui;
};

