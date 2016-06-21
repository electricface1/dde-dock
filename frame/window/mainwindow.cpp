#include "mainwindow.h"
#include "panel/mainpanel.h"

#include <QDebug>
#include <QResizeEvent>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
      m_mainPanel(new MainPanel(this)),

      m_settings(new DockSettings(this)),
      m_displayInter(new DBusDisplay(this)),

      m_xcbMisc(XcbMisc::instance()),

      m_positionUpdateTimer(new QTimer(this))
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);

    m_xcbMisc->set_window_type(winId(), XcbMisc::Dock);

    initComponents();
    initConnections();
}

MainWindow::~MainWindow()
{
    delete m_xcbMisc;
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);

    m_mainPanel->setFixedSize(e->size());
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    e->ignore();

    if (e->button() == Qt::RightButton)
        m_settings->showDockSettingsMenu();
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
    {
#ifdef QT_DEBUG
    case Qt::Key_Escape:        qApp->quit();       break;
#endif
    default:;
    }
}

void MainWindow::initComponents()
{
    m_positionUpdateTimer->setSingleShot(true);
    m_positionUpdateTimer->setInterval(200);
    m_positionUpdateTimer->start();
}

void MainWindow::initConnections()
{
    connect(m_displayInter, &DBusDisplay::PrimaryRectChanged, m_positionUpdateTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(m_settings, &DockSettings::dataChanged, m_positionUpdateTimer, static_cast<void (QTimer::*)()>(&QTimer::start));

    connect(m_positionUpdateTimer, &QTimer::timeout, this, &MainWindow::updatePosition);
}

void MainWindow::updatePosition()
{
    Q_ASSERT(sender() == m_positionUpdateTimer);

    clearStrutPartial();
    setFixedSize(m_settings->windowSize());
    m_mainPanel->updateDockPosition(m_settings->position());

    const QRect primaryRect = m_displayInter->primaryRect();
    switch (m_settings->position())
    {
    case Top:
    case Left:
        move(primaryRect.topLeft());                    break;
    case Right:
        move(primaryRect.right() - width(), 0);         break;
    case Bottom:
        move(0, primaryRect.bottom() - height() + 1);   break;
    default:
        Q_ASSERT(false);
    }

    setStrutPartial();
}

void MainWindow::clearStrutPartial()
{
    m_xcbMisc->clear_strut_partial(winId());
}

void MainWindow::setStrutPartial()
{
    // first, clear old strut partial
    clearStrutPartial();

    const Position side = m_settings->position();
    const int maxScreenHeight = m_displayInter->screenHeight();

    XcbMisc::Orientation orientation;
    uint strut;
    uint strutStart;
    uint strutEnd;

    const QPoint p = pos();
    const QRect r = rect();
    switch (side)
    {
    case Position::Top:
        orientation = XcbMisc::OrientationTop;
        strut = r.bottom();
        strutStart = r.left();
        strutEnd = r.right();
        break;
    case Position::Bottom:
        orientation = XcbMisc::OrientationBottom;
        strut = maxScreenHeight - p.y();
        strutStart = r.left();
        strutEnd = r.right();
        break;
    case Position::Left:
        orientation = XcbMisc::OrientationLeft;
        strut = r.width();
        strutStart = r.top();
        strutEnd = r.bottom();
        break;
    case Position::Right:
        orientation = XcbMisc::OrientationRight;
        strut = r.width();
        strutStart = r.top();
        strutEnd = r.bottom();
        break;
    default:
        Q_ASSERT(false);
    }

    m_xcbMisc->set_strut_partial(winId(), orientation, strut, strutStart, strutEnd);
}