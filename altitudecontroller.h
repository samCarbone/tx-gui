#ifndef ALTITUDECONTROLLER_H
#define ALTITUDECONTROLLER_H

#include <QObject>
#include <fstream>
#include "def.h"
#include <eigen/Eigen/Dense>

class AltitudeController : public QObject
{
    Q_OBJECT
public:
    explicit AltitudeController(QObject *parent = nullptr);
    AltState_t getState();
    AltState_t getForwardState(int time_pc, bool limit=false, int limit_ms=100);
    double LQR(AltState_t target_state, AltState_t current_state);
    Q_INVOKABLE int getAltitudeEstimate();

    Q_INVOKABLE bool openFiles();
    Q_INVOKABLE void closeFiles();
    Q_INVOKABLE void setSuffix(QString suffix_in);
    Q_INVOKABLE std::string getSuffix();
    Q_INVOKABLE void setFileDirectory(QString directory);

public slots:
    void altitudeReceived(RangingData_t alt);

signals:

private:
    bool state_set = false;
    RangingData_t prev_meas;
    int time_esp;
    int time_pc;
    Eigen::Matrix<double, 2, 1> x; // [z (mm), z_dot (mm/s)]
    Eigen::Matrix<double, 2, 2> P;
    double sigma_v = 100; // mm/s, process uncertainty in velocity

    std::ofstream file_alt_est;
    bool record = false;
    std::string header_alt_est = "time_esp_ms,time_pc_ms,z,z_dot,P_11,P_12,P_21,P_22,Delta_t_s,range_mm,sigma_mm,sigma_v";
    std::string prefix_alt_est = "alt_est_";
    std::string format = ".txt";
    std::string suffix = "temp";
    std::string fileDirectory = "";
};

#endif // ALTITUDECONTROLLER_H
