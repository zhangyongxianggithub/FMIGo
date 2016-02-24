#include "master/FMIClient.h"
#include "assert.h"

using namespace fmitcp_master;

StrongConnector::StrongConnector(FMIClient* slave) : sc::Connector() {
    m_userData = (void*)slave;
    m_hasPosition = m_hasQuaternion = m_hasShaftAngle = m_hasVelocity = m_hasAcceleration = m_hasAngularVelocity = m_hasAngularAcceleration = m_hasForce = m_hasTorque = false;
}

StrongConnector::~StrongConnector(){

}

void StrongConnector::setPositionValueRefs(int x, int y, int z){
    m_hasPosition = true;
    m_vref_position[0] = x;
    m_vref_position[1] = y;
    m_vref_position[2] = z;
}

void StrongConnector::setQuaternionValueRefs(int x, int y, int z, int w){
    m_hasQuaternion = true;
    m_vref_quaternion[0] = x;
    m_vref_quaternion[1] = y;
    m_vref_quaternion[2] = z;
    m_vref_quaternion[3] = w;
}

void StrongConnector::setShaftAngleValueRef(int x){
    m_hasShaftAngle = true;
    m_vref_shaftAngle = x;
}

void StrongConnector::setVelocityValueRefs(int x, int y, int z){
    m_hasVelocity = true;
    m_vref_velocity[0] = x;
    m_vref_velocity[1] = y;
    m_vref_velocity[2] = z;
}

void StrongConnector::setAccelerationValueRefs(int x, int y, int z){
    m_hasAcceleration = true;
    m_vref_acceleration[0] = x;
    m_vref_acceleration[1] = y;
    m_vref_acceleration[2] = z;
}

void StrongConnector::setAngularVelocityValueRefs(int x, int y, int z){
    m_hasAngularVelocity = true;
    m_vref_angularVelocity[0] = x;
    m_vref_angularVelocity[1] = y;
    m_vref_angularVelocity[2] = z;
}

void StrongConnector::setAngularAccelerationValueRefs(int x, int y, int z){
    m_hasAngularAcceleration = true;
    m_vref_angularAcceleration[0] = x;
    m_vref_angularAcceleration[1] = y;
    m_vref_angularAcceleration[2] = z;
}

void StrongConnector::setForceValueRefs(int x, int y, int z){
    m_hasForce = true;
    m_vref_force[0] = x;
    m_vref_force[1] = y;
    m_vref_force[2] = z;
}

void StrongConnector::setTorqueValueRefs(int x, int y, int z){
    m_hasTorque = true;
    m_vref_torque[0] = x;
    m_vref_torque[1] = y;
    m_vref_torque[2] = z;
}


bool StrongConnector::hasPosition(){ return m_hasPosition; };
bool StrongConnector::hasQuaternion(){ return m_hasQuaternion; };
bool StrongConnector::hasShaftAngle(){ return m_hasShaftAngle; };
bool StrongConnector::hasVelocity(){ return m_hasVelocity; };
bool StrongConnector::hasAcceleration(){ return m_hasAcceleration; };
bool StrongConnector::hasAngularVelocity(){ return m_hasAngularVelocity; };
bool StrongConnector::hasAngularAcceleration(){ return m_hasAngularAcceleration; };
bool StrongConnector::hasForce(){ return m_hasForce; };
bool StrongConnector::hasTorque(){ return m_hasTorque; };

std::vector<int> StrongConnector::getForceValueRefs() const {
    std::vector<int> result;
    for(int i=0; m_hasForce && i<3; i++)
        result.push_back(m_vref_force[i]);
    return result;
};

std::vector<int> StrongConnector::getVelocityValueRefs() const {
    std::vector<int> result;
    if(m_hasVelocity){
        for(int i=0; i<3; i++)
            result.push_back(m_vref_velocity[i]);
    }
    return result;
};

std::vector<int> StrongConnector::getAccelerationValueRefs() const {
    std::vector<int> result;
    if(m_hasAcceleration){
        for(int i=0; i<3; i++)
            result.push_back(m_vref_acceleration[i]);
    }
    return result;
}

std::vector<int> StrongConnector::getPositionValueRefs() const {
    std::vector<int> result;
    for(int i=0; m_hasPosition && i<3; i++)
        result.push_back(m_vref_position[i]);
    return result;
};

std::vector<int> StrongConnector::getQuaternionValueRefs() const {
    std::vector<int> result;
    for(int i=0; m_hasQuaternion && i<4; i++)
        result.push_back(m_vref_quaternion[i]);
    return result;
};

std::vector<int> StrongConnector::getShaftAngleValueRefs() const {
    std::vector<int> result;
    if (m_hasShaftAngle) {
        result.push_back(m_vref_shaftAngle);
    }
    return result;
}

std::vector<int> StrongConnector::getAngularVelocityValueRefs() const {
    std::vector<int> result;
    for(int i=0; m_hasAngularVelocity && i<3; i++)
        if (m_vref_angularVelocity[i] >= 0)
        result.push_back(m_vref_angularVelocity[i]);
    return result;
};

