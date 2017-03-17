//decent_wallet_ui_gui_purchasedtab
/*
 *	File: decent_wallet_ui_gui_purchasedtab.hpp
 *
 *	Created on: 11 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#ifndef DECENT_WALLET_UI_GUI_PURCHASEDTAB_HPP
#define DECENT_WALLET_UI_GUI_PURCHASEDTAB_HPP

#include <QWidget>
#include <string>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <QPushButton>
#include "qt_commonheader.hpp"
#include "gui_wallet_tabcontentmanager.hpp"

class P_TableWidget : public QTableWidget
{
    Q_OBJECT
public:
    P_TableWidget(int row, int col) : QTableWidget(row,col)
    {
        this->setMouseTracking(true);
    }
    
    virtual void mouseMoveEvent(QMouseEvent * event);
public:
signals:
    void mouseMoveEventDid();
};


class PButton : public QLabel
{
    Q_OBJECT
public:
    PButton() : QLabel() {this->setMouseTracking(true);}
    PButton(QString str) : QLabel(str){this->setMouseTracking(true);}
public:
signals:
    void mouseWasMoved();
public:
    virtual void mouseMoveEvent(QMouseEvent * event)
    {
        emit mouseWasMoved();
        QLabel::mouseMoveEvent(event);
    }
};

class EButton : public QPushButton
{
    Q_OBJECT
public:
    EButton(){this->setMouseTracking(true);}
public:
signals:
    void mouseMoved();
public:
    virtual void mouseMoveEvent(QMouseEvent * event)
    {
        emit mouseMoved();
    }
};

namespace gui_wallet {

namespace DCF_PURCHASE {
    enum DIG_CONT_FIELDS { TIME, SYNOPSIS, RATING,SIZE, PRICE, PURCHASED, NUM_OF_DIG_CONT_FIELDS };
}

class PurchasedTab : public TabContentManager
{

    friend class CentralWigdet;
    friend class Mainwindow_gui_wallet;
    Q_OBJECT
public:
    PurchasedTab();
    virtual ~PurchasedTab();

    void ShowDigitalContentsGUI(std::vector<SDigitalContent>& contents);

public:
    virtual void content_activated() {
        m_contentUpdateTimer.start();
    }
    virtual void content_deactivated() {
        m_contentUpdateTimer.stop();
    }
    
public:
signals:
    void ShowDetailsOnDigContentSig(SDigitalContent dig_cont);

protected:
    void PrepareTableWidgetHeaderGUI();
    void DigContCallback(_NEEDED_ARGS2_);
    virtual void resizeEvent ( QResizeEvent * a_event );
    void ArrangeSize();

public slots:
    void doRowColor();
    void onTextChanged(const QString& text);
    void updateContents();
    void maybeUpdateContent();
    void extractPackage();

protected:
    QVBoxLayout     m_main_layout;
    P_TableWidget*    m_pTableWidget;
    QLineEdit       m_filterLineEditer;
    
    
private:
    QTimer  m_contentUpdateTimer;
    bool m_doUpdate = true;
    std::string last_contents;
};

}


#include "decent_wallet_ui_gui_common.tos"

#endif // DECENT_WALLET_UI_GUI_PURCHASEDTAB_HPP