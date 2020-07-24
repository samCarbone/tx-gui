#ifndef ALTITUDECONTROLLER_H
#define ALTITUDECONTROLLER_H

#include <QObject>
#include "def.h"

typedef struct {
    double pos_z;
    double vel_z;
    int time;
} AltState_t;

class AltitudeController : public QObject
{
    Q_OBJECT
public:
    explicit AltitudeController(QObject *parent = nullptr);
    AltState_t getState();
    AltState_t getForwardState(int time);

public slots:
    void altitudeReceived(RangingData_t alt);

signals:

private:
    bool state_set = false;
    AltState_t state_est;
    RangingData_t measurement;
};

#endif // ALTITUDECONTROLLER_H
