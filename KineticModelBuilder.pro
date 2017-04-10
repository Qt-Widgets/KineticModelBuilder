TARGET = KineticModelBuilder
TEMPLATE = app
QT += core gui widgets opengl xml concurrent svg

#release: DESTDIR = Release
release: OBJECTS_DIR = Release/.obj
release: MOC_DIR = Release/.moc
release: RCC_DIR = Release/.rcc
release: UI_DIR = Release/.ui

#debug: DESTDIR = Debug
debug: OBJECTS_DIR = Debug/.obj
debug: MOC_DIR = Debug/.moc
debug: RCC_DIR = Debug/.rcc
debug: UI_DIR = Debug/.ui

# MACX application bundle
macx: CONFIG += app_bundle

# C++11
CONFIG += c++11

# defines
debug: DEFINES += DEBUG
release: DEFINES += NDEBUG
release: DEFINES += EIGEN_NO_DEBUG

# arch
macx: DEFINES += MACX
linux: DEFINES += LINUX
win32: DEFINES += WIN32
win64: DEFINES += WIN64

# compiler flags
QMAKE_CFLAGS += -O3
QMAKE_CXXFLAGS += -march=native

# GLU
#INCLUDEPATH += /usr/X11/include
#LIBS += -L/usr/X11/lib -lGLU

# Qwt
include(/usr/local/Cellar/qwt/6.1.3_3/features/qwt.prf)
CONFIG += qwt

# Eigen
INCLUDEPATH += /usr/local/include/eigen3

# Gnu Scientific Library (GSL)
INCLUDEPATH += /usr/local/Cellar/gsl/1.16/include
LIBS += -L/usr/local/Cellar/gsl/1.16/lib -lgsl -lgslcblas

# Files
HEADERS += \
EigenLab.h \
MarkovModel.h \
MarkovModelPropertyEditor.h \
MarkovModelViewer.h \
QFont3D.h \
QObjectPropertyEditor.h \
QObjectPropertyTreeSerializer.h \
StimulusClampProtocol.h \
StimulusClampProtocolPropertyEditor.h

SOURCES += main.cpp \
MarkovModel.cpp \
MarkovModelPropertyEditor.cpp \
MarkovModelViewer.cpp \
QFont3D.cpp \
QObjectPropertyEditor.cpp \
QObjectPropertyTreeSerializer.cpp \
StimulusClampProtocol.cpp \
StimulusClampProtocolPropertyEditor.cpp
