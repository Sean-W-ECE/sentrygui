/********************************************************************************
** Form generated from reading UI file 'sentrygui.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SENTRYGUI_H
#define UI_SENTRYGUI_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_sentryguiClass
{
public:
    QWidget *layoutWidget;
    QHBoxLayout *horizontalLayout;
    QLabel *camVLabel;
    QLineEdit *camVDisplay;
    QLabel *turretHLabel;
    QLineEdit *turretHDisplay;
    QLabel *camHLabel;
    QLineEdit *camHDisplay;
    QLabel *rangeLabel;
    QLineEdit *rangeDisplay;
    QWidget *layoutWidget1;
    QHBoxLayout *horizontalLayout_2;
    QLabel *statusLabel;
    QLineEdit *statusDisplay;
    QLabel *modeLabel;
    QLineEdit *modeDisplay;
    QGraphicsView *camView;
    QTextEdit *console;

    void setupUi(QWidget *sentryguiClass)
    {
        if (sentryguiClass->objectName().isEmpty())
            sentryguiClass->setObjectName(QStringLiteral("sentryguiClass"));
        sentryguiClass->resize(1141, 618);
        layoutWidget = new QWidget(sentryguiClass);
        layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
        layoutWidget->setGeometry(QRect(210, 570, 721, 35));
        horizontalLayout = new QHBoxLayout(layoutWidget);
        horizontalLayout->setSpacing(6);
        horizontalLayout->setContentsMargins(11, 11, 11, 11);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        camVLabel = new QLabel(layoutWidget);
        camVLabel->setObjectName(QStringLiteral("camVLabel"));
        QFont font;
        font.setPointSize(11);
        camVLabel->setFont(font);
        camVLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout->addWidget(camVLabel);

        camVDisplay = new QLineEdit(layoutWidget);
        camVDisplay->setObjectName(QStringLiteral("camVDisplay"));
        camVDisplay->setEnabled(true);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(camVDisplay->sizePolicy().hasHeightForWidth());
        camVDisplay->setSizePolicy(sizePolicy);
        camVDisplay->setMaximumSize(QSize(100, 16777215));
        camVDisplay->setFont(font);
        camVDisplay->setMouseTracking(false);
        camVDisplay->setAcceptDrops(false);
        camVDisplay->setReadOnly(true);

        horizontalLayout->addWidget(camVDisplay);

        turretHLabel = new QLabel(layoutWidget);
        turretHLabel->setObjectName(QStringLiteral("turretHLabel"));
        turretHLabel->setFont(font);
        turretHLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout->addWidget(turretHLabel);

        turretHDisplay = new QLineEdit(layoutWidget);
        turretHDisplay->setObjectName(QStringLiteral("turretHDisplay"));
        turretHDisplay->setEnabled(true);
        sizePolicy.setHeightForWidth(turretHDisplay->sizePolicy().hasHeightForWidth());
        turretHDisplay->setSizePolicy(sizePolicy);
        turretHDisplay->setMaximumSize(QSize(100, 16777215));
        turretHDisplay->setFont(font);
        turretHDisplay->setMouseTracking(false);
        turretHDisplay->setAcceptDrops(false);
        turretHDisplay->setReadOnly(true);

        horizontalLayout->addWidget(turretHDisplay);

        camHLabel = new QLabel(layoutWidget);
        camHLabel->setObjectName(QStringLiteral("camHLabel"));
        camHLabel->setFont(font);
        camHLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout->addWidget(camHLabel);

        camHDisplay = new QLineEdit(layoutWidget);
        camHDisplay->setObjectName(QStringLiteral("camHDisplay"));
        camHDisplay->setEnabled(true);
        sizePolicy.setHeightForWidth(camHDisplay->sizePolicy().hasHeightForWidth());
        camHDisplay->setSizePolicy(sizePolicy);
        camHDisplay->setMaximumSize(QSize(100, 16777215));
        camHDisplay->setFont(font);
        camHDisplay->setMouseTracking(false);
        camHDisplay->setAcceptDrops(false);
        camHDisplay->setReadOnly(true);

        horizontalLayout->addWidget(camHDisplay);

        rangeLabel = new QLabel(layoutWidget);
        rangeLabel->setObjectName(QStringLiteral("rangeLabel"));
        rangeLabel->setFont(font);
        rangeLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout->addWidget(rangeLabel);

        rangeDisplay = new QLineEdit(layoutWidget);
        rangeDisplay->setObjectName(QStringLiteral("rangeDisplay"));
        rangeDisplay->setEnabled(true);
        sizePolicy.setHeightForWidth(rangeDisplay->sizePolicy().hasHeightForWidth());
        rangeDisplay->setSizePolicy(sizePolicy);
        rangeDisplay->setMaximumSize(QSize(100, 16777215));
        rangeDisplay->setFont(font);
        rangeDisplay->setMouseTracking(false);
        rangeDisplay->setAcceptDrops(false);
        rangeDisplay->setReadOnly(true);

        horizontalLayout->addWidget(rangeDisplay);

        layoutWidget1 = new QWidget(sentryguiClass);
        layoutWidget1->setObjectName(QStringLiteral("layoutWidget1"));
        layoutWidget1->setGeometry(QRect(370, 530, 371, 35));
        horizontalLayout_2 = new QHBoxLayout(layoutWidget1);
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        statusLabel = new QLabel(layoutWidget1);
        statusLabel->setObjectName(QStringLiteral("statusLabel"));
        statusLabel->setFont(font);
        statusLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_2->addWidget(statusLabel);

        statusDisplay = new QLineEdit(layoutWidget1);
        statusDisplay->setObjectName(QStringLiteral("statusDisplay"));
        statusDisplay->setEnabled(true);
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(statusDisplay->sizePolicy().hasHeightForWidth());
        statusDisplay->setSizePolicy(sizePolicy1);
        statusDisplay->setMaximumSize(QSize(200, 16777215));
        statusDisplay->setFont(font);
        statusDisplay->setMouseTracking(false);
        statusDisplay->setAcceptDrops(false);
        statusDisplay->setReadOnly(true);

        horizontalLayout_2->addWidget(statusDisplay);

        modeLabel = new QLabel(layoutWidget1);
        modeLabel->setObjectName(QStringLiteral("modeLabel"));
        QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(modeLabel->sizePolicy().hasHeightForWidth());
        modeLabel->setSizePolicy(sizePolicy2);
        modeLabel->setMinimumSize(QSize(50, 0));
        modeLabel->setFont(font);
        modeLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_2->addWidget(modeLabel);

        modeDisplay = new QLineEdit(layoutWidget1);
        modeDisplay->setObjectName(QStringLiteral("modeDisplay"));
        modeDisplay->setEnabled(true);
        sizePolicy.setHeightForWidth(modeDisplay->sizePolicy().hasHeightForWidth());
        modeDisplay->setSizePolicy(sizePolicy);
        modeDisplay->setMaximumSize(QSize(100, 16777215));
        modeDisplay->setFont(font);
        modeDisplay->setMouseTracking(false);
        modeDisplay->setAcceptDrops(false);
        modeDisplay->setReadOnly(true);

        horizontalLayout_2->addWidget(modeDisplay);

        camView = new QGraphicsView(sentryguiClass);
        camView->setObjectName(QStringLiteral("camView"));
        camView->setGeometry(QRect(20, 20, 661, 501));
        console = new QTextEdit(sentryguiClass);
        console->setObjectName(QStringLiteral("console"));
        console->setGeometry(QRect(710, 20, 411, 501));

        retranslateUi(sentryguiClass);

        QMetaObject::connectSlotsByName(sentryguiClass);
    } // setupUi

    void retranslateUi(QWidget *sentryguiClass)
    {
        sentryguiClass->setWindowTitle(QApplication::translate("sentryguiClass", "sentrygui", 0));
        camVLabel->setText(QApplication::translate("sentryguiClass", "Vertical Pitch", 0));
        turretHLabel->setText(QApplication::translate("sentryguiClass", "Turret Pan", 0));
        camHLabel->setText(QApplication::translate("sentryguiClass", "Camera Pan", 0));
        rangeLabel->setText(QApplication::translate("sentryguiClass", "Range", 0));
        statusLabel->setText(QApplication::translate("sentryguiClass", "Status:", 0));
        statusDisplay->setText(QApplication::translate("sentryguiClass", "Initializing", 0));
        modeLabel->setText(QApplication::translate("sentryguiClass", "Mode:", 0));
        modeDisplay->setText(QApplication::translate("sentryguiClass", "Manual", 0));
    } // retranslateUi

};

namespace Ui {
    class sentryguiClass: public Ui_sentryguiClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SENTRYGUI_H
