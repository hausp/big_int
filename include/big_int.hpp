
#ifndef __BIG_INT_HPP__
#define __BIG_INT_HPP__

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <string>
#include <ostream>
#include <regex>
#include <tuple>

namespace hausp {
    class big_int {
        // Friend non-member operators
        friend std::ostream& operator<<(std::ostream&, const big_int&);
        friend bool operator==(const big_int&, const big_int&);
        friend bool operator<(const big_int&, const big_int&);
        // Aliases
        using Group = uint32_t;
        using DoubleGroup = uint64_t;
        using GroupVector = std::deque<Group>;
        // Constant values
        static constexpr auto GROUP_MAX = 0xffffffff;
        static constexpr auto GROUP_RADIX = 0x100000000;
        static constexpr auto GROUP_BIT_SIZE = 32;
     public:
        big_int() = default;
        template<typename T, std::enable_if_t<std::is_signed<T>::value, int> = 0>
        big_int(T);
        template<typename T, std::enable_if_t<std::is_unsigned<T>::value, int> = 0>
        big_int(T);

        static big_int from_string(const std::string&);
        big_int& operator+=(const big_int&);
        big_int& operator<<=(uintmax_t);
        big_int& operator>>=(uintmax_t);
     private:
        GroupVector data = {0};
        bool sign = false;

        std::pair<big_int, size_t> double_dabble() const;
        static GroupVector convert_base(DoubleGroup);
        static GroupVector convert_base(const std::string&);
    };

    template<typename T, std::enable_if_t<std::is_signed<T>::value, int>>
    big_int::big_int(T value):
      data{convert_base(std::abs(value))}, sign{value < 0} { }

    template<typename T, std::enable_if_t<std::is_unsigned<T>::value, int>>
    big_int::big_int(T value):
     data{convert_base(value)} { }

    inline big_int big_int::from_string(const std::string& str_value) {
        std::regex number_regex("\\s*(\\+|-)?\\s*([0-9]+)\\s*");
        std::smatch number_match;

        if (!std::regex_match(str_value, number_match, number_regex)) {
            throw std::runtime_error(
                "Could not create big_int from string: non-integer value"
            );
        }
        big_int integer;
        integer.sign = (number_match[1] == "-");
        integer.data = convert_base(number_match[2]);
        return integer;
    }

    inline big_int::GroupVector big_int::convert_base(DoubleGroup value) {
        GroupVector data;
        if (value != 0) {
            while (value > 0) {
                data.push_back(value & GROUP_MAX);
                value >>= GROUP_BIT_SIZE;
            }
        } else {
            data.push_back(0);
        }
        return data;
    }

    inline big_int::GroupVector big_int::convert_base(const std::string& str_value) {
        GroupVector data;
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
        size_t k = 0;
        while (k < data.size()) {
            for (size_t i = data.size() - 1; i > k; --i) {
                DoubleGroup true_value = data[i] * 1000000000ull;
                true_value += data[i - 1];
                data[i - 1] = true_value;
                data[i] = true_value >> (GROUP_BIT_SIZE);
            }
            while (data.back() == 0) data.pop_back();
            k++;
        }
        return data;
    }

