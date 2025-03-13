#ifndef extra_math_h
#define extra_math_h

#include <cassert>
#include <cstdint>
#include <cmath>
#include <cstddef>
#include <ostream>
#include <vector>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef ptrdiff_t isize;
typedef float f32;
typedef double f64;

template <typename T> T max(T x, T y) { return x < y ? y : x; }
template <typename T> T min(T x, T y) { return x > y ? y : x; }

template <typename T>
inline constexpr T sqr(T v) { return v * v; }

template <typename T>
inline T FMA(T a, T b, T c) { return a * b + c; }

template <typename Ta, typename Tb, typename Tc, typename Td>
inline auto sumOfProducts(Ta a, Tb b, Tc c, Td d) {
    auto cd = c * d;
    auto sumOfProducts = FMA(a, b, cd);
    auto error = FMA(c, d, -cd);
    return sumOfProducts + error;
    }

template <typename T> struct TupleLength { using type = float; };
template <> struct TupleLength<double> { using type = double; };
template <> struct TupleLength<long double> { using type = long double; };
template <template <typename> class Child, typename T>
class Tuple2 {
public:
    static const int n_dimensions = 2;
    Tuple2() = default;
    Tuple2(T x, T y) : x(x), y(y) {}

    bool hasNaN() { return std::isnan(x) || std::isnan(y); }

    T &operator[](int i) {
        assert(i >= 0 && i <= 1);
        return (i == 0) ? x : y;
    }

    T operator[](int i) const {
        assert(i >= 0 && i <= 1);
        return (i == 0) ? x : y;
    }

    Child<T> operator-() const { return {-x, -y}; }

    template <typename U>
    auto operator+(Child<U> c) const -> Child<decltype(T{} + U{})> {
        return {x + c.x, y + c.y};
    }

    template <typename U>
    Child<T> &operator+=(Child<U> c) {
        x += c.x; y += c.y;
        return static_cast<Child<T> &>(*this);
    }

    template <typename U>
    auto operator-(Child<U> c) const -> Child<decltype(T{} - U{})> {
        return {x - c.x, y - c.y};
    }

    template <typename U>
    Child<T> &operator-=(Child<U> c) {
        x -= c.x; y -= c.y;
        return static_cast<Child<T> &>(*this);
    }

    template <typename U>
    auto operator*(U s) const -> Child<decltype(T{} * U{})> {
        return {x * s, y * s};
    }

    template <typename U>
    Child<T> &operator*=(U s) {
        x *= s; y *= s;
        return static_cast<Child<T> &>(*this);
    }

    template <typename U>
    auto operator/(U d) const -> Child<decltype(T{} / U{})> {
        assert(d != 0 && !std::isnan(d));
        return {x / d, y / d};
    }

    template <typename U>
    Child<T> &operator/=(U d) {
        assert(d != 0 && !std::isnan(d));
        x /= d;
        y /= d;
        return static_cast<Child<T> &>(*this);
    }
    bool operator==(Child<T> c) const { return x == c.x && y == c.y; }
    bool operator!=(Child<T> c) const { return x != c.x || y != c.y; }
    T x{}, y{};
};

template <template <class> class C, typename T, typename U>
inline auto operator*(U s, Tuple2<C, T> t) -> C<decltype(T{} * U{})> {
    assert(!t.hasNaN());
    return t * s;
}

template <template <class> class C, typename T, typename U>
inline C<T> abs(Tuple2<C, T> t) {
    using std::abs;
    return { abs(t.x), abs(t.y) };
}

template <template <class> class C, typename T, typename U>
inline C<T> ceil(Tuple2<C, T> t) {
    using std::ceil;
    return { ceil(t.x), ceil(t.y) };
}

template <template <class> class C, typename T, typename U>
inline C<T> floor(Tuple2<C, T> t) {
    using std::floor;
    return { floor(t.x), floor(t.y) };
}

template <template <class> class C, typename T, typename U>
inline C<T> lerp(T t, Tuple2<C, T> t0, Tuple2<C, T> t1) {
    assert(std::abs(t) <= 1);
    return (1 - t) * t0 + t * t1;
}

template <template <typename> class Child, typename T>
class Tuple3 {
public:
    static const int n_dimensions = 3;
    Tuple3() = default;
    Tuple3(T x, T y, T z) : x(x), y(y), z(z) {}