std::vector<int> StrongConnector::getAngularAccelerationValueRefs() const {
    std::vector<int> result;
    for(int i=0; m_hasAngularAcceleration && i<3; i++)
        if (m_vref_angularAcceleration[i] >= 0)
        result.push_back(m_vref_angularAcceleration[i]);
    return result;
};

std::vector<int> StrongConnector::getTorqueValueRefs() const {
    std::vector<int> result;
    for(int i=0; m_hasTorque && i<3; i++)
        if (m_vref_torque[i] >= 0)
        result.push_back(m_vref_torque[i]);
    return result;
};

void StrongConnector::setValues(std::vector<int> valueReferences, std::vector<double> values){
    //NOTE: acceleration values are never written, only read
    assert(valueReferences.size() == values.size());

    for(int i=0; i<valueReferences.size(); i++){
        int vr = valueReferences[i];
        double val = values[i];
        for(int j=0; j<3; j++){
            if(m_hasPosition        && vr == m_vref_position[j])        m_position[j] =          val;
            if(m_hasVelocity        && vr == m_vref_velocity[j])        m_velocity[j] =          val;
            if(m_hasAngularVelocity && vr == m_vref_angularVelocity[j]) m_angularVelocity[j] =   val;
            if(m_hasForce           && vr == m_vref_force[j])           m_force[j] =             val;
            if(m_hasTorque          && vr == m_vref_torque[j])          m_torque[j] =            val;
        }
        for(int j=0; j<4; j++){
            if(m_hasQuaternion      && vr == m_vref_quaternion[j])      m_quaternion[j] =        val;
        }
        if (m_hasShaftAngle         && vr == m_vref_shaftAngle)         m_shaftAngle =           val;
    }
};

void StrongConnector::setFutureValues(std::vector<int> valueReferences, std::vector<double> values){
    assert(valueReferences.size() == values.size());

    for(int i=0; i<valueReferences.size(); i++){
        int vr = valueReferences[i];
        double val = values[i];
        for(int j=0; j<3; j++){
            if(m_hasVelocity        && vr == m_vref_velocity[j])        m_futureVelocity[j] =          val;
            if(m_hasAngularVelocity && vr == m_vref_angularVelocity[j]) m_futureAngularVelocity[j] =   val;
        }
    }
};

bool StrongConnector::matchesBallLockConnector(
        int posX, int posY, int posZ,
        int accX, int accY, int accZ,
        int forceX, int forceY, int forceZ,
        int quatX, int quatY, int quatZ, int quatW,
        int angAccX, int angAccY, int angAccZ,
        int torqueX, int torqueY, int torqueZ) {
    //ball/lock joints require position, acceleration etc.
    //if this connector is missing any of them, then it isn't a match
    if (!m_hasPosition || !m_hasAcceleration || !m_hasForce || !m_hasQuaternion || !m_hasAngularAcceleration || !m_hasTorque) {
        return false;
    }

    //holey moley this is ugly. but check that the value references match
    //a better solutions could be to make use of polymorphism and have different kinds of StrongConnectors
    if (    posX != m_vref_position[0] ||
            posY != m_vref_position[1] ||
            posZ != m_vref_position[2] ||
            accX != m_vref_acceleration[0] ||
            accY != m_vref_acceleration[1] ||
            accZ != m_vref_acceleration[2] ||
            forceX != m_vref_force[0] ||
            forceY != m_vref_force[1] ||
            forceZ != m_vref_force[2] ||
            quatX != m_vref_quaternion[0] ||
            quatY != m_vref_quaternion[1] ||
            quatZ != m_vref_quaternion[2] ||
            quatW != m_vref_quaternion[3] ||
            angAccX != m_vref_angularAcceleration[0] ||
            angAccY != m_vref_angularAcceleration[1] ||
            angAccZ != m_vref_angularAcceleration[2] ||
            torqueX != m_vref_torque[0] ||
            torqueY != m_vref_torque[1] ||
            torqueZ != m_vref_torque[2]) {
        return false;
    }
    return true;
}

bool StrongConnector::matchesShaftConnector(int angle, int angularVel, int angularAcc, int torque) {
    //this works similar to matchesBallLockConnector(), but for 1-dimentional shaft angle connectors
    if (!m_hasShaftAngle || !m_hasAngularVelocity || !m_hasAngularAcceleration || !m_hasTorque) {
        return false;
    }

    if (    m_vref_shaftAngle != angle ||
            m_vref_angularVelocity[0] != angularVel ||
            m_vref_angularVelocity[1] != -1 ||
            m_vref_angularVelocity[2] != -1 ||
            m_vref_angularAcceleration[0] != angularAcc ||
            m_vref_angularAcceleration[1] != -1 ||
            m_vref_angularAcceleration[2] != -1 ||
            m_vref_torque[0] != torque ||
            m_vref_torque[1] != -1 ||
            m_vref_torque[2] != -1) {
        return false;
    }

    return true;
}