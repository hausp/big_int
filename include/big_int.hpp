
#ifndef __BIG_INT_HPP__
#define __BIG_INT_HPP__

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <ostream>
#include <tuple>
#include <vector>

namespace hausp {
    class big_int {
        // Friend non-member operators
        friend std::ostream& operator<<(std::ostream&, const big_int&);
        friend bool operator==(const big_int&, const big_int&);
        friend bool operator<(const big_int&, const big_int&);
        // Aliases
        using UNIT = uint32_t;
        using num_vector = std::vector<UNIT>;
        // Constant values
        static constexpr auto UNIT_MAX = 999999999ULL;
        static constexpr auto UNIT_RADIX = 1000000000ULL;
        static constexpr auto PRIMITIVE_MAX = UINT32_MAX;
     public:
        big_int() = default;
        template<typename T, std::enable_if_t<std::is_signed<T>::value, int> = 0>
        big_int(T);
        template<typename T, std::enable_if_t<std::is_unsigned<T>::value, int> = 0>
        big_int(T);
        big_int(const std::string&);
     private:
        num_vector data = {};
        bool sign = false;

        static num_vector convert_base(uintmax_t);
        static num_vector convert_base(const std::string&);
    };

    template<typename T, std::enable_if_t<std::is_signed<T>::value, int>>
    big_int::big_int(T value):
      data{convert_base(std::abs(value))}, sign{value < 0} { }

    template<typename T, std::enable_if_t<std::is_unsigned<T>::value, int>>
    big_int::big_int(T value):
     data{convert_base(value)} { }

    inline big_int::big_int(const std::string& str_value) {
        data = convert_base(str_value);
    }

    inline big_int::num_vector big_int::convert_base(uintmax_t value) {
        num_vector data;
        if (value > UNIT_MAX) {
            while (value > UNIT_MAX) {
                uintmax_t q = value / UNIT_RADIX;
                uintmax_t r = value % UNIT_RADIX;
                data.push_back(r);
                value = q;
            }
            if (value != 0) {
                data.push_back(value);
            }
        } else {
            data.push_back(value);
        }
        return data;
    }

    inline big_int::num_vector big_int::convert_base(const std::string& str_value) {
        num_vector data;
        auto i = str_value.size();
        while (i > 0) {
            if (i > 8) {
                i = i - 9;
                data.push_back(std::stoull(str_value.substr(i, 9)));
            } else {
                data.push_back(std::stoull(str_value.substr(0, i)));
                i = 0;
            }
        }
        return data;
    }

    inline bool operator==(const big_int& lhs, const big_int& rhs) {
        if (lhs.sign != rhs.sign) {
            return false;
        } else if (lhs.data.size() == rhs.data.size()) {
            size_t i = lhs.data.size() - 1;
            while (i > 0 && lhs.data[i] == rhs.data[i]) --i;
            return i == 0 && lhs.data[i] == rhs.data[i];
        }
        return false;
    }

    inline bool operator!=(const big_int& lhs, const big_int rhs) {
        return !(lhs == rhs);
    }

    inline bool operator<(const big_int& lhs, const big_int& rhs) {
        if (lhs.sign != rhs.sign) {
            return lhs.sign;
        } else if (lhs.data.size() == rhs.data.size()) {
            size_t i = lhs.data.size() - 1;
            while (i > 0 && lhs.data[i] == rhs.data[i]) --i;
            return lhs.data[i] < rhs.data[i];
        }
        return lhs.data.size() < rhs.data.size();
    }

    inline bool operator>(const big_int& lhs, const big_int& rhs) {
        return rhs < lhs;
    }

    inline bool operator<=(const big_int& lhs, const big_int& rhs) {
        return !(lhs > rhs);
    }

    inline bool operator>=(const big_int& lhs, const big_int& rhs) {
        return !(lhs < rhs);
    }

    inline std::ostream& operator<<(std::ostream& out, const big_int& number) {
        if (number.sign) out << "-";
        auto last = number.data.size() - 1;
        for (size_t i = last + 1; i > 0; --i) {
            auto index = i - 1;
            auto fragment = std::to_string(number.data[index]);
            if (index != last) {
                while (fragment.size() < 9) {
                    fragment.insert(0, 1, '0');
                }
            }
            out << fragment;
        }
        return out;
    }
}

#endif /* __BIG_INT_HPP__ */