    bool hasNaN() {  return std::isnan(x) || std::isnan(y) || std::isnan(z);  }

    T &operator[](int i) {
        assert(i >= 0 && i <= 2);
        switch (i) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
        }
    }

    T operator[](int i) const {
        assert(i >= 0 && i <= 2);
        switch (i) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
        }
    }

    Child<T> operator-() const { return {-x, -y, -z}; }

    template <typename U>
    auto operator+(Child<U> c) const -> Child<decltype(T{} + U{})> {
        return {x + c.x, y + c.y, z + c.z };
    }

    template <typename U>
    Child<T> &operator+=(Child<U> c) {
        x += c.x; y += c.y; z += c.z;
        return static_cast<Child<T> &>(*this);
    }

    template <typename U>
    auto operator-(Child<U> c) const -> Child<decltype(T{} - U{})> {
        return {x - c.x, y - c.y, z - c.z};
    }

    template <typename U>
    Child<T> &operator-=(Child<U> c) {
        x -= c.x; y -= c.y; z -= c.z;
        return static_cast<Child<T> &>(*this);
    }

    template <typename U>
    auto operator*(U s) const -> Child<decltype(T{} * U{})> {
        return {x * s, y * s, z * s};
    }

    template <typename U>
    Child<T> &operator*=(U s) {
        x *= s; y *= s; z *= s;
        return static_cast<Child<T> &>(*this);
    }

    template <typename U>
    auto operator/(U d) const -> Child<decltype(T{} / U{})> {
        assert(d != 0 && !std::isnan(d));
        return {x / d, y / d, z / d};
    }

    template <typename U>
    Child<T> &operator/=(U d) {
        assert(d != 0 && !std::isnan(d));
        x /= d; y /= d; z /= d;
        return static_cast<Child<T> &>(*this);
    }
    bool operator==(Child<T> c) const { return x == c.x && y == c.y && z == c.z; }
    bool operator!=(Child<T> c) const { return x != c.x || y != c.y || z != c.z; }
    T x{}, y{}, z{};
};
template <template <class> class C, typename T, typename U>
inline auto operator*(U s, Tuple3<C, T> t) -> C<decltype(T{} * U{})> {
    assert(!t.hasNaN());
    return t * s;
}
template <template <class> class C, typename T, typename U>
inline C<T> abs(Tuple3<C, T> t) {
    using std::abs;
    return { abs(t.x), abs(t.y), abs(t.z) };
}
template <template <class> class C, typename T, typename U>
inline C<T> ceil(Tuple3<C, T> t) {
    using std::ceil;
    return { ceil(t.x), ceil(t.y), ceil(t.z) };
}
template <template <class> class C, typename T, typename U>
inline C<T> floor(Tuple3<C, T> t) {
    using std::floor;
    return { floor(t.x), floor(t.y), floor(t.z) };
}
template <template <class> class C, typename T, typename U>
inline C<T> lerp(T t, Tuple3<C, T> t0, Tuple3<C, T> t1) {
    assert(std::abs(t) <= 1);
    return (1 - t) * t0 + t * t1;
}

// SPECIALIZATIONS

template<typename T> class Point2;

template <typename T>
class Vec2 : public Tuple2<Vec2, T> {
public:
    using Tuple2<Vec2, T>::x;
    using Tuple2<Vec2, T>::y;
    Vec2() = default;
    Vec2(T x, T y) : Tuple2<Vec2, T>(x, y) {}
    template <typename U>
    explicit Vec2(Vec2<U> u) : Tuple2<Vec2, T>(T(u.x), T(u.y)) {}
    template <typename U>
    explicit Vec2(Point2<U> u); // : Tuple2<Vec2, T>(T(u.x), T(u.y)) {}
};

using Vec2f = Vec2<float>;
using Vec2i = Vec2<i32>;

template <typename T>
class Point2 : public Tuple2<Point2, T> {
public:
    using Tuple2<Point2, T>::x;
    using Tuple2<Point2, T>::y;
    using Tuple2<Point2, T>::hasNaN;
    using Tuple2<Point2, T>::operator+;
    using Tuple2<Point2, T>::operator+=;
    using Tuple2<Point2, T>::operator*;
    using Tuple2<Point2, T>::operator*=;

