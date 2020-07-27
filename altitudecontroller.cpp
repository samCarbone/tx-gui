#include "altitudecontroller.h"
#include <math.h>
#include <iostream>

AltitudeController::AltitudeController(QObject *parent) : QObject(parent)
{

    // Initialise state and covariance matrices
    x.fill(0);
    P.fill(10);

}

void AltitudeController::altitudeReceived(RangingData_t alt)
{
    prev_meas = alt;

    if(!state_set) {
        x(0, 0) = alt.range_mm;
        x(1, 0) = 0;
        time_esp = alt.time_esp_ms;
        time_pc = alt.time_recv_ms;
        state_set = true;
        return;
    }

    // Time delta from previous measurement
    double Delta_t = (alt.time_esp_ms - time_esp)/1000.0; // delta_t in s
    time_esp = alt.time_esp_ms;
    time_pc = alt.time_recv_ms;
    // State transition
    Eigen::Matrix<double, 2, 2> F;
    F << 1, Delta_t,
         0, 1;
    // Process covariance
    Eigen::Matrix<double, 2, 2> Q;
    Q << pow(Delta_t, 2)*pow(sigma_v, 2), Delta_t*pow(sigma_v, 2),
         Delta_t*pow(sigma_v, 2), pow(sigma_v, 2);
    // Measurement variance
    Eigen::Matrix<double, 1, 1> R;
    R << pow(alt.sigma_mm, 2);
    // Measurement
    Eigen::Matrix<double, 1, 1> z;
    z << alt.range_mm;
    // Measurement map matrix
    Eigen::Matrix<double, 1, 2> H;
    H << 1, 0;

    // Propagate with constant velocity assumption
    x = F*x;
    P = F*P*F.inverse() + Q;
    // Measurement update
    // Kalman gain
    Eigen::Matrix<double, 2, 1> K;
    K = P*H.transpose()*((H*P*H.transpose() + R).inverse());
    // State update
    x = x + K*(z - H*x);
    // State covariance update
    P = (Eigen::MatrixXd::Identity(2,2) - K*H)*P*((Eigen::MatrixXd::Identity(2,2) - K*H).transpose()) + K*R*K.transpose();

    // File writes
    if(record) {
        // header
        // time_esp_ms,time_pc_ms,z,z_dot,P_11,P_12,P_21,P_22,Delta_t_s,range_mm,sigma_mm,sigma_v
        file_alt_est << time_esp << "," << time_pc << "," << x(0,0) << "," << x(1,0) << ","
                     << P(0,0) << "," << P(0,1) << "," << P(1,0) << "," << P(1,1) << ","
                     << Delta_t << "," << alt.range_mm << "," << alt.sigma_mm << "," << sigma_v << std::endl;
    }

}

int AltitudeController::getAltitudeEstimate()
{
    return x(0,0);
}

AltState_t AltitudeController::getState() {
    AltState_t state_result;
    state_result.z = x(0,0);
    state_result.z_dot = x(0,1);
    state_result.time_esp = time_esp;
    state_result.time_pc = time_pc;
    state_result.P = P;
    return state_result;
}

AltState_t AltitudeController::getForwardState(int current_time_pc, bool limit, int limit_ms)
{
    // Time difference based on PC time
    double Delta_t = (current_time_pc - time_pc)/1000.0;

    if(limit && current_time_pc - time_pc > limit_ms) {
        std::cout << "[warn] Long forward propagation time: " << current_time_pc - time_pc << "ms" << std::endl;
        Delta_t = limit_ms/1000.0;
    }

    Eigen::Matrix<double, 2, 2> F;
    F << 1, Delta_t,
         0, 1;
    // Process covariance
    Eigen::Matrix<double, 2, 2> Q;
    Q << pow(Delta_t, 2)*pow(sigma_v, 2), Delta_t*pow(sigma_v, 2),
         Delta_t*pow(sigma_v, 2), pow(sigma_v, 2);
    // Propagate with constant velocity assumption
    Eigen::Matrix<double, 2, 1> x_prop;
    Eigen::Matrix<double, 2, 2> P_prop;
    x_prop = F*x;
    P_prop = F*P*F.inverse() + Q;

    AltState_t state_result;
    state_result.z = x_prop(0,0);
    state_result.z_dot = x_prop(1,0);
    state_result.time_esp = time_esp + current_time_pc - time_pc; // Push the estimated esp time forward as well
    state_result.time_pc = current_time_pc;
    state_result.P = P_prop;
    return state_result;

}

double AltitudeController::LQR(AltState_t target_state, AltState_t current_state)
{
    // Need to make controller here
    return -100;
}

bool AltitudeController::openFiles() {
    if (file_alt_est.is_open())
        return false;
    std::string name_alt_est = fileDirectory + "/" + prefix_alt_est + suffix + format;
    file_alt_est.open(name_alt_est, std::ios::out | std::ios::app); // Append the file contents to prevent overwrite
    if (file_alt_est.is_open()) {
        file_alt_est << std::endl << header_alt_est << std::endl;
        record = true;
        return true;
    }
    else {
        return false;
    }
}

void AltitudeController::closeFiles() {
    file_alt_est.close();
    record = false;
}

void AltitudeController::setSuffix(QString suffix_in) {
    suffix = suffix_in.toStdString();
}

std::string AltitudeController::getSuffix() {
    return suffix;
}

void AltitudeController::setFileDirectory(QString directory) {
    fileDirectory = directory.toStdString();
}
