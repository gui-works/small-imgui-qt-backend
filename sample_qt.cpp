// sample_qt.cpp - public domain
// authored from 2012-2013 by Adrien Herubel 


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <cmath>
#include <string>
#include <iostream>

#include <QDebug>
#include <QDirIterator>
#include <QMouseEvent>
#include <QElapsedTimer>
#include <QWaitCondition>
#include <QPainter>
#include <QWindow>
#include <QOpenGLContext>
#include <QOpenGLPaintDevice>
#include <QOpenGLFunctions>
#include <QOpenGLDebugLogger>
#include <QGuiApplication>

#include "imgui.h"
#include "imguiRenderGLES.h"

class SleepSimulator {
	QMutex localMutex;
	QWaitCondition sleepSimulator;
public:
	SleepSimulator() { localMutex.lock(); }
	void sleep(unsigned long sleepMS) { sleepSimulator.wait(&localMutex, sleepMS); }
	void CancelSleep() { sleepSimulator.wakeAll(); }
};

double qtGetTime() {
	static QElapsedTimer timer;
	if (!timer.isValid())
	timer.start();
	return timer.elapsed() / 1000.;
}

void qtDelay(long ms) {
	SleepSimulator s;
	s.sleep(ms);
}

static int const CAPTURED_FRAMES_NUM   = 30;    // 30 captures
static float const AVG_TIME            = 0.5;   // 500 millisecondes

class FPSComputer
{
    float history[CAPTURED_FRAMES_NUM];
    int indx; long int total;
    float timestamp, average, last;
    const float step;

	QElapsedTimer time_, timer_;

public:
    FPSComputer(void)
    : indx(0), total(0)
    , timestamp(0)
    , step(AVG_TIME / CAPTURED_FRAMES_NUM)
    , average(0), last(0)
    {
        time_.start(); timer_.start();
        for (int i = 0; i < CAPTURED_FRAMES_NUM; ++i) history[i] = 0;
    }

    float ComputeFPS()
    {
        const float delta_time = timer_.restart()/1000.;
        const float total_time = time_.elapsed()/1000.;

        float fpsFrame = 1. / delta_time;
        if (total_time - last > step)
        {
            last = total_time;
            ++indx %= CAPTURED_FRAMES_NUM;
            average -= history[indx];
            history[indx] = fpsFrame / CAPTURED_FRAMES_NUM;
            average += history[indx];
            ++total;
        }
        return average;
    }

    float ComputeFPS(float delta_time, float total_time)
    {
        float fpsFrame = 1. / delta_time;
        if (total_time - last > step)
        {
            last = total_time;
            ++indx %= CAPTURED_FRAMES_NUM;
            average -= history[indx];
            history[indx] = fpsFrame / CAPTURED_FRAMES_NUM;
            average += history[indx];
            ++total;
        }
        return average;
    }

    float GetLastAverage(void) { return average; }
    long int GetTotalFrames(void) { return total; }
};

static FPSComputer fps;

// Qt window

class Window : public QWindow, protected QOpenGLFunctions
{
	Q_OBJECT
private:
	bool m_done, m_update_pending, m_resize_pending, m_auto_refresh;
	QOpenGLContext *m_context;
	QOpenGLPaintDevice *m_device;
public:
	QPoint cursorPos;
    bool leftButton, rightButton, middleButton;
    int currentglfwscroll;
public:
	Window(QWindow *parent = 0) : QWindow(parent)
	, m_update_pending(false)
	, m_resize_pending(false)
	, m_auto_refresh(true)
	, m_context(0)
	, m_device(0)
	, m_done(false) {
		setSurfaceType(QWindow::OpenGLSurface);
	}
	~Window() {
        imguiRenderGLDestroy();
        delete m_device;
    }
	void setAutoRefresh(bool a) { m_auto_refresh = a; }