    Point2() {x = y = 0; }
    Point2(T x, T y) : Tuple2<Point2, T>(x, y) {}
    template <typename U>
    explicit Point2(Point2<U> u) : Tuple2<Point2, T>(T(u.x), T(u.y)) {}

    template <typename U>
    explicit Point2(Vec2<U> u) : Tuple2<Point2, T>(T(u.x), T(u.y)) {}

    template <typename U>
    auto operator+(Vec2<U> c) const -> Point2<decltype(T{} + U{})> {
        return {x + c.x, y + c.y};
    }

    template <typename U>
    Point2<T> &operator+=(Vec2<U> c) {
        x += c.x;
        y += c.y;
        return *this;
    }

    Point2<T> operator-() const { return {-x, -y}; }

    template <typename U>
    auto operator-(Point2<U> c) const -> Vec2<decltype(T{} - U{})> {
        return {x - c.x, y - c.y};
    }

    template <typename U>
    auto operator-(Vec2<U> c) const -> Point2<decltype(T{} - U{})> {
        return {x - c.x, y - c.y};
    }

    template <typename U>
    Point2<T> &operator-=(Vec2<U> c) {
        x -= c.x;
        y -= c.y;
        return *this;
    }
};

template<typename T> class Point3;

template <typename T>
class Vec3 : public Tuple3<Vec3, T> {
public:
    using Tuple3<Vec3, T>::x;
    using Tuple3<Vec3, T>::y;
    using Tuple3<Vec3, T>::z;
    Vec3() = default;
    Vec3(T x, T y, T z) : Tuple3<Vec3, T>(x, y, z) {}
    template <typename U>
    explicit Vec3(Vec3<U> u) 
        : Tuple3<Vec3, T>(T(u.x), T(u.y), T(u.z)) {}

    template <typename U>
    explicit Vec3(Point3<U> u); // : Tuple3<Vec3, T>(T(u.x), T(u.y)) {}
};


using Vec3f = Vec3<float>;
using Vec3i = Vec3<int>;

template <typename T>
class Point3 : public Tuple3<Point3, T> {
public:
    using Tuple3<Point3, T>::x;
    using Tuple3<Point3, T>::y;
    using Tuple3<Point3, T>::z;
    using Tuple3<Point3, T>::hasNaN;
    using Tuple3<Point3, T>::operator+;
    using Tuple3<Point3, T>::operator+=;
    using Tuple3<Point3, T>::operator*;
    using Tuple3<Point3, T>::operator*=;

    Point3() {x = y = 0; }
    Point3(T x, T y) : Tuple3<Point3, T>(x, y) {}
    // this doesn't allow me to = assign point to Vec or viceversa
    template <typename U>
    explicit Point3(Point3<U> u) : Tuple3<Point3, T>(T(u.x), T(u.y), T(u.z)) {}
    template <typename U>
    explicit Point3(Vec3<U> u) : Tuple3<Point3, T>(T(u.x), T(u.y), T(u.z)) {}

    template <typename U>
    auto operator+(Vec3<U> c) const -> Point3<decltype(T{} + U{})> {
        return {x + c.x, y + c.y, z + c.z};
    }

    template <typename U>
    Point3<T> &operator+=(Vec3<U> c) {
        x += c.x;
        y += c.y;
        z += c.z;
        return *this;
    }

    Point3<T> operator-() const { return {-x, -y, -z}; }

    template <typename U>
    auto operator-(Point3<U> c) const -> Vec3<decltype(T{} - U{})> {
        return {x - c.x, y - c.y, z - c.z};
    }

    template <typename U>
    auto operator-(Vec3<U> c) const -> Point3<decltype(T{} - U{})> {
        return {x - c.x, y - c.y, z - c.z};
    }

    template <typename U>
    Point3<T> &operator-=(Vec3<U> c) {
        x -= c.x;
        y -= c.y;
        z -= c.z;
        return *this;
    }
};

using Point3f = Point3<float>;
using Point3i = Point3<int>;

template <typename T>
template <typename U>
Vec2<T>::Vec2(Point2<U> u) : Tuple2<Vec2, T>(T(u.x), T(u.y)) {}

template <typename T>
template <typename U>
Vec3<T>::Vec3(Point3<U> u) : Tuple3<Vec3, T>(T(u.x), T(u.y), T(u.z)) {}

