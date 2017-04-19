/* --------------------------------------------------------------------------------
 * 3D viewer for a MarkovModel.
 *
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __MarkovModelWindow_H__
#define __MarkovModelWindow_H__

#include <QMainWindow>
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif
namespace KineticModelBuilder { class Project; }

namespace MarkovModel
{
    class MarkovModel;
    class MarkovModelViewer;
    
    /* --------------------------------------------------------------------------------
     * UI window for a MarkovModel.
     * -------------------------------------------------------------------------------- */
    class MarkovModelWindow : public QMainWindow
    {
        Q_OBJECT
        
    public:
        MarkovModelWindow(MarkovModel* model, QWidget *parent = 0);
        
        MarkovModel* model() const { return _model; }
        MarkovModelViewer* viewer() const { return _viewer; }
        
    protected:
        KineticModelBuilder::Project *_project;
        MarkovModel *_model;
        MarkovModelViewer *_viewer;
    };
    
} // MarkovModel

#endif
