#include <gtest/gtest.h>
#include <memory>

#include "robot_utils/ros_thread_obj.h"

using namespace std;

class ROSThreadObjTest
{
private:
    std::shared_ptr<int> var;
    std::vector<ROSThreadObj> threads;
    std::vector<void *(*)(void *)> functions; // <out_type(*)(param_types)>
public:
    /**
     * Constructors
     * @param var_value: value of shared pointer var
     * @param threads_no: no of threads
     *
     */
    ROSThreadObjTest(): var(std::make_shared<int>(0)) {}
    ROSThreadObjTest(int var_value, int threads_no): var(std::make_shared<int>(var_value))
    {
        for(int i=0; i < threads_no; i++)
        {
            threads.push_back(ROSThreadObj());
        }
    }

    /**
     * Adds a specified number (one by default) threads to the function
     * @param threads_no: number of threads to add
     */
    void add_thread(int threads_no=1)
    {
        for(int i=0; i < threads_no; i++)
        {
            threads.push_back(ROSThreadObj());
        }
    }

    /**
     * add a function to the functions vector
     * @param func: the pointer to a function
     */
    void add_function(void *(* func)(void *))
    {
        functions.push_back(func);
    }

    /**
    * returns the value of the shared ptr
    */
    int var_value()
    {
        return *var;
    }

    /**
    * returns the use count of the shared ptr
    */
    int var_count()
    {
        return var.use_count();
    }

    /**
     * starts each ROSThreadObj thread with a matching function in order
     * then joins threads in order
     * @return true if success, false if failure
     */
    bool start_threads()
    {
        if(threads.size() != functions.size())
        {
            return false;
        }

        for(int i=0, size=threads.size(); i < size; i++)
        {
            threads[i].start(functions[i], static_cast<void*>(var.get()));
        }

        for(int i=0, size=threads.size(); i < size; i++)
        {
            threads[i].join();
        }

        return true;
    }

    /**
     * Provides useful output
     */
    void debugger()
    {
        cout << "This ROSThreadObjTest object's address is: " << this << endl;
        cout << "### VAR INFO ###" << endl;
        cout << "*var is: " << *var << endl;
        cout << "&var is: " << &var << endl;
        cout << "var is: " << var << endl;
        cout << "var.get() is: " << var.get() << endl;
        cout << "### THREADS INFO ###" << endl;
        cout << "The threads vector size is: " << threads.size() << endl;
        cout << "The threads vector address is: " << &threads << endl;
        cout << "threads[0] has a type: " << typeid(threads[0]).name() << endl;
        cout << "### FUNCTIONS INFO ###" << endl;
        cout << "The functions vector size is: " << functions.size() << endl;
        cout << "functions[0] has a type: " << typeid(functions[0]).name() << endl;
    }
};

/**
 * thread entry function that prints var
 * @param var: the value of the shared ptr
 */
void* print_var(void* var)
{
    auto var_instance = static_cast<int*>(var);
    cout << *var_instance << endl;
    return (void*) 1;
}

/**
 * thread entry function that adds 1 to var
 * @param var: the value of the shared ptr
 */
void* one_adder(void* var)
{
    auto var_instance = static_cast<int*>(var);
    *var_instance = *var_instance + 1;
    return (void*) 1;
}

/**
 * thread entry function that adds 1 to var
 * @param var: the value of the shared ptr
 */
void* ten_adder(void* var)
{
    auto var_instance = static_cast<int*>(var);
    for(int i = 0; i < 10; i++)
    {
        *var_instance = *var_instance + 1;
        sleep(0.1);
    }
    return (void*) 1;
}

TEST(ROSThreadTest, testSingleThread)
{
   ROSThreadObjTest rtot(10, 0);
   rtot.add_function(ten_adder);
   rtot.add_thread();
   rtot.start_threads();
   EXPECT_EQ(20, rtot.var_value());
}

TEST(ROSThreadTest, testConcurrentThreads)
{
   ROSThreadObjTest rtot(0, 2);
   rtot.add_function(ten_adder);
   rtot.add_function(ten_adder);
   rtot.start_threads();
   EXPECT_EQ(20, rtot.var_value());
}

// Run all the tests that were declared with TEST()
int main(int argc, char **argv)
{
    ros::init(argc, argv, "test_ros_thread_obj");
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}