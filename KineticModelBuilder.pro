TARGET = KineticModelBuilder
TEMPLATE = app
QT += core widgets opengl xml concurrent svg

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

# Qwt
include(/usr/local/Cellar/qwt/6.1.3_3/features/qwt.prf)
CONFIG += qwt

# Eigen
INCLUDEPATH += /usr/local/include/eigen3

# Gnu Scientific Library (GSL)
INCLUDEPATH += /usr/local/Cellar/gsl/1.16/include
LIBS += -L/usr/local/Cellar/gsl/1.16/lib -lgsl -lgslcblas

# Files
HEADERS += EigenLab.h
#HEADERS += exprtk.hpp
HEADERS += MarkovModel.h
HEADERS += QObjectPropertyEditor.h
HEADERS += QObjectPropertyTreeSerializer.h

SOURCES += main.cpp
SOURCES += MarkovModel.cpp
SOURCES += QObjectPropertyEditor.cpp
SOURCES += QObjectPropertyTreeSerializer.cpp
