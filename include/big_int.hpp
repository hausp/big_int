
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
        using SignedGroup = int64_t;
        using DoubleGroup = uint64_t;
        using GroupVector = std::deque<Group>;
        // Constant values
        static constexpr auto GROUP_MAX = 0xffffffff;
        static constexpr auto GROUP_RADIX = 0x100000000;
        static constexpr auto GROUP_BIT_SIZE = 32;
     public:
        big_int() = default;
        template<typename T, std::enable_if_t<std::is_integral<T>::value, int> = 0>
        big_int(T);

        static big_int from_string(const std::string&);
        big_int& operator+=(const big_int&);
        big_int& operator-=(const big_int&);
        big_int operator-() const;
        big_int& operator<<=(uintmax_t);
        big_int& operator>>=(uintmax_t);
     private:
        bool negative = false;
        GroupVector data = {0};

        GroupVector to_decimal() const;
        static GroupVector convert_base(intmax_t);
        static GroupVector convert_base(const std::string&);
        static void two_complement(GroupVector&);
    };

    template<typename T, std::enable_if_t<std::is_integral<T>::value, int>>
    big_int::big_int(T value):
     negative{value < 0}, data{convert_base(value)} { }

    inline big_int big_int::from_string(const std::string& str_value) {
        std::regex number_regex("\\s*(\\+|-)?\\s*([0-9]+)\\s*");
        std::smatch number_match;

        if (!std::regex_match(str_value, number_match, number_regex)) {
            throw std::runtime_error(
                "Could not create big_int from string: non-integer value"
            );
        }
        big_int integer;
        integer.negative = (number_match[1] == "-");
        integer.data = convert_base(number_match[2]);
        if (integer.negative) {
            two_complement(integer.data);
        }
        return integer;
    }

    inline big_int::GroupVector big_int::convert_base(intmax_t value) {
        GroupVector data;
        auto negative = value < 0;
        value = std::abs(value);
        
        if (value != 0) {
            while (value > 0) {
                data.emplace_back(value & GROUP_MAX);
                value >>= GROUP_BIT_SIZE;
            }
        } else {
            data.emplace_back(0);
        }
        
        if (negative) {
            two_complement(data);
        }
        
        return data;
    }

    inline big_int::GroupVector big_int::convert_base(const std::string& str_value) {
        GroupVector data;
        auto i = str_value.size();
        while (i > 0) {
            if (i > 8) {
                i = i - 9;
                data.emplace_back(std::stoull(str_value.substr(i, 9)));
            } else {
                data.emplace_back(std::stoull(str_value.substr(0, i)));
                i = 0;
            }
        }
        size_t k = 0;
        while (k < data.size()) {
            for (size_t i = data.size() - 1; i > k; --i) {
                DoubleGroup true_value = data[i] * 1000000000ull + data[i - 1];
                data[i - 1] = true_value;
                data[i] = true_value >> GROUP_BIT_SIZE;
            }
            while (data.back() == 0 && data.size() > 1) data.pop_back();
            k++;
        }
        return data;
    }

    inline big_int::GroupVector big_int::to_decimal() const {
        GroupVector dec_data = data;
        if (negative) two_complement(dec_data);
        
        size_t k = 0;
        while (k < dec_data.size()) {
            for (size_t i = dec_data.size() - 1; i > k; --i) {
                DoubleGroup true_value = dec_data[i] * GROUP_RADIX;
                true_value += dec_data[i - 1];
                dec_data[i - 1] = true_value % 1000000000ull;
                DoubleGroup q = true_value / 1000000000ull;
                if (q > GROUP_RADIX) {
                    if (i < dec_data.size() - 1) {
                        DoubleGroup value = (q / GROUP_RADIX);
                        dec_data[i + 1] += value;
                    } else {
                        dec_data.emplace_back(q / GROUP_RADIX);
                    }
                    q = q % GROUP_RADIX;
                }
                dec_data[i] = q;
            }
            k++;
        }

        if (dec_data.back() > 1000000000ull) {
            auto q = dec_data.back() / 1000000000ull;
            dec_data.back() = dec_data.back() % 1000000000ull;
            dec_data.push_back(q);
        }

        return dec_data;
    }

    inline void big_int::two_complement(GroupVector& data) {
        DoubleGroup carry = 1;
        for (auto& group : data) {
            DoubleGroup complement = ~group + carry;
            group = complement;
            carry = complement >> GROUP_BIT_SIZE;
        }
        
        if (carry > 0) {
            data.emplace_back(GROUP_MAX);
        } else {
            while (data.back() == 0 && data.size() > 1) data.pop_back();
        }
    }

    inline bool operator==(const big_int& lhs, const big_int& rhs) {
        if (lhs.negative != rhs.negative) {
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
        if (lhs.negative != rhs.negative) {
            return lhs.negative;
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

    inline big_int& big_int::operator+=(const big_int& rhs) {
        size_t size = data.size();
        Group last = data.back();
        DoubleGroup carry = 0;
        auto min = std::min(data.size(), rhs.data.size());
        
        for (size_t i = 0; i < min; ++i) {
            DoubleGroup result = data[i] + carry + rhs.data[i];
            data[i] = result;
            carry = result >> GROUP_BIT_SIZE;
        }
        
        size_t i = min;
        if (data.size() >= rhs.data.size()) {
            while (i < data.size() && carry != 0) {
                DoubleGroup result = data[i] + carry;
                data[i] = result;
                carry = result >> GROUP_BIT_SIZE;
                i++;
            }
        } else {
            while (i < rhs.data.size() && carry != 0) {
                DoubleGroup result = rhs.data[i] + carry;
                data.emplace_back(result);
                carry = result >> GROUP_BIT_SIZE;
                i++;
            }
        }
        
        if (negative ^ rhs.negative) {
            negative = size < data.size() || last < data.back();
        } else if (carry > 0 && !negative) {
            data.emplace_back(carry);
        }
        
        auto comparison = negative ? GROUP_MAX : 0;
        while (data.back() == comparison && data.size() > 1) data.pop_back();
        return *this;
    }

    inline big_int& big_int::operator-=(const big_int& rhs) {
        return (*this) += (-rhs);
    }

    inline big_int big_int::operator-() const {
        big_int result;
        result.negative = !negative;
        result.data = data;
        two_complement(result.data);
        return result;
    }

    inline big_int operator+(const big_int& lhs, const big_int& rhs) {
        auto copy = lhs;
        return copy += rhs;
    }

    inline big_int operator-(const big_int& lhs, const big_int& rhs) {
        auto copy = lhs;
        return copy -= rhs;
    }

    inline big_int& big_int::operator<<=(uintmax_t shift) {
        uintmax_t digit_shift = std::floor(shift / GROUP_BIT_SIZE);
        shift = shift % GROUP_BIT_SIZE;

        for (size_t i = 0; i < digit_shift; ++i) data.emplace_front(0);
        
        auto shift_mask = ~((2 << (GROUP_BIT_SIZE - shift - 1)) - 1);

        Group carried_bits = 0;
        for (auto& digit : data) {
            auto shifted_bits = (digit & shift_mask) >> (GROUP_BIT_SIZE - shift);
            digit = (digit << shift) | carried_bits;
            carried_bits = shifted_bits;
        }

        if (carried_bits > 0) {
            if (negative) {
                size_t bit_position = std::floor(std::log2(carried_bits));
                auto mask = (bit_position << 2) - 1;
                auto validation = carried_bits ^ mask;
                if (validation) {
                    data.emplace_back(carried_bits | ~validation);
                } else {
                    while (data.back() == GROUP_MAX && data.size() > 1) {
                        data.pop_back();
                    }
                }
            } else {
                data.emplace_back(carried_bits);
            }
        }

        while (data.back() == 0 && data.size() > 1) data.pop_back();

        return *this;
    };

    inline big_int& big_int::operator>>=(uintmax_t shift) {
        uintmax_t digit_shift = std::floor(shift / GROUP_BIT_SIZE);
        auto feed = negative ? GROUP_MAX : 0;
        shift = shift % GROUP_BIT_SIZE;

        for (size_t i = 0; i < digit_shift; ++i) {
            data.emplace_back(feed);
            data.pop_front();
        }
        
        auto shift_mask = (1 << shift) - 1;
        Group carried_bits = 0;

        for (int i = data.size() - 1; i >= 0; --i) {
            auto shifted_bits = data[i] & shift_mask;
            data[i] = (data[i] >> shift) | carried_bits;
            carried_bits = shifted_bits << (GROUP_BIT_SIZE - shift);
        }

        if (negative) {
            size_t bit_position = std::floor(std::log2(data.back()));
            auto mask = (bit_position << 2) - 1;
            auto expansion = ~(data.back() ^ mask);
            data.back() |= expansion;
        }

        while (data.back() == feed && data.size() > 1) data.pop_back();

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

    template<typename... Args>
    big_int stobi(Args&&... args) {
        return big_int::from_string(std::forward<Args>(args)...);
    }

    inline std::ostream& operator<<(std::ostream& out, const big_int& number) {
        auto dec_data = number.to_decimal();
        if (number.negative) out << "-";
        auto last = dec_data.size() - 1;
        for (size_t i = last + 1; i > 0; --i) {
            auto index = i - 1;
            auto fragment = std::to_string(dec_data[index]);
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