template <typename T>
inline auto dot(Vec2<T> v1, Vec2<T> v2) ->
    typename TupleLength<T>::type {
    assert(!v1.hasNaN() && !v2.hasNaN());
    return sumOfProducts(v1.x, v2.x, v1.y, v2.y);
}

template <typename T>
inline auto absDot(Vec2<T> v1, Vec2<T> v2) ->
    typename TupleLength<T>::type {
    assert(!v1.hasNaN() && !v2.HasNaN());
    return std::abs(dot(v1, v2));
}

template <typename T>
inline auto lengthSquared(Vec2<T> v) -> typename TupleLength<T>::type {
    return sqr(v.x) + sqr(v.y);
}

template <typename T>
inline auto length(Vec2<T> v) -> typename TupleLength<T>::type {
    using std::sqrt;
    return sqrt(lengthSquared(v));
}

template <typename T>
inline auto normalize(Vec2<T> v) {
    return v / Length(v);
}

template <typename T>
inline auto distance(Point2<T> p1, Point2<T> p2) ->
    typename TupleLength<T>::type {
    return length(p1 - p2);
}

template <typename T>
inline auto distanceSquared(Point2<T> p1, Point2<T> p2) ->
    typename TupleLength<T>::type {
    return lengthSquared(p1 - p2);
}

/*
 * An arbitrary size integer class.
 * The overloaded operators are mostly implementation of the algorithms in
 * The Art of Computer Programming vol.2 by Donald Knuth
 */

struct Integer {
    static constexpr u64 BASE = 4294967296;
    Integer(i32 x = 0);
    Integer(const Integer &x);
    Integer(Integer &&x);
    explicit Integer(const std::string &str);
    Integer operator=(const Integer &x);
    Integer operator=(Integer &&x);
    void fromString(const std::string &str); 
    void normalize();
    u32 operator[](usize i) const { return digits[i]; }
    u32 &operator[](usize i) { return digits[i]; }

    int sign;
    std::vector<u32> digits;
};

Integer operator+(const Integer &x, const Integer &y);
Integer operator-(const Integer &x, const Integer &y);
Integer operator-(const Integer &x);
std::pair<Integer, Integer> longDivision(Integer x, Integer y, bool compute_remainder);
Integer operator/(const Integer &x, const Integer &y);
Integer operator%(const Integer &x, const Integer &y);
std::pair<Integer, u32> shortDivision(const Integer &x, u32 y, bool compute_remainder);
Integer operator/(const Integer &x, u32 y);
Integer operator%(const Integer &x, u32 y);
Integer operator*(const Integer &x, const Integer &y);
Integer abs(Integer x);
Integer gcd(Integer p, Integer q);
Integer lcm(Integer p, Integer q);
Integer pow(const Integer &x, Integer n);
Integer pow(Integer x, i64 n);
bool operator==(const Integer &x, const Integer &y);
bool operator!=(const Integer &x, const Integer &y);
bool operator<( const Integer &x, const Integer &y);
bool operator<=(const Integer &x, const Integer &y);
bool operator>( const Integer &x, const Integer &y);
bool operator>=(const Integer &x, const Integer &y);
std::ostream &operator<<(std::ostream &out, Integer a);
std::istream &operator>>(std::istream &out, Integer &a);
Integer unsignedAdd(const Integer &x, const Integer &y);
Integer unsignedSub(const Integer &x, const Integer &y);
bool  unsignedGreater(const Integer &x, const Integer &y);

/*
 * Simple rational implementation based on the Integer structure.
 * Not much to notice here, the usual c++ operator have been overloaded
 */

struct Rational {
    Rational(i64 n = 0);
    Rational(i64 p, i64 q);
    Rational(const Integer &a, const Integer &b);
    Rational(Integer n);
    Rational(const Rational &x);
    Rational(Rational &&x);
    Rational operator=(const Rational &x);
    Rational operator=(Rational &&x);
    void normalize();
    Integer p, q;
};

