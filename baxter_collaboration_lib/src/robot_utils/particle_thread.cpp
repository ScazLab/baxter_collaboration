/**
 * Copyright (C) 2017 Social Robotics Lab, Yale University
 * Author: Alessandro Roncone
 * email:  alessandro.roncone@yale.edu
 * website: www.scazlab.yale.edu
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU Lesser General Public License, version 2.1 or any
 * later version published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
**/

#include "robot_utils/particle_thread.h"

/*****************************************************************************/
/*                             ParticleThread                                */
/*****************************************************************************/

ParticleThread::ParticleThread(std::string _name, double _thread_rate) :
                               nh(_name), spinner(4), name(_name), r(_thread_rate),
                               is_running(false), is_closing(false),
                               start_time(ros::Time::now()), is_particle_set(false)
{
    spinner.start();
}

void ParticleThread::internalThread()
{
    start_time = ros::Time::now();

    while(ros::ok() && not isClosing())
    {
        // ROS_INFO("Running..");
        Eigen::VectorXd new_pt;

        updateParticle(new_pt);
        setCurrPoint(new_pt);

        ROS_INFO_STREAM("New particle position: " << getCurrPoint().transpose());

        r.sleep();
    }
}

bool ParticleThread::start()
{
    if (is_particle_set && not isRunning())
    {
        setIsClosing(false);
        thread     = std::thread(&ParticleThread::internalThread, this);
        setIsRunning(true);

        return true;
    }
    else
    {
        return false;
    }
}

bool ParticleThread::stop()
{
    setIsClosing(true);
    setIsRunning(false);
    is_particle_set = false;

    if (thread.joinable())
    {
        thread.join();
    }

    setIsClosing(false);

    return true;
}

bool ParticleThread::setIsClosing(bool _is_closing)
{
    std::lock_guard<std::mutex> lock(mtx_is_closing);
    is_closing = _is_closing;

    return true;
}

bool ParticleThread::isClosing()
{
    std::lock_guard<std::mutex> lock(mtx_is_closing);
    // ROS_INFO("isClosing %s", is_closing?"TRUE":"FALSE");

    return is_closing;
}

bool ParticleThread::setIsRunning(bool _is_running)
{
    std::lock_guard<std::mutex> lock(mtx_is_running);
    is_running = _is_running;

    return true;
}

double ParticleThread::getRate()
{
    return 1/r.expectedCycleTime().toSec();
}

bool ParticleThread::isRunning()
{
    std::lock_guard<std::mutex> lock(mtx_is_running);
    return is_running;
}

Eigen::VectorXd ParticleThread::getCurrPoint()
{
    std::lock_guard<std::mutex> lg(mtx_curr_pt);
    return curr_pt;
}

bool ParticleThread::setCurrPoint(const Eigen::VectorXd& _curr_pt)
{
    std::lock_guard<std::mutex> lg(mtx_curr_pt);
    curr_pt = _curr_pt;
    return true;
}

ParticleThread::~ParticleThread()
{
    stop();
}

/*****************************************************************************/
/*                           ParticleThreadImpl                              */
/*****************************************************************************/
ParticleThreadImpl::ParticleThreadImpl(std::string _name, double _thread_rate) :
                                           ParticleThread(_name, _thread_rate)
{
    is_particle_set = true;
}

bool ParticleThreadImpl::updateParticle(Eigen::VectorXd& _new_pt)
{
    _new_pt = Eigen::Vector3d(1.0, 1.0, 1.0);

    return true;
}

/*****************************************************************************/
/*                          LinearPointParticle                              */
/*****************************************************************************/

LinearPointParticle::LinearPointParticle(std::string _name, double _thread_rate) :
                                         ParticleThread(_name, _thread_rate),
                                         speed(0.0), start_pt(0.0, 0.0, 0.0),
                                         des_pt(0.0, 0.0, 0.0)
{

}

bool LinearPointParticle::updateParticle(Eigen::VectorXd& _new_pt)
{
    double elap_time = (ros::Time::now() - start_time).toSec();

    Eigen::Vector3d p_sd = des_pt - start_pt;

    // We model the particle as a 3D point that moves toward the
    // target with a straight trajectory and constant speed.
    _new_pt = start_pt + p_sd / p_sd.norm() * speed * elap_time;

    Eigen::Vector3d p_cd = des_pt - _new_pt;

    // Check if the current position is overshooting the desired position
    // By checking the sign of the cosine of the angle between p_sd and p_cd
    // This would mean equal to 1 within some small epsilon (1e-8)
    if ((p_sd.dot(p_cd))/(p_sd.norm()*p_cd.norm()) - 1 <  EPSILON &&
        (p_sd.dot(p_cd))/(p_sd.norm()*p_cd.norm()) - 1 > -EPSILON)
    {
        return true;
    }

    _new_pt = des_pt;
    return false;
}

bool LinearPointParticle::setupParticle(const Eigen::Vector3d& _start_pt,
                                        const Eigen::Vector3d&   _des_pt,
                                        double _speed)
{
    start_pt = _start_pt;
      des_pt =   _des_pt;
       speed =    _speed;

    is_particle_set = true;

    return true;
}

LinearPointParticle::~LinearPointParticle()
{

}

