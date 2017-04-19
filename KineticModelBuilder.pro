# ------------------------------------------------
# User defined paths.
QWT_PRF = /usr/local/Cellar/qwt/6.1.3_3/features/qwt.prf
EIGEN_HEADERS = /usr/local/include/eigen3
GSL_HEADERS = /usr/local/Cellar/gsl/1.16/include
GSL_LIBS = /usr/local/Cellar/gsl/1.16/lib
EIGENLAB_HEADERS = $$(HOME)/projects/EigenLab
# ------------------------------------------------

TARGET = KineticModelBuilder
TEMPLATE = app
QT += core gui widgets opengl xml concurrent svg

# MACX application bundle
macx: CONFIG += app_bundle

# C++11
CONFIG += c++11

# compiler flags
QMAKE_CFLAGS += -O3
QMAKE_CXXFLAGS += -march=native

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

# defines
debug: DEFINES += DEBUG
release: DEFINES += NDEBUG
release: DEFINES += EIGEN_NO_DEBUG

# arch
macx: DEFINES += MACX
linux: DEFINES += LINUX
win32: DEFINES += WIN32
win64: DEFINES += WIN64

# Qwt
include($$QWT_PRF)
CONFIG += qwt

# Eigen
INCLUDEPATH += $$EIGEN_HEADERS

# Gnu Scientific Library (GSL)
INCLUDEPATH += $$GSL_HEADERS
LIBS += -L$${GSL_LIBS} -lgsl -lgslcblas

# EigenLab
INCLUDEPATH += $$EIGENLAB_HEADERS

# Files
SOURCES += main.cpp

HEADERS += MarkovModel.h
SOURCES += MarkovModel.cpp

HEADERS += MarkovModelPropertyEditor.h
SOURCES += MarkovModelPropertyEditor.cpp

HEADERS += MarkovModelViewer.h
SOURCES += MarkovModelViewer.cpp

HEADERS += MarkovModelWindow.h
SOURCES += MarkovModelWindow.cpp

HEADERS += Project.h
SOURCES += Project.cpp

HEADERS += QFont3D.h
SOURCES += QFont3D.cpp

HEADERS += QMultipleIndexSpinBox.h
SOURCES += QMultipleIndexSpinBox.cpp

HEADERS += QObjectPropertyEditor.h
SOURCES += QObjectPropertyEditor.cpp

HEADERS += QObjectPropertyTreeSerializer.h
SOURCES += QObjectPropertyTreeSerializer.cpp

HEADERS += StimulusClampProtocol.h
SOURCES += StimulusClampProtocol.cpp

HEADERS += StimulusClampProtocolPlot.h
SOURCES += StimulusClampProtocolPlot.cpp

HEADERS += StimulusClampProtocolPropertyEditor.h
SOURCES += StimulusClampProtocolPropertyEditor.cpp

HEADERS += StimulusClampProtocolWindow.h
SOURCES += StimulusClampProtocolWindow.cpp