Rational operator+(const Rational &x, const Rational &y);
Rational operator-(const Rational &x, const Rational &y);
Rational operator-(const Rational &x);
Rational operator/(const Rational &x, const Rational &y);
Rational operator/(const Rational &x, u64 y);
Rational operator*(const Rational &x, const Rational &y);
bool operator==(const Rational &x, const Rational &y);
bool operator!=(const Rational &x, const Rational &y);
bool operator<( const Rational &x, const Rational &y);
bool operator<=(const Rational &x, const Rational &y);
bool operator>( const Rational &x, const Rational &y);
bool operator>=(const Rational &x, const Rational &y);
Rational abs(Rational x);
Rational pow(const Rational &x, Integer n);
Rational pow(Rational x, i64 n);
std::ostream &operator<<(std::ostream &out, Rational a);

/*
 * This is an implementation of the paper 
 * Arbitrary precision real arithmetic: design and algorithms
 * by
 * Valérie Ménissier-Morain
 */

//struct Real {
//};


#ifdef EXTRA_MATH_IMPLEMENTATION

#include <iomanip>
#include <iostream>
#include <cstring>

Integer::Integer(i32 x) {
    sign = 1;
    if (x < 0) { sign = -1; x = -x; }
    digits.push_back(x);
}

Integer::Integer(const std::string &str) {
    fromString(str);
}

Integer::Integer(const Integer &x) {
    if (&x == this) return;
    sign = x.sign;
    digits = x.digits;
}

Integer Integer::operator=(const Integer &x) {
    if (&x == this) return *this;
    sign = x.sign;
    digits = x.digits;
    return *this;
}

Integer::Integer(Integer &&x) {
    if (&x == this) return;
    sign = x.sign;
    digits = std::move(x.digits);
}

Integer Integer::operator=(Integer &&x) {
    if (&x != this) {
        sign = x.sign;
        digits = std::move(x.digits);
    }
    return *this;
}

void Integer::fromString(const std::string &str) {
    if (!str.size()) throw std::invalid_argument("Invalid initializer");  
    sign = 1;
    digits.resize(0);

    if (str[0] == '-') sign = -1;
    usize i = sign > 0 ? 0 : 1;
    for (; i < str.size(); ++i) {
        *this = (*this * 10) + (str[i] - '0');
    }

    normalize();
    if (digits.empty()) sign = 1;
}

void Integer::normalize() {
    while (!digits.empty() && digits.back() == 0) digits.pop_back();
    if (digits.empty()) sign = 1;
}

Integer unsignedAdd(const Integer &x, const Integer &y) {
    usize n = max(x.digits.size(), y.digits.size());
    Integer z;
    z.digits.resize(n+1);
    u32 k = 0;
    for (u64 j = 0; j < n; ++j) {
        u64 xj = (j < x.digits.size() ? x.digits[j] : 0);
        u64 yj = (j < y.digits.size() ? y.digits[j] : 0);
        u64 sum = (xj + yj + k);
        z.digits[j] = (u32)(sum & 0xffffffff);
        k = sum >= Integer::BASE;
    }
    z.digits[n] = k;
    z.normalize();
    return z;
}

Integer unsignedSub(const Integer &x, const Integer &y) {
    i32 k = 0;
    usize n = max(x.digits.size(), y.digits.size());
    Integer z;
    z.digits.resize(n, 0);
    for (usize j = 0; j < n; ++j) {
        u64 xj = (j < x.digits.size() ? x.digits[j] : 0);
        u64 yj = (j < y.digits.size() ? y.digits[j] : 0);
        u64 sub = (xj - yj + k);
        z.digits[j] = sub & 0xffffffff;
        k = (sub >> 32) == 0 ? 0 : -1;
    }
    z.normalize();
    return z;
}

bool unsignedGreater(const Integer &x, const Integer &y) {
    if (x.digits.size() > y.digits.size()) return true;
    if (x.digits.size() < y.digits.size()) return false;
    for (isize i = x.digits.size() - 1; i >= 0; --i) {
        if (x.digits[i] > y.digits[i]) return true;
    }
    return false;
}

Integer operator+(const Integer &x, const Integer &y) {
    Integer z;
    if (x.sign != y.sign) {
        if (unsignedGreater(x, y)) {
            z = unsignedSub(x, y);
            z.sign = x.sign;
        } else {
            z = unsignedSub(y, x);
            z.sign = y.sign;
        }
    } else {
        z = unsignedAdd(x, y);
        z.sign = x.sign;
    }
    return z;
}

