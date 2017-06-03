#include <gtest/gtest.h>
#include <sstream>
#include "big_int.hpp"

class Tests : public ::testing::Test {};

using hausp::big_int;

const std::vector<std::string>& sampleNumbers() {
    static bool prepared = false;
    static std::vector<std::string> numbers = {
        "42",
        "123456781234567812345678"
    };

    if (!prepared) {
        std::stringstream ss;
        for (size_t i = 0; i < 300; i++) {
            ss << "123456781234567812345678";
        }
        numbers.push_back(ss.str());
        prepared = true;
    }

    return numbers;
}

TEST_F(Tests, SimpleConstruction) {
    const auto& numbers = sampleNumbers();
    std::stringstream ss;
    for (auto& number : numbers) {
        ss << big_int(number);
        ASSERT_EQ(ss.str(), number);
        ss.str("");
    }
}

TEST_F(Tests, ExplicitSignConstruction) {
    const auto& numbers = sampleNumbers();
    std::stringstream ss;
    for (auto& number : numbers) {
        auto n = "-" + number;
        ss << big_int(n);
        ASSERT_EQ(ss.str(), n);
        ss.str("");

        ss << big_int("+" + number);
        ASSERT_EQ(ss.str(), number);
        ss.str("");
    }
}

TEST_F(Tests, Inequalities) {
    ASSERT_TRUE(
        big_int("8423982138934987132893497547132978423978132") ==
        big_int("8423982138934987132893497547132978423978132")
    );

    ASSERT_TRUE(
        big_int("8423982138934987132893497547132978423978132") !=
        big_int("8423982138934987132893497547132978423978131")
    );

    ASSERT_TRUE(
        big_int("8423982138934987132893497547132978423978132") !=
        big_int("2349547891279342674589009129045978120945789")
    );

    ASSERT_TRUE(
        big_int("8423982138934987132893497547132978423978132") !=
        big_int("234954789127934229045978120945789")
    );

    ASSERT_TRUE(big_int(-42) == big_int(-42));
    ASSERT_TRUE(big_int(-42) < big_int(42));
    ASSERT_TRUE(big_int(42) > big_int(-42));

    // ASSERT_TRUE(big_int() == big_int(0));
    ASSERT_TRUE(big_int(0) <= big_int(0));
    ASSERT_TRUE(big_int(0) >= big_int(0));
    ASSERT_TRUE(big_int(-1) < big_int(0));
    ASSERT_TRUE(big_int(0) > big_int(-1));

    auto n1 = big_int("1323089548042380213098650892138790");
    auto n2 = big_int("2109428218005820520572960106810672");
    auto n3 = big_int("132308954804238021309865089213879450");
    ASSERT_TRUE(n1 < n2);
    ASSERT_TRUE(n2 < n3);
    ASSERT_TRUE(n1 < n3);
    ASSERT_TRUE(n2 > n1);
    ASSERT_TRUE(n3 > n2);
    ASSERT_TRUE(n3 > n1);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