	void render(QPainter *painter) {
		Q_UNUSED(painter);

        glClearColor(0.8f, 0.8f, 0.8f, 1.f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        // imgui states
        bool checked1 = false;
        bool checked2 = false;
        bool checked3 = true;
        bool checked4 = false;
        float value1 = 50.f;
        float value2 = 30.f;
        int scrollarea1 = 0;
        int scrollarea2 = 0;

        // glfw scrolling
        int glfwscroll = 0;
        glViewport(0, 0, width()*devicePixelRatio(), height()*devicePixelRatio());

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Mouse states
        unsigned char mousebutton = 0;
        int mscroll = 0;
        if (currentglfwscroll < glfwscroll)
            mscroll = 2;
         if (currentglfwscroll > glfwscroll)
            mscroll = -2;
        glfwscroll = currentglfwscroll;
        int mousex = cursorPos.x(), mousey = cursorPos.y();
        mousey = height() - mousey;
        int toggle = 0;
        if( leftButton )
            mousebutton |= IMGUI_MBUT_LEFT;

        // Draw UI
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float projection[16] = { 2.f/width(), 0.f, 0.f,  0.f,
                                 0.f, 2.f/height(),  0.f,  0.f,
                                 0.f,  0.f, -2.f, 0.f,
                                 -1.f, -1.f,  -1.f,  1.f };
        glLoadMatrixf(projection);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glUseProgram(0);


        imguiBeginFrame(mousex, mousey, mousebutton, mscroll);

        imguiBeginScrollArea("Scroll area", 10, 10, width() / 5, height() - 20, &scrollarea1);
        imguiSeparatorLine();
        imguiSeparator();

        imguiButton("Button");
        imguiButton("Disabled button", false);
        imguiItem("Item");
        imguiItem("Disabled item", false);
        toggle = imguiCheck("Checkbox", checked1);
        if (toggle)
            checked1 = !checked1;
        toggle = imguiCheck("Disabled checkbox", checked2, false);
        if (toggle)
            checked2 = !checked2;
        toggle = imguiCollapse("Collapse", "subtext", checked3);
        if (checked3)
        {
            imguiIndent();
            imguiLabel("Collapsible element");
            imguiUnindent();
        }
        if (toggle)
            checked3 = !checked3;
        toggle = imguiCollapse("Disabled collapse", "subtext", checked4, false);
        if (toggle)
            checked4 = !checked4;
        imguiLabel("Label");
        imguiValue("Value");
        imguiSlider("Slider", &value1, 0.f, 100.f, 1.f);
        imguiSlider("Disabled slider", &value2, 0.f, 100.f, 1.f, false);
        imguiIndent();
        imguiLabel("Indented");
        imguiUnindent();
        imguiLabel("Unindented");

        imguiEndScrollArea();

        imguiBeginScrollArea("Scroll area", 20 + width() / 5, 500, width() / 5, height() - 510, &scrollarea2);
        imguiSeparatorLine();
        imguiSeparator();
        for (int i = 0; i < 100; ++i)
            imguiLabel("A wall of text");

        imguiEndScrollArea();
        imguiEndFrame();

        imguiDrawText(30 + width() / 5 * 2, height() - 20, IMGUI_ALIGN_LEFT, "Free text",  imguiRGBA(32,192, 32,192));
        imguiDrawText(30 + width() / 5 * 2 + 100, height() - 40, IMGUI_ALIGN_RIGHT, "Free text",  imguiRGBA(32, 32, 192, 192));
        imguiDrawText(30 + width() / 5 * 2 + 50, height() - 60, IMGUI_ALIGN_CENTER, "Free text",  imguiRGBA(192, 32, 32,192));

        imguiDrawLine(30 + width() / 5 * 2, height() - 80, 30 + width() / 5 * 2 + 100, height() - 60, 1.f, imguiRGBA(32,192, 32,192));
        imguiDrawLine(30 + width() / 5 * 2, height() - 100, 30 + width() / 5 * 2 + 100, height() - 80, 2.f, imguiRGBA(32, 32, 192, 192));
        imguiDrawLine(30 + width() / 5 * 2, height() - 120, 30 + width() / 5 * 2 + 100, height() - 100, 3.f, imguiRGBA(192, 32, 32,192));

        imguiDrawRoundedRect(30 + width() / 5 * 2, height() - 240, 100, 100, 5.f, imguiRGBA(32,192, 32,192));
        imguiDrawRoundedRect(30 + width() / 5 * 2, height() - 350, 100, 100, 10.f, imguiRGBA(32, 32, 192, 192));
        imguiDrawRoundedRect(30 + width() / 5 * 2, height() - 470, 100, 100, 20.f, imguiRGBA(192, 32, 32,192));

        imguiDrawRect(30 + width() / 5 * 2, height() - 590, 100, 100, imguiRGBA(32, 192, 32, 192));
        imguiDrawRect(30 + width() / 5 * 2, height() - 710, 100, 100, imguiRGBA(32, 32, 192, 192));
        imguiDrawRect(30 + width() / 5 * 2, height() - 830, 100, 100, imguiRGBA(192, 32, 32,192));

        imguiRenderGLDraw(width(), height());
    }
	void initialize() {
		QOpenGLDebugLogger *m_logger = new QOpenGLDebugLogger(this);
		connect( m_logger, &QOpenGLDebugLogger::messageLogged, []( QOpenGLDebugMessage message ){
			qWarning() << "[OpenGL]" << message;
		});
		if ( m_logger->initialize() ) {
			m_logger->startLogging( QOpenGLDebugLogger::SynchronousLogging );
			m_logger->enableMessages();
			qDebug() << "QOpenGLDebugLogger initialized.";
		}
		qDebug() << "OpenGL infos with gl functions:";
		qDebug() << "-------------------------------";
		qDebug() << " Renderer:" << (const char*)glGetString(GL_RENDERER);
		qDebug() << " Vendor:" << (const char*)glGetString(GL_VENDOR);
		qDebug() << " OpenGL Version:" << (const char*)glGetString(GL_VERSION);
		qDebug() << " GLSL Version:" << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
		setTitle(QString("Qt %1 - %2 (%3)").arg(QT_VERSION_STR).arg((const char*)glGetString(GL_VERSION)).arg((const char*)glGetString(GL_RENDERER)));
	}
	void update() { renderLater(); }
	void render() {
		if (!m_device) m_device = new QOpenGLPaintDevice;
		m_device->setSize(size());
		QPainter painter(m_device);
		render(&painter);
	}
	void mousePressEvent(QMouseEvent *event) {
		cursorPos = QPoint(event->x(), event->y());
		Qt::KeyboardModifiers modifiers = event->modifiers();
		leftButton = event->buttons() & Qt::LeftButton;
		rightButton = event->buttons() & Qt::RightButton;
		middleButton = event->buttons() & Qt::MiddleButton;
	}
	void mouseReleaseEvent(QMouseEvent *event) {
		cursorPos = QPoint(event->x(), event->y());
		Qt::KeyboardModifiers modifiers = event->modifiers();
		leftButton = event->button() == Qt::LeftButton;
		rightButton = event->button() == Qt::RightButton;
		middleButton = event->button() == Qt::MiddleButton;
	}
	void mouseMoveEvent(QMouseEvent *event) {
		cursorPos = QPoint(event->x(), event->y());
	}
	void keyPressEvent(QKeyEvent* event) {
		switch(event->key()) {
			case Qt::Key_Escape: close(); break;
			default: event->ignore();
			break;
		}
	}
	void quit() { m_done = true; }
	bool done() const { return m_done; }
protected:
	void closeEvent(QCloseEvent *event) { quit(); }
	bool event(QEvent *event) {
		switch (event->type()) {
			case QEvent::UpdateRequest:
				m_update_pending = false;
				renderNow();
				return true;
			default:
				return QWindow::event(event);
		}
	}
	void exposeEvent(QExposeEvent *event) {
		Q_UNUSED(event);
		if (isExposed()) renderNow();
	}
	void resizeEvent(QResizeEvent *event)
	{
		renderLater();
	}
public slots:
	void renderLater() {
		if (!m_update_pending) {
			m_update_pending = true;
			QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
		}
	}
	void renderNow() {
		if (!isExposed()) return;
		bool needsInitialize = false;
		if (!m_context) {
			m_context = new QOpenGLContext(this);
			m_context->setFormat(requestedFormat());
			m_context->create();
			needsInitialize = true;
		}
		m_context->makeCurrent(this);
		if (needsInitialize) {
			initializeOpenGLFunctions();
			initialize();
		}
		render();
		m_context->swapBuffers(this);
		if (m_auto_refresh) renderLater();
	}
};


int main( int argc, char **argv )
{
    int width = 1024, height=768;

    QGuiApplication app(argc, argv);

    // Init UI
    if (!imguiRenderGLInit("ttf/Consola.ttf"))
    {
        fprintf(stderr, "Could not init GUI renderer.\n");
        exit(EXIT_FAILURE);
    }

    Window w;
    w.resize(width, height);

    return app.exec();
}

#include "sample_qt.moc"