Integer operator-(const Integer &x, const Integer &y) {
    Integer z;
    if (x.digits.empty()) {
        z = y;
        z.sign = -1 * y.sign;
        return z;
    }
    if (y.digits.empty()) return x;
    if (x.sign != y.sign) {
        z = unsignedAdd(x, y);
        z.sign = x.sign;
    } else {
        if (unsignedGreater(x, y)) {
            z = unsignedSub(x, y);
            z.sign = x.sign;
        } else {
            z = unsignedSub(y, x);
            z.sign = -1 * x.sign;
        }
    }
    return z;
}

Integer operator-(const Integer &x) {
    Integer z = x;
    z.sign *= -1;
    return z;
}

Integer operator*(const Integer &x, const Integer &y) {
    if (x.digits.empty() || y.digits.empty()) return 0;
    usize m = x.digits.size(), n = y.digits.size();
    Integer z;
    z.digits.resize(m + n, 0);
    for (usize j = 0; j < n; ++j) {
        u32 k = 0;
        for (usize i = 0; i < m; ++i) {
            u64 t = (u64)x.digits[i] * (u64)y.digits[j] + z.digits[i+j] + k;
            z.digits[i+j] = t & 0xffffffff;
            k = t >> 32;
        }
        z.digits[j+m] = k;
    }
    z.sign = x.sign * y.sign;
    z.normalize();
    return z;
}

std::pair<Integer, u32> shortDivision(const Integer &x, u32 y, bool compute_remainder) {
    if (!y) throw std::invalid_argument("Division by zero");  
    if (x == 0) return {0, 0};
    usize m = x.digits.size();
    Integer q;
    Integer r;
    r.digits.resize(m+1, 0);
    q.digits.resize(m, 0);
    for (i64 k = m - 1; k >= 0; --k) {
        q[k] = (((u64)r[k+1] << 32) + (u64)x[k]) / y;
        r[k] = (((u64)r[k+1] << 32) + (u64)x[k]) % y;
    }
    q.normalize();
    return { q, r[0] };
}

i32 leadingZeroes(u32 x) {
    i32 n = 32;
    u32 y;
    y = x >>16; if (y != 0) { n = n -16; x = y; }
    y = x >> 8; if (y != 0) { n = n - 8; x = y; }
    y = x >> 4; if (y != 0) { n = n - 4; x = y; }
    y = x >> 2; if (y != 0) { n = n - 2; x = y; }
    y = x >> 1; if (y != 0) return n - 2;
    return n - x;

}

// from Algorithm D, The Art of computer programming vol 2. Donald knuth
std::pair<Integer, Integer> longDivision(Integer u, Integer v, bool compute_remainder) {
    std::cout << "here" <<std::endl;
    if (v == 0) throw std::invalid_argument("Division bv zero");  
    if (u == 0) return {0, 0};
    if (u < v) return {0, u};

    usize n = v.digits.size();
    usize m = u.digits.size() - n;
    if (n == 1) return shortDivision(u, v.digits[0], compute_remainder);
    u64 b = Integer::BASE;
    // normalization
    i32 s = leadingZeroes(v[n-1]);
    for (u64 i = v.digits.size() - 1; i > 0; i--) {
        v[i] = ((u64)v[i] << s) | ((u64)v[i-1] >> (32-s));
    }
    v[0] = v[0] << s;
    u.digits.resize(m+n+1, 0);
    u[m+n] = (u64)u[m+n-1] >> (32-s);
    for (u64 i = u.digits.size() - 2; i > 0; i--) {
        u[i] = ((u64)u[i] << s) | ((u64)u[i-1] >> (32-s));
    }
    u[0] = u[0] << s;
    Integer q;
    q.sign = u.sign * v.sign;
    q.digits.resize(m+1, 0);
    // D2 / D7
    for (i64 j = m; j >= 0; --j)  {
        // D3.
        u64 qhat = (((u64)u[j+n]<<32) + (u64)u[j+n-1]) / (u64)v[n-1];
        u64 rhat = (((u64)u[j+n]<<32) + (u64)u[j+n-1]) % (u64)v[n-1];
        while (qhat >= b || qhat * (u64)v[n-2] > b * rhat + (u64)u[j+n-2]) {
            --qhat;
            rhat = rhat + v[n-1];
            if (rhat >= b) break;
        }
        // D4. multiply and subtract
        u64 k = 0;
        i64 t;
        for (u64 i = 0; i < n; ++i) {
            u64 p = qhat * (u64)v[i];
            t = u[i+j] - k - (p & 0xffffffff);
            u[i+j] = t;
            k = (p >> 32) - (t >> 32);
        }
        // remember borrow
        t = u[j+n] - k;
        u[j+n] = t;
        // D5.
        q[j] = qhat;
        // D6. Add Back
        if (t < 0) {
            --q[j];
            k = 0;
            for (u64 i = 0; i < n; ++i) {
                t = (u64)u[i+j] + (u64)v[i] + k;
                u[i+j] = t;
                k = t >> 32;
            }
            u[j+n] = u[j+n] + k;
        }
    }

    q.normalize();
    if (compute_remainder) {
        u.digits.resize(n);
        for (u64 i = 0; i < n-1; i++) {
            u[i] = (u[i] >> s) | ((u64)u[i+1] << (32-s));
        }
        u[n-1] = u[n-1] >> s;
    }
    return {q, u};
}

