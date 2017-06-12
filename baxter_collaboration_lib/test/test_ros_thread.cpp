#include <gtest/gtest.h>
#include "robot_utils/ros_thread.h"

class ROSThreadObject: public ROSThread
{
private:
    void InternalThreadEntry()
    {
        std::cout << "test" << std::endl;
    }
};

TEST(RosThreadTest, testConstructorDefaultValues)
{
    ROSThreadObject rt;

    EXPECT_TRUE(rt.startInternalThread());
    rt.joinInternalThread();
}

// Run all the tests that were declared with TEST()
int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
