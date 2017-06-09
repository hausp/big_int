
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

#include "debug.hpp"

namespace hausp {
    class BigInt {
        // Friend non-member operators
        friend std::ostream& operator<<(std::ostream&, const BigInt&);
        friend bool operator==(const BigInt&, const BigInt&);
        friend bool operator<(const BigInt&, const BigInt&);
        // Aliases
        using Group = uint32_t;
        using SignedGroup = int64_t;
        using DoubleGroup = uint64_t;
        using GroupVector = std::deque<Group>;
        // Constant values
        static constexpr auto GROUP_MAX = 0xffffffff;
        static constexpr auto GROUP_RADIX = 0x100000000;
        static constexpr auto GROUP_BIT_SIZE = 32;
        static constexpr auto POSITIVE = false;
        static constexpr auto NEGATIVE = true;
     public:
        BigInt() = default;
        template<typename T, std::enable_if_t<std::is_unsigned<T>::value, int> = 0>
        BigInt(T);
        template<typename T, std::enable_if_t<std::is_signed<T>::value, int> = 0>
        BigInt(T);

        static BigInt fromString(const std::string&);
        BigInt& operator+=(const BigInt&);
        BigInt& operator-=(const BigInt&);
        BigInt operator-() const;
        BigInt& operator*=(const BigInt&);
        BigInt& operator<<=(intmax_t);
        BigInt& operator>>=(intmax_t);
     private:
        bool signal = POSITIVE;
        GroupVector data = {0};

        template<typename Operation>
        BigInt& carryOn(const BigInt&, bool, Operation);
        BigInt& longMult(const BigInt&);
        void shrink();

        GroupVector toDecimal() const;

        static GroupVector convertBase(uintmax_t);
        static GroupVector convertBase(const std::string&);
        static void twoComplement(GroupVector&, Group);
    };

    template<typename T, std::enable_if_t<std::is_unsigned<T>::value, int>>
    BigInt::BigInt(T value): data{convertBase(value)} { }

    template<typename T, std::enable_if_t<std::is_signed<T>::value, int>>
    BigInt::BigInt(T value):
     signal{value < 0}, data{convertBase(std::abs(value))} { }

    inline BigInt BigInt::fromString(const std::string& str_value) {
        std::regex number_regex("\\s*(\\+|-)?\\s*([0-9]+)\\s*");
        std::smatch number_match;
        if (!std::regex_match(str_value, number_match, number_regex)) {
            throw std::runtime_error(
                "Could not create BigInt from string: non-integer value"
            );
        }
        BigInt integer;
        integer.data = convertBase(number_match[2]);
        integer.signal = number_match[1] == "-";
        integer.shrink();
        return integer;
    }

    inline BigInt::GroupVector BigInt::convertBase(uintmax_t value) {
        GroupVector data;
        if (value != 0) {
            while (value > 0) {
                data.emplace_back(value & GROUP_MAX);
                value >>= GROUP_BIT_SIZE;
            }
        } else {
            data.emplace_back(0);
        }
        return data;
    }

