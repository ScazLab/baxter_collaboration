#ifndef __CUBE_PICKER_H__
#define __CUBE_PICKER_H__

#include <robot_interface/arm_ctrl.h>

class ToolPicker : public ArmCtrl
{
private:
    double elap_time;

    /**
     * [moveObjectTowardHuman description]
     * @return true/false if success/failure
     */
    bool moveObjectTowardHuman();

    /**
     * [getObject description]
     * @return true/false if success/failure
     */
    bool getObject();

    /**
     * [passObject description]
     * @return true/false if success/failure
     */
    bool passObject();

    /**
     * [getPassObject description]
     * @return true/false if success/failure
     */
    bool getPassObject();

    /**
     * Sets the joint-level configuration for the home position
     */
    void setHomeConfiguration();

public:
    /**
     * Constructor
     */
    ToolPicker(std::string _name, std::string _limb, bool _no_robot = false);

    /**
     * Destructor
     */
    ~ToolPicker();
};

#endif