    inline bool operator==(const big_int& lhs, const big_int& rhs) {
        if (lhs.sign != rhs.sign) {
            return false;
        } else if (lhs.data.size() == rhs.data.size()) {
            size_t i = lhs.data.size() - 1;
            while (i > 0 && lhs.data[i] == rhs.data[i]) --i;
            // std::cout << "lhs.data[i] = " << lhs.data[i] << std::endl;
            // std::cout << "rhs.data[i] = " << rhs.data[i] << std::endl;
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

    inline big_int& big_int::operator<<=(uintmax_t shift) {
        uintmax_t digit_shift = std::floor(shift / GROUP_BIT_SIZE);
        shift = shift % GROUP_BIT_SIZE;

        for (size_t i = 0; i < digit_shift; ++i) data.push_front(0);
        
        auto shift_mask = (-1) ^ ((2 << (GROUP_BIT_SIZE - shift - 1)) - 1);

        Group carried_bits = 0;
        for (auto& digit : data) {
            auto shifted_bits = (digit & shift_mask) >> (GROUP_BIT_SIZE - shift);
            digit = ((digit << shift) | carried_bits);
            carried_bits = shifted_bits;
        }

        if (carried_bits > 0) data.push_back(carried_bits);

        return *this;
    };

    inline big_int& big_int::operator>>=(uintmax_t shift) {
        uintmax_t digit_shift = std::floor(shift / GROUP_BIT_SIZE);
        shift = shift % GROUP_BIT_SIZE;

        for (size_t i = 0; i < digit_shift; ++i) {
            data.push_back(0);
            data.pop_front();
        }
        
        auto shift_mask = (1 << shift) - 1;

        Group carried_bits = 0;
        for (int i = data.size() - 1; i >= 0; --i) {
            auto shifted_bits = data[i] & shift_mask;
            data[i] = ((data[i] >> shift) | carried_bits);
            carried_bits = shifted_bits << (GROUP_BIT_SIZE - shift);
        }

        while (data.back() == 0) data.pop_back();

        return *this;
    };

    inline big_int operator<<(const big_int& lhs, uintmax_t rhs) {
        auto result = lhs;
        return result <<= rhs;
    }

    inline big_int operator>>(const big_int& lhs, uintmax_t rhs) {
        auto result = lhs;
        return result >>= rhs;
    }

    inline std::pair<big_int, size_t> big_int::double_dabble() const {
        auto bit_size = data.size() * GROUP_BIT_SIZE;
        // auto needed_space = (bit_size + 4) * std::ceil(bit_size / 3);
        // auto extra_digits = std::ceil((needed_space - bit_size) / GROUP_BIT_SIZE);
        auto nibble_masks = std::vector<Group> {
            0xf, 0xf0, 0xf00, 0xf000, 0xf0000, 0xf00000, 0xf000000, 0xf0000000
        };
        big_int reg;
        reg.data.insert(reg.data.begin(), data.begin(), data.end());
        // reg.data.insert(reg.data.end(), extra_digits, 0);
        reg.data.insert(reg.data.end(), 0);

        reg <<= 3;
        for (size_t i = 0; i < bit_size - 3; ++i) {
            for (size_t j = data.size(); j < reg.data.size(); ++j) {
                for (size_t k = 0; k < nibble_masks.size(); ++k) {
                    auto bcd_value = (reg.data[j] & nibble_masks[k]) >> k * 4;
                    if (bcd_value > 4) {
                        auto context = (nibble_masks[k] ^ GROUP_MAX) & reg.data[j];
                        bcd_value += 3;
                        bcd_value <<= k * 4;
                        reg.data[j] = context | bcd_value;
                    }
                }
            }
            reg <<= 1;
        }
        return {reg, data.size()};
    }

    template<typename... Args>
    big_int stobi(Args&&... args) {
        return big_int::from_string(std::forward<Args>(args)...);
    }

    inline std::ostream& operator<<(std::ostream& out, const big_int& number) {
        auto nibble_masks = std::vector<big_int::Group> {
            0xf, 0xf0, 0xf00, 0xf000, 0xf0000, 0xf00000, 0xf000000, 0xf0000000
        };
        
        if (number.sign) out << "-";
        big_int bcd;
        size_t first_index;
        std::tie(bcd, first_index) = number.double_dabble();
        bool zeros_only = true;
        for (size_t i = bcd.data.size() - 1; i >= first_index; --i) {
            for (int j = nibble_masks.size() - 1; j >= 0; --j) {
                auto value = ((bcd.data[i] & nibble_masks[j]) >> j * 4);
                if (zeros_only) {
                    if (value != 0) {
                        out << value;
                        zeros_only = false;
                    }
                } else {
                    out << value;
                }
            }
        }
        return out;
    }
}

#endif /* __BIG_INT_HPP__ */
