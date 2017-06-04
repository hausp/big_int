#include <gtest/gtest.h>
#include <sstream>
#include <unordered_map>
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
        for (size_t i = 0; i < 150; i++) {
            ss << "123456781234567812345678";
        }
        numbers.push_back(ss.str());
        prepared = true;
    }

    return numbers;
}

inline std::string repeat(size_t v, size_t n) {
    return std::string(n, v + '0');
}

inline big_int fs(const std::string& s) {
    return big_int::from_string(s);
}

TEST_F(Tests, SimpleConstruction) {
    const auto& numbers = sampleNumbers();
    std::stringstream ss;
    for (auto& number : numbers) {
        ss << fs(number);
        ASSERT_EQ(ss.str(), number);
        ss.str("");
    }
}

TEST_F(Tests, ExplicitSignConstruction) {
    const auto& numbers = sampleNumbers();
    std::stringstream ss;
    for (auto& number : numbers) {
        auto n = "-" + number;
        ss << fs(n);
        ASSERT_EQ(ss.str(), n);
        ss.str("");

        ss << fs("+" + number);
        ASSERT_EQ(ss.str(), number);
        ss.str("");
    }
}

TEST_F(Tests, Inequalities) {
    ASSERT_TRUE(
        fs("8423982138934987132893497547132978423978132") ==
        fs("8423982138934987132893497547132978423978132")
    );

    ASSERT_TRUE(
        fs("8423982138934987132893497547132978423978132") !=
        fs("8423982138934987132893497547132978423978131")
    );

    ASSERT_TRUE(
        fs("8423982138934987132893497547132978423978132") !=
        fs("2349547891279342674589009129045978120945789")
    );

    ASSERT_TRUE(
        fs("8423982138934987132893497547132978423978132") !=
        fs("234954789127934229045978120945789")
    );

    ASSERT_TRUE(big_int(-42) == big_int(-42));
    ASSERT_TRUE(big_int(-42) < big_int(42));
    ASSERT_TRUE(big_int(42) > big_int(-42));

    ASSERT_TRUE(big_int() == big_int(0));
    ASSERT_TRUE(big_int(0) <= big_int(0));
    ASSERT_TRUE(big_int(0) >= big_int(0));
    ASSERT_TRUE(big_int(-1) < big_int(0));
    ASSERT_TRUE(big_int(0) > big_int(-1));

    auto n1 = fs("1323089548042380213098650892138790");
    auto n2 = fs("2109428218005820520572960106810672");
    auto n3 = fs("132308954804238021309865089213879450");
    ASSERT_TRUE(n1 < n2);
    ASSERT_TRUE(n2 < n3);
    ASSERT_TRUE(n1 < n3);
    ASSERT_TRUE(n2 > n1);
    ASSERT_TRUE(n3 > n2);
    ASSERT_TRUE(n3 > n1);
}

TEST_F(Tests, Shifts) {
    std::unordered_map<size_t, big_int> powers = {
        {1, 2},
        {2, 4},
        {3, 8},
        {4, 16},
        {5, 32},
        {100, fs("1267650600228229401496703205376")},
        {150, fs("1427247692705959881058285969449495136382746624")},
        {200, fs("1606938044258990275541962092341162602522202993782792835301376")}
    };

    auto n = big_int(1);
    for (size_t i = 1; i <= 200; i++) {
        n <<= 1;
        if (powers.count(i) > 0) {
            ASSERT_EQ(n, powers[i]);
        }
    }

    for (size_t i = 199; i > 0; i--) {
        n >>= 1;
        if (powers.count(i) > 0) {
            ASSERT_EQ(n, powers[i]);
        }
    }

    auto a = fs("46819283774865195682109389689290389645910928367102845391784");
    auto b = fs("365775654491134341266479606947581169108679127867990979623");
    auto c = fs("46819283774865195682109389689290389645910928367102845391744");
    ASSERT_EQ(a >> 7, b);
    ASSERT_EQ(b << 7, c);
    ASSERT_EQ(a >> 2 >> 3 >> 1 >> 1, b);
    ASSERT_EQ(b << 3 << 4, c);
    ASSERT_EQ(a >> 2 >> 2 >> 1 >> 2, b);
    ASSERT_EQ(b << 2 << 2 << 1 << 2, c);
}

// TEST_F(Tests, AddAndSub) {
//     auto n1 = fs(repeat(1, 1000));
//     auto n2 = fs(repeat(2, 1000));
//     auto n3 = fs(repeat(3, 1000));

//     ASSERT_EQ(n1 + n1, n2);
//     ASSERT_EQ(n1 + n2, n3);
//     ASSERT_EQ(n3 - n1, n2);
//     ASSERT_EQ(n3 - n2, n1);
//     ASSERT_EQ(n1 - n2, -n1);
//     ASSERT_EQ(n1 + n1 + n1, n3);

//     ASSERT_EQ(
//         fs("1" + repeat(0, 100)) - fs(repeat(9, 100)),
//         big_int(1)
//     );

//     ASSERT_EQ(
//         fs(repeat(9, 100)) - fs("1" + repeat(0, 100)),
//         big_int(-1)
//     );

//     ASSERT_EQ(
//         fs(repeat(4, 123)) + fs(repeat(5, 123)) + fs(repeat(1, 123)),
//         fs(repeat(1, 123) + "0")
//     );
// }

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
