#include <src/cpp/SharedObjectMgr.h>
#include <gtest/gtest.h>
#include <atomic>

namespace pongasoft::VST::SampleSplitter::Test {

struct MyTestValue
{
  explicit MyTestValue(int iValue = 0) : fValue(iValue)
  {
    instanceCounter++;
//    std::cout << "MyTestValue() = " << instanceCounter.load() << std::endl;
  }

  MyTestValue(MyTestValue const &other) : fValue{other.fValue}
  {
    instanceCounter++;
//    std::cout << "MyTestValue(cc) = " << instanceCounter.load() << std::endl;
  }

  ~MyTestValue()
  {
    instanceCounter--;
//    std::cout << "~MyTestValue = " << instanceCounter.load() << std::endl;
  }

  int fValue;

  static std::atomic<int> instanceCounter;
};

std::atomic<int> MyTestValue::instanceCounter{0};

// SharedObjectMgr - test
TEST(SharedObjectMgr, test)
{
  // no instance at start
  ASSERT_EQ(0, MyTestValue::instanceCounter.load());

  {
    SharedObjectMgr<MyTestValue> mgr{};

    bool updated{};

    // no instance should have been created
    ASSERT_EQ(0, MyTestValue::instanceCounter.load());
    ASSERT_TRUE(mgr.rtGetObject() == nullptr);
    ASSERT_TRUE(mgr.uiGetObject() == nullptr);

    // ui -> 34 [1]
    ASSERT_EQ(1, mgr.uiSetObject(std::make_shared<MyTestValue>(34)));
    ASSERT_EQ(1, MyTestValue::instanceCounter.load());
    ASSERT_TRUE(mgr.rtGetObject() == nullptr);
    ASSERT_EQ(34, mgr.uiGetObject()->fValue);

    // adjusting from RT
    ASSERT_EQ(34, mgr.rtAdjustObjectFromUI(1, &updated)->fValue);
    ASSERT_EQ(1, MyTestValue::instanceCounter.load());
    ASSERT_EQ(34, mgr.uiGetObject()->fValue);
    ASSERT_EQ(34, mgr.rtGetObject()->fValue);
    ASSERT_TRUE(updated);

    // rt -> 45 [2]
    ASSERT_EQ(2, mgr.rtSetObject(std::make_shared<MyTestValue>(45)));
    ASSERT_EQ(2, MyTestValue::instanceCounter.load()); // 2 instances
    ASSERT_EQ(45, mgr.rtGetObject()->fValue);
    ASSERT_EQ(34, mgr.uiGetObject()->fValue);

    // adjusting from UI
    ASSERT_EQ(45, mgr.uiAdjustObjectFromRT(2, &updated)->fValue);
    ASSERT_EQ(1, MyTestValue::instanceCounter.load()); // back to 1
    ASSERT_EQ(45, mgr.uiGetObject()->fValue);
    ASSERT_EQ(45, mgr.rtGetObject()->fValue);
    ASSERT_TRUE(updated);

    // can't replay past events
    ASSERT_TRUE(mgr.uiAdjustObjectFromRT(1, &updated) == nullptr);
    ASSERT_FALSE(updated);
    ASSERT_TRUE(mgr.rtAdjustObjectFromUI(1, &updated) == nullptr);
    ASSERT_FALSE(updated);
    ASSERT_TRUE(mgr.uiAdjustObjectFromRT(2, &updated) == nullptr);
    ASSERT_FALSE(updated);
    ASSERT_TRUE(mgr.rtAdjustObjectFromUI(2, &updated) == nullptr);
    ASSERT_FALSE(updated);
    ASSERT_EQ(1, MyTestValue::instanceCounter.load());

    // rt -> 56 [3]
    ASSERT_EQ(3, mgr.rtSetObject(std::make_shared<MyTestValue>(56)));
    ASSERT_EQ(2, MyTestValue::instanceCounter.load()); // 2 instances
    ASSERT_EQ(56, mgr.rtGetObject()->fValue);
    ASSERT_EQ(45, mgr.uiGetObject()->fValue);

    // ui -> 67 [4]
    ASSERT_EQ(4, mgr.uiSetObject(std::make_shared<MyTestValue>(67)));
    ASSERT_EQ(2, MyTestValue::instanceCounter.load()); // 2 instances
    ASSERT_EQ(56, mgr.rtGetObject()->fValue);
    ASSERT_EQ(67, mgr.uiGetObject()->fValue);
    ASSERT_TRUE(mgr.uiAdjustObjectFromRT(3) == nullptr); // UI was set AFTER RT so it shouldn't change
    ASSERT_EQ(2, MyTestValue::instanceCounter.load()); // 2 instances
    ASSERT_EQ(56, mgr.rtGetObject()->fValue);
    ASSERT_EQ(67, mgr.uiGetObject()->fValue);

    // rt -> 78 [5]
    ASSERT_EQ(5, mgr.rtSetObject(std::make_shared<MyTestValue>(78)));
    ASSERT_EQ(2, MyTestValue::instanceCounter.load()); // 2 instances
    ASSERT_EQ(78, mgr.rtGetObject()->fValue);
    ASSERT_EQ(67, mgr.uiGetObject()->fValue);
    ASSERT_TRUE(mgr.rtAdjustObjectFromUI(4) == nullptr); // RT is more recent
    ASSERT_EQ(2, MyTestValue::instanceCounter.load()); // 2 instances
    ASSERT_EQ(78, mgr.rtGetObject()->fValue);
    ASSERT_EQ(67, mgr.uiGetObject()->fValue);

    // rt -> 89 [6]
    ASSERT_EQ(6, mgr.rtSetObject(std::make_shared<MyTestValue>(89)));
    ASSERT_EQ(2, MyTestValue::instanceCounter.load()); // 2 instances
    ASSERT_EQ(89, mgr.rtGetObject()->fValue);
    ASSERT_EQ(67, mgr.uiGetObject()->fValue);

    ASSERT_EQ(89, mgr.uiAdjustObjectFromRT(5)->fValue); // rt was changed twice and UI will get the latest value
    ASSERT_EQ(1, MyTestValue::instanceCounter.load()); // back to 1
    ASSERT_EQ(89, mgr.uiGetObject()->fValue);
    ASSERT_EQ(89, mgr.rtGetObject()->fValue);

    ASSERT_TRUE(mgr.uiAdjustObjectFromRT(6) == nullptr); // already up to date

    // ui -> 90 [7]
    ASSERT_EQ(7, mgr.uiSetObject(std::make_shared<MyTestValue>(90)));
    ASSERT_EQ(2, MyTestValue::instanceCounter.load()); // 2 instances
    ASSERT_EQ(89, mgr.rtGetObject()->fValue);
    ASSERT_EQ(90, mgr.uiGetObject()->fValue);
    ASSERT_TRUE(mgr.uiAdjustObjectFromRT(6) == nullptr); // UI is more recent
    ASSERT_EQ(2, MyTestValue::instanceCounter.load()); // 2 instances
    ASSERT_EQ(89, mgr.rtGetObject()->fValue);
    ASSERT_EQ(90, mgr.uiGetObject()->fValue);

    // ui -> 35 [8]
    ASSERT_EQ(8, mgr.uiSetObject(std::make_shared<MyTestValue>(35)));
    ASSERT_EQ(2, MyTestValue::instanceCounter.load()); // 2 instances
    ASSERT_EQ(89, mgr.rtGetObject()->fValue);
    ASSERT_EQ(35, mgr.uiGetObject()->fValue);

    ASSERT_EQ(35, mgr.rtAdjustObjectFromUI(7)->fValue); // UI was changed twice and RT will get the latest value
    ASSERT_EQ(1, MyTestValue::instanceCounter.load()); // back to 1
    ASSERT_EQ(35, mgr.uiGetObject()->fValue);
    ASSERT_EQ(35, mgr.rtGetObject()->fValue);

    ASSERT_TRUE(mgr.rtAdjustObjectFromUI(7) == nullptr); // already up to date

    // ui -> 46 [9]
    ASSERT_EQ(9, mgr.uiSetObject(std::make_shared<MyTestValue>(46)));
    ASSERT_EQ(2, MyTestValue::instanceCounter.load()); // 2 instances
    ASSERT_EQ(35, mgr.rtGetObject()->fValue);
    ASSERT_EQ(46, mgr.uiGetObject()->fValue);

    ASSERT_TRUE(mgr.rtAdjustObjectFromUI(10) == nullptr); // event does not exist

    // rt -> 57 [10]
    ASSERT_EQ(10, mgr.rtSetObject(std::make_shared<MyTestValue>(57)));
    ASSERT_EQ(2, MyTestValue::instanceCounter.load()); // 2 instances
    ASSERT_EQ(57, mgr.rtGetObject()->fValue);
    ASSERT_EQ(46, mgr.uiGetObject()->fValue);

    ASSERT_TRUE(mgr.uiAdjustObjectFromRT(11) == nullptr); // event does not exist

    // rt -> nullptr [11]
    ASSERT_EQ(11, mgr.rtSetObject(nullptr));
    ASSERT_EQ(1, MyTestValue::instanceCounter.load()); // 1 instance
    ASSERT_TRUE(mgr.rtGetObject() == nullptr);
    ASSERT_EQ(46, mgr.uiGetObject()->fValue);

    ASSERT_TRUE(mgr.uiAdjustObjectFromRT(9, &updated) == nullptr); // past event, nothing happens
    ASSERT_EQ(1, MyTestValue::instanceCounter.load()); // 1 instance
    ASSERT_TRUE(mgr.rtGetObject() == nullptr);
    ASSERT_EQ(46, mgr.uiGetObject()->fValue);
    ASSERT_FALSE(updated);

    ASSERT_TRUE(mgr.uiAdjustObjectFromRT(11, &updated) == nullptr); // ui adjusted to nullptr
    ASSERT_EQ(0, MyTestValue::instanceCounter.load()); // 0 instance
    ASSERT_TRUE(mgr.rtGetObject() == nullptr);
    ASSERT_TRUE(mgr.uiGetObject() == nullptr);
    ASSERT_TRUE(updated);

  }

  // no instance at end
  ASSERT_EQ(0, MyTestValue::instanceCounter.load());
}

}