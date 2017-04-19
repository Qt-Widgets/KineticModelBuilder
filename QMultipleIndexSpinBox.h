/* --------------------------------------------------------------------------------
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#ifndef __QMultipleIndexSpinBox_H__
#define __QMultipleIndexSpinBox_H__

#include <vector>
#include <QAbstractSpinBox>
#include <QLineEdit>
#include <QString>
#ifdef DEBUG
#include <iostream>
#include <QDebug>
#endif

/* --------------------------------------------------------------------------------
 * -------------------------------------------------------------------------------- */
class QMultipleIndexSpinBox : public QAbstractSpinBox
{
    Q_OBJECT
    
public:
    QMultipleIndexSpinBox(QWidget *parent = 0);

    void setRange(int min, int max) { _min = min; _max = max; }
    void setValue(int i) { lineEdit()->setText(QString::number(i)); }
    int minimum();
    int maximum();
    std::vector<int> indicesInRange();
    void stepBy(int steps);
    QValidator::State validate(QString & /* input */, int & /* pos */) const { return QValidator::Acceptable; }
    
    static std::vector<int> str2indexes(const QString &str, const QString &delimiterRegex = "[,\\s]\\s*", const QString &rangeDelimiterRegex = ":");

protected:
    StepEnabled stepEnabled() const { return (StepUpEnabled | StepDownEnabled); }

public slots:
    void textWasChanged(QString) { emit valueChanged(); }
    void finishedChangingText() { emit valueChanged(); }

signals:
    void valueChanged();

private:
    int _min, _max;
};

#endif