Integer operator/(const Integer &x, const Integer &y) {
    return longDivision(x, y, false).first;
}
Integer operator%(const Integer &x, const Integer &y) {
    return longDivision(x, y, true).second;
}

Integer operator/(const Integer &x, u64 y) {
    return shortDivision(x, y, false).first;
}

Integer operator%(const Integer &x, u64 y) {
    return shortDivision(x, y, true).second;
}

Integer abs(Integer x) {
    x.sign = 1;
    return x;
}

Integer pow(const Integer &x, Integer n) {
    Integer z = 1;
    while (n > 0) {
        n = n - 1;
        z = z * x;
    }
    return z;
}

Integer pow(Integer x, i64 n) {
    Integer z = 1;
    while (n > 0) {
        n = n - 1;
        z = z * x;
    }
    return z;
}

Integer gcd(Integer p, Integer q) {
    Integer t;
    while (q != 0) {
        t = q;
        q = p % q;
        p = t;
    }

    return p;
}

Integer lcm(Integer p, Integer q) {
    return p * q / gcd(p, q);
}

bool operator==(const Integer &x, const Integer &y) {
    if (x.sign != y.sign) return false;
    if (x.digits.size() != y.digits.size()) return false;
    if (std::memcmp(x.digits.data(), y.digits.data(), x.digits.size() * sizeof(u32)) != 0) {
        return false;
    }
    return true;
}

bool operator>( const Integer &x, const Integer &y) {
    if (x.sign > y.sign) return true;
    if (x.sign < y.sign) return false;
    if (x.sign < 0) {
        if (x.digits.size() > y.digits.size()) return false;
        if (x.digits.size() < y.digits.size()) return true;
        for (isize i = x.digits.size() - 1; i >= 0; --i) {
            if (x.digits[i] < y.digits[i]) return true;
        }
    } else {
        if (x.digits.size() > y.digits.size()) return true;
        if (x.digits.size() < y.digits.size()) return false;
        for (isize i = x.digits.size() - 1; i >= 0; --i) {
            if (x.digits[i] > y.digits[i]) return true;
        }
    }
    return false;
}

bool operator>=(const Integer &x, const Integer &y) {
    if (x.sign > y.sign) return true;
    if (x.sign < y.sign) return false;
    if (x.sign < 0) {
        if (x.digits.size() > y.digits.size()) return false;
        if (x.digits.size() < y.digits.size()) return true;
        for (isize i = x.digits.size() - 1; i >= 0; --i) {
            if (x.digits[i] > y.digits[i]) return false;
        }
    } else {
        if (x.digits.size() > y.digits.size()) return true;
        if (x.digits.size() < y.digits.size()) return false;
        for (isize i = x.digits.size() - 1; i >= 0; --i) {
            if (x.digits[i] < y.digits[i]) return false;
        }
    }
    return true;
}

bool operator!=(const Integer &x, const Integer &y) { return !(x == y); }
bool operator<( const Integer &x, const Integer &y) { return !(x >= y); } 
bool operator<=(const Integer &x, const Integer &y) { return !(x >  y); }