    inline BigInt::GroupVector BigInt::convertBase(const std::string& str_value) {
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

    inline BigInt::GroupVector BigInt::toDecimal() const {
        auto dec_data = data;
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

    void BigInt::twoComplement(GroupVector& data, Group signal) {
        DoubleGroup carry = 1;
        for (auto& segment : data) {
            DoubleGroup complement = ~segment + carry;
            segment = complement;
            carry = complement >> GROUP_BIT_SIZE;
        }
        signal = ~signal + carry;
        if (signal == 1) {
            data.emplace_back(signal);
            signal = 0;
        }
    }

    inline void BigInt::shrink() {
        while (data.back() == 0 && data.size() > 1) {
            data.pop_back();
        }
    }

    template<typename Operation>
    BigInt& BigInt::carryOn(const BigInt& rhs, bool cmpl, Operation op) {
        if (data.size() < rhs.data.size()) {
            data.insert(data.end(), rhs.data.size() - data.size(), 0);
        }
        DoubleGroup carry = cmpl; // Uses DoubleGroup to coerce operation
        size_t i = 0;
        for (i = 0; i < rhs.data.size(); ++i) {
            DoubleGroup result = op(data[i], rhs.data[i], carry);
            data[i] = result;
            carry = (result >> GROUP_BIT_SIZE) > 0;
        }
        while (i < data.size() && carry != 0) {
            DoubleGroup result = op(data[i], 0, carry);
            data[i] = result;
            carry = (result >> GROUP_BIT_SIZE) > 0;
            ++i;
        }

        if (cmpl) {
            auto signal_update = op(0, 0, carry);
            auto carry_out = (signal_update >> GROUP_BIT_SIZE) > 0;
            Group value = signal_update;
            if (carry_out ^ carry) {
                data.emplace_back(carry);
            }
            if (value == GROUP_MAX) {
                twoComplement(data, value);
                signal = NEGATIVE;
            } else if (value == 0) {
                signal = POSITIVE;
            }
        } else if (carry > 0) {
            data.push_back(carry);
        }
        shrink(); // Is it worth?
        return *this;
    }

    inline BigInt& BigInt::operator+=(const BigInt& rhs) {
        if (signal == rhs.signal) {
            return carryOn(rhs, false,
                [](Group lhs, Group rhs, DoubleGroup carry) {
                    return carry + lhs + rhs;
                }
            );
        } else if (signal && !rhs.signal) {
            return carryOn(rhs, true,
                [](Group lhs, Group rhs, DoubleGroup carry) {
                    return carry + ~lhs + rhs;
                }
            );
        } else {
            return carryOn(rhs, true,
                [](Group lhs, Group rhs, DoubleGroup carry) {
                    auto result = carry + lhs + ~rhs;
                    return result;
                }
            );
        }
    }

    inline BigInt& BigInt::operator-=(const BigInt& rhs) {
        if (signal == rhs.signal) {
            return carryOn(rhs, true,
                [](Group lhs, Group rhs, DoubleGroup carry) {
                    return carry + lhs + ~rhs;
                }
            );
        } else {
            return carryOn(rhs, false,
                [](Group lhs, Group rhs, DoubleGroup carry) {
                    return carry + lhs + rhs;
                }
            );
        }
    }

    inline BigInt BigInt::operator-() const {
        BigInt result = *this;
        result.signal = !signal;
        return result;
    }

    inline BigInt operator+(const BigInt& lhs, const BigInt& rhs) {
        auto copy = lhs;
        return copy += rhs;
    }

    inline BigInt operator-(const BigInt& lhs, const BigInt& rhs) {
        auto copy = lhs;
        return copy -= rhs;
    }

    BigInt& BigInt::longMult(const BigInt& rhs) {
        auto product = GroupVector(data.size() + rhs.data.size() + 1, 0);
        for (size_t i = 0; i < rhs.data.size(); ++i) {
            DoubleGroup carry = 0;
            for (size_t j = 0; j < data.size(); ++j) {
                DoubleGroup result = rhs.data[i];
                result = product[i + j] + result * data[j] + carry;
                product[i + j] = result;
                carry = result >> GROUP_BIT_SIZE;
            }
            product[data.size() + i] += carry;
        }
        signal = signal != rhs.signal;
        data = product;
        shrink();
        return *this;
    }

    inline BigInt& BigInt::operator*=(const BigInt& rhs) {
        return longMult(rhs);
    }

    inline BigInt operator*(const BigInt& lhs, const BigInt& rhs) {
        auto copy = lhs;
        return copy *= rhs;
    }

    inline BigInt& BigInt::operator<<=(intmax_t shift) {
        if (shift < 0) {
            return (*this) >>= std::abs(shift);
        }
        
        uintmax_t group_shift = std::floor(shift / GROUP_BIT_SIZE);
        data.insert(data.cbegin(), group_shift, 0);
        shift = shift % GROUP_BIT_SIZE;        
        auto shift_mask = ~((2 << (GROUP_BIT_SIZE - shift - 1)) - 1);
        Group carried_bits = 0;

        for (auto& digit : data) {
            auto shifted_bits = (digit & shift_mask) >> (GROUP_BIT_SIZE - shift);
            digit = (digit << shift) | carried_bits;
            carried_bits = shifted_bits;
        }

        if (carried_bits > 0) {
            data.emplace_back(carried_bits);
        }

        shrink(); // Is it worth?
        return *this;
    };

    inline BigInt& BigInt::operator>>=(intmax_t shift) {
        if (shift < 0) {
            return (*this) <<= std::abs(shift);
        }
        
        uintmax_t group_shift = std::floor(shift / GROUP_BIT_SIZE);
        shift = shift % GROUP_BIT_SIZE;
        auto shift_mask = (1 << shift) - 1;

        if (group_shift > data.size()) {
            data.clear();
            data.emplace_back(signal);
            return *this;
        }

        for (size_t i = 0; i < group_shift; ++i) {
            data.pop_front();
        }

        // Group carried_bits = signal.value << (GROUP_BIT_SIZE - shift);
        Group carried_bits = 0;
        for (intmax_t i = data.size() - 1; i >= 0; --i) {
            auto shifted_bits = data[i] & shift_mask;
            data[i] = (data[i] >> shift) | carried_bits;
            carried_bits = shifted_bits << (GROUP_BIT_SIZE - shift);
        }

        shrink(); // Is it worth?

        if (signal && data.size() == 1 && data.back() == 0) {
            data.back() = 1;
        }

        return *this;
    };

    inline BigInt operator<<(const BigInt& lhs, uintmax_t rhs) {
        auto result = lhs;
        return result <<= rhs;
    }

    inline BigInt operator>>(const BigInt& lhs, uintmax_t rhs) {
        auto result = lhs;
        return result >>= rhs;
    }

    inline bool operator==(const BigInt& lhs, const BigInt& rhs) {
        if (lhs.signal != rhs.signal) {
            return false;
        } else if (lhs.data.size() == rhs.data.size()) {
            size_t i = lhs.data.size() - 1;
            while (i > 0 && lhs.data[i] == rhs.data[i]) --i;
            return i == 0 && lhs.data[i] == rhs.data[i];
        }
        return false;
    }

    inline bool operator!=(const BigInt& lhs, const BigInt rhs) {
        return !(lhs == rhs);
    }

    inline bool operator<(const BigInt& lhs, const BigInt& rhs) {
        if (lhs.signal != rhs.signal) {
            return lhs.signal;
        } else if (lhs.data.size() == rhs.data.size()) {
            size_t i = lhs.data.size() - 1;
            while (i > 0 && lhs.data[i] == rhs.data[i]) --i;
            return lhs.data[i] < rhs.data[i];
        }
        return lhs.data.size() < rhs.data.size();
    }

    inline bool operator>(const BigInt& lhs, const BigInt& rhs) {
        return rhs < lhs;
    }

    inline bool operator<=(const BigInt& lhs, const BigInt& rhs) {
        return !(lhs > rhs);
    }

    inline bool operator>=(const BigInt& lhs, const BigInt& rhs) {
        return !(lhs < rhs);
    }

    template<typename... Args>
    BigInt stobi(Args&&... args) {
        return BigInt::fromString(std::forward<Args>(args)...);
    }

    inline std::ostream& operator<<(std::ostream& out, const BigInt& number) {
        auto dec_data = number.toDecimal();
        if (number.signal == BigInt::NEGATIVE) out << "-";
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
