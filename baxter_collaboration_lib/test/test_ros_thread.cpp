#include <gtest/gtest.h>
#include "robot_utils/ros_thread.h"
//#include <pthread.h>
//#include <typeinfo>
#include "robot_utils/ros_thread_obj.h"



class ROSThreadObject: public ROSThread
{
   public:
        int value;
        void InternalThreadEntry();
};

 void ROSThreadObject::InternalThreadEntry() {
        std::cout << "test" << std::endl;
}; 

TEST(RosThreadTest, testConstructorDefaultValues)
{
    ROSThreadObject rt;
    rt.InternalThreadEntry();

    rt.joinInternalThread();
    EXPECT_TRUE(rt.startInternalThread());
    //EXPECT_TRUE(rt.closeInternalThread());
    EXPECT_TRUE(rt.killInternalThread());
}

// Run all the tests that were declared with TEST()
int main(int argc, char **argv)
{
    ros::init(argc, argv, "ros_thread_test"); 
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