std::ostream &operator<<(std::ostream &out, Integer a) {
    if (a.digits.empty()) { 
        out << 0;
        return out;
    }
    if (a.sign < 0) out << '-';
    std::stringstream ss;
    while (a.digits.size() != 0) {
        auto res = shortDivision(a, 10, true);
        a = res.first;
        ss << res.second;
    }

    std::string str = ss.str();
    for (isize i = str.size() - 1; i >= 0; --i) {
        out << str[i];
    }

    return out;
}

std::istream &operator>>(std::istream &in, Integer &a) {
   std::string s;
   in >> s;
   a.fromString(s);
   return in;
}

#define assert_integer_equals(num, sr)                                     \
    do { std::stringstream s; s << num;                                    \
         assert( s.str().compare(sr) == 0 );                               \
    } while (false)


Rational::Rational(i64 n) {
    p = n;
    q = 1;
}
Rational::Rational(i64 a, i64 b) {
    p = a;
    q = b;
    normalize();
}

Rational::Rational(const Integer &a, const Integer &b) {
    p = a;
    q = b;
    normalize();
}

Rational::Rational(Integer n) {
    p = n;
    q = 1;
    normalize();
}
Rational::Rational(const Rational &x) {
    p = x.p;
    q = x.q;
}

Rational::Rational(Rational &&x) {
    p = x.p;
    q = x.q;
}

Rational Rational::operator=(const Rational &x) {
    p = x.p;
    q = x.q;
    return *this;
}

Rational Rational::operator=(Rational &&x) {
    p = x.p;
    q = x.q;
    return *this;
}

void Rational::normalize() {
    if (q.sign == -1) {
        p.sign *= -1;
        q.sign = 1;
    }

    if (p == 1 || q == 1) return;
    std::cout << *this << std::endl;
    Integer z = gcd(p, q);
    while (z != 1) {
        p = p / z;
        q = q / z;
        z = gcd(p, q);
        std::cout << *this << " gcd = " << z << std::endl;
    }
}

Rational operator+(const Rational &x, const Rational &y) { 
    Rational z;
    Integer denominator = lcm(x.q, y.q);
    z.q = denominator;
    z.p = x.p * (denominator / x.q) + y.p * (denominator / y.q);
    z.normalize();
    return z;
}

Rational operator-(const Rational &x, const Rational &y) {
    Rational z;
    Integer denominator = lcm(x.q, y.q);
    z.q = denominator;
    z.p = x.p * denominator - y.p * denominator;
    z.normalize();
    return z;
}

Rational operator-(const Rational &x) {
    Rational z = x;
    z.p.sign *= -1;
    return z;
}

Rational operator/(const Rational &x, const Rational &y) {
    Rational z{ x.p * y.q, x.q * y.p };
    z.normalize();
    return z;
}

Rational operator/(const Rational &x, u64 y) {
    Rational z{ x.p, x.q * y};
    z.normalize();
    return z;
}

Rational operator*(const Rational &x, const Rational &y) {
    Rational z;
    z.p = x.p * y.p;
    z.q = x.q * y.q;
    z.normalize();
    return z;
}

bool operator==(const Rational &x, const Rational &y) {
    return x.p == y.p && x.q == y.q;
}

bool operator!=(const Rational &x, const Rational &y) {
    return !(x == y);
}
bool operator<( const Rational &x, const Rational &y) {
    Integer z = x.p * y.q - x.q * y.p;
    return z < 0;
}

bool operator<=(const Rational &x, const Rational &y) {
    Integer z = x.p * y.q - x.q * y.p;
    return z <= 0;
}

bool operator>( const Rational &x, const Rational &y) {
    return !(x <= y);
}

bool operator>=(const Rational &x, const Rational &y) {
    return !(x < y);
}

Rational abs(Rational x) { 
    x.p.sign = 1;
    return x;
}

Rational pow(const Rational &x, Integer n) {
    Rational z = 1;
    while (n > 0) {
        n = n - 1;
        z = z * x;
    }
    return z;
}

Rational pow(Rational x, i64 n) {
    Rational z = 1;
    while (n > 0) {
        n = n - 1;
        z = z * x;
    }
    return z;
}

std::ostream &operator<<(std::ostream &out, Rational a) {
    if (a.q == 1) {
        out << a.p;
    } else {
        out << a.p << "/" << a.q;
    }
    return out;
}



#endif // EXTRA_MATH_IMPLEMENTATION
#endif // extra_math_h
