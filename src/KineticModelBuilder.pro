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
# !!! On MacOSX you have to manually copy the qwt.framework to Libary/Frameworks folder
#     to avoid getting the "dyld: Library not loaded" error. No idea why this is necessary.
include($$(HOME)/qwt-6.1.2-qt-5.5.0/features/qwt.prf)
CONFIG += qwt

# Eigen
INCLUDEPATH += /usr/local/include/eigen3

# Gnu Scientific Library (GSL)
INCLUDEPATH += /usr/local/Cellar/gsl/1.16/include
LIBS += -L/usr/local/Cellar/gsl/1.16/lib -lgsl -lgslcblas

# source files
SOURCES += main.cpp

#HEADERS += exprtk.hpp

HEADERS += EigenLab.h

#HEADERS += EigenHMM.h
#SOURCES += EigenHMM.cpp

#HEADERS += Object.h
#SOURCES += Object.cpp

#HEADERS += ObjectListPropertyTableView.h
#SOURCES += ObjectListPropertyTableView.cpp

#HEADERS += ObjectPropertyTableView.h
#SOURCES += ObjectPropertyTableView.cpp

HEADERS += ObjectPropertyTreeSerializer.h
SOURCES += ObjectPropertyTreeSerializer.cpp

#HEADERS += MarkovModel.h
#SOURCES += MarkovModel.cpp

HEADERS += MarkovModel.h
SOURCES += MarkovModel.cpp

#HEADERS += MarkovModelViewer.h
#SOURCES += MarkovModelViewer.cpp

#HEADERS += MarkovModelWindow.h
#SOURCES += MarkovModelWindow.cpp

#HEADERS += Random.h

#HEADERS += StimulusClampProtocol.h
#SOURCES += StimulusClampProtocol.cpp

#HEADERS += StimulusClampProtocolPlot.h
#SOURCES += StimulusClampProtocolPlot.cpp

#HEADERS += StimulusClampProtocolSimulation.h
#SOURCES += StimulusClampProtocolSimulation.cpp

#HEADERS += StimulusClampProtocolWindow.h
#SOURCES += StimulusClampProtocolWindow.cpp

#HEADERS += StimulusClampProtocolSimulationDialog.h
#SOURCES += StimulusClampProtocolSimulationDialog.cpp

#HEADERS += StimulusClampProtocolOptimizationDialog.h
#SOURCES += StimulusClampProtocolOptimizationDialog.cpp

#HEADERS += Minimizer.h
#SOURCES += Minimizer.cpp

#HEADERS += MultipleIndexSpinBox.h
#SOURCES += MultipleIndexSpinBox.cpp

#HEADERS += Text3D.h
#SOURCES += Text3D.cpp

#HEADERS += Variable.h
#SOURCES += Variable.cpp

#HEADERS += WindowManager.h
#SOURCES += WindowManager.cpp

#HEADERS += test/test_ObjectListPropertyTableView.h
#SOURCES += test/test_ObjectListPropertyTableView.cpp
