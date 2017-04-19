/* --------------------------------------------------------------------------------
 * Author: Marcel Paz Goldschen-Ohm
 * Email: marcel.goldschen@gmail.com
 * -------------------------------------------------------------------------------- */

#include "QMultipleIndexSpinBox.h"
#include <algorithm>
#include <sstream>
#include <QRegularExpression>

QMultipleIndexSpinBox::QMultipleIndexSpinBox(QWidget *parent) :
    QAbstractSpinBox(parent)
{
    lineEdit()->setText("");
    setRange(1, 99);
    connect(this, SIGNAL(editingFinished()), this, SLOT(finishedChangingText()));
    connect(lineEdit(), SIGNAL(editingFinished()), this, SLOT(finishedChangingText()));
    connect(lineEdit(), SIGNAL(returnPressed()), this, SLOT(finishedChangingText()));
}

int QMultipleIndexSpinBox::minimum()
{
    std::vector<int> indices = indicesInRange();
    return (indices.size() ? indices.front() : _min);
}

int QMultipleIndexSpinBox::maximum()
{
    std::vector<int> indices = indicesInRange();
    return (indices.size() ? indices.back() : _max);
}

std::vector<int> QMultipleIndexSpinBox::indicesInRange()
{
    std::vector<int> indexes = str2indexes(text());
    std::sort(indexes.begin(), indexes.end()); // ascending order
    indexes.resize(std::distance(indexes.begin(), std::unique(indexes.begin(), indexes.end()))); // keep unique only
    std::vector<int> indexesInRange;
    for(int index : indexes) {
        if(index >= _min && index <= _max)
            indexesInRange.push_back(index);
    }
    return indexesInRange;
}

void QMultipleIndexSpinBox::stepBy(int steps)
{
    if(steps < 0) {
        int index = minimum() - 1;
        if(index < _min)
            index = _max;
        lineEdit()->setText(QString::number(index));
        emit valueChanged();
    } else if(steps > 0) {
        int index = maximum() + 1;
        if(index > _max)
            index = _min;
        lineEdit()->setText(QString::number(index));
        emit valueChanged();
    }
}

std::vector<int> QMultipleIndexSpinBox::str2indexes(const QString &str, const QString &delimiterRegex, const QString &rangeDelimiterRegex)
{
    std::vector<int> vec;
    std::istringstream iss;
    int value;
    QStringList fields = str.split(QRegularExpression(delimiterRegex), QString::SkipEmptyParts);
    vec.reserve(fields.size());
    foreach(QString field, fields) {
        field = field.trimmed();
        if(!field.isEmpty()) {
            QStringList subfields = field.split(QRegularExpression(rangeDelimiterRegex), QString::SkipEmptyParts);
            if(subfields.size() == 1) { // single value
                iss.clear();
                iss.str(field.toStdString());
                iss >> value;
                if(!iss.fail() && !iss.bad() && iss.eof())
                    vec.push_back(value);
            } else if(subfields.size() == 2) { // start:stop
                int start;
                iss.clear();
                iss.str(subfields[0].toStdString());
                iss >> start;
                if(!iss.fail() && !iss.bad() && iss.eof()) {
                    int stop;
                    iss.clear();
                    iss.str(subfields[1].toStdString());
                    iss >> stop;
                    if(!iss.fail() && !iss.bad() && iss.eof()) {
                        for(value = start; value <= stop; value += 1)
                            vec.push_back(value);
                    }
                }
            } else if(subfields.size() == 3) { // start:step:stop
                int start;
                iss.clear();
                iss.str(subfields[0].toStdString());
                iss >> start;
                if(!iss.fail() && !iss.bad() && iss.eof()) {
                    int step;
                    iss.clear();
                    iss.str(subfields[1].toStdString());
                    iss >> step;
                    if(!iss.fail() && !iss.bad() && iss.eof()) {
                        int stop;
                        iss.clear();
                        iss.str(subfields[2].toStdString());
                        iss >> stop;
                        if(!iss.fail() && !iss.bad() && iss.eof()) {
                            if(step > 0) {
                                for(value = start; value <= stop; value += step)
                                    vec.push_back(value);
                            } else if(step < 0) {
                                for(value = start; value >= stop; value += step)
                                    vec.push_back(value);
                            }
                        }
                    }
                }
            }
        }
    }
    return vec;
}
