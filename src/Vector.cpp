#include "Vector.h"
#include "VUtils.h"
#include "VUtilsMath.h"

const Vector2 Vector2::ZERO(0, 0);

Vector2 &Vector2::operator=(const Vector2& other) {
    x = other.x;
    y = other.y;
    return *this;
}

Vector2 Vector2::operator+(const Vector2& other) const {
    return Vector2(x + other.x, y + other.y);
}
Vector2 Vector2::operator-(const Vector2& other) const {
    return Vector2(x - other.x, y - other.y);
}
Vector2 Vector2::operator*(const Vector2& other) const {
    return Vector2(x * other.x, y * other.y);
}
Vector2 Vector2::operator/(const Vector2& other) const {
    return Vector2(x / other.x, y / other.y);
}

Vector2 Vector2::operator*(float other) const {
    return Vector2(x * other, y * other);
}
Vector2 Vector2::operator/(float other) const {
    return Vector2(x / other, y / other);
}

Vector2& Vector2::operator+=(const Vector2& other) {
    x += other.x;
    y += other.y;
    return *this;
}
Vector2& Vector2::operator-=(const Vector2& other) {
    x -= other.x;
    y -= other.y;
    return *this;
}
Vector2& Vector2::operator*=(const Vector2& other) {
    x *= other.x;
    y *= other.y;
    return *this;
}
Vector2& Vector2::operator/=(const Vector2& other) {
    x /= other.x;
    y /= other.y;
    return *this;
}

Vector2& Vector2::operator*=(float other) {
    x *= other;
    y *= other;
    return *this;
}
Vector2& Vector2::operator/=(float other) {
    x /= other;
    y /= other;
    return *this;
}

bool Vector2::operator==(const Vector2& other) const {
    return x == other.x
        && y == other.y;
}
bool Vector2::operator!=(const Vector2& other) const {
    return !(*this == other);
}


Vector2& Vector2::Normalize() {
    // normalization is diving by length

    // its also the same as multiplying by the inverse length

    // also the same as multiplying by the inverse sqrt length
    auto sqmagnitude = SqMagnitude();

    if (sqmagnitude > 1E-05f * 1E-05f) {
        *this *= VUtils::Math::FISQRT(sqmagnitude);
    }
    else {
        *this = Vector2::ZERO;
    }
    return *this;
}

Vector2 Vector2::Normalized() {
    Vector2 vec(x, y);
    vec.Normalize();
    return vec;
}

std::ostream& operator<<(std::ostream& st, const Vector2& vec) {
    return st << "(" << vec.x << ", " << vec.y << ")";
}










const Vector2i Vector2i::ZERO(0, 0);

Vector2i& Vector2i::operator=(const Vector2i& other) {
    x = other.x;
    y = other.y;
    return *this;
}

Vector2i Vector2i::operator+(const Vector2i& other) const {
    return Vector2i(x + other.x, y + other.y);
}
Vector2i Vector2i::operator-(const Vector2i& other) const {
    return Vector2i(x - other.x, y - other.y);
}
Vector2i Vector2i::operator*(const Vector2i& other) const {
    return Vector2i(x * other.x, y * other.y);
}
Vector2i Vector2i::operator/(const Vector2i& other) const {
    return Vector2i(x / other.x, y / other.y);
}

Vector2i Vector2i::operator*(float other) const {
    return Vector2i(x * other, y * other);
}
Vector2i Vector2i::operator/(float other) const {
    return Vector2i(x / other, y / other);
}



Vector2i& Vector2i::operator+=(const Vector2i& other) {
    x += other.x;
    y += other.y;
    return *this;
}
Vector2i& Vector2i::operator-=(const Vector2i& other) {
    x -= other.x;
    y -= other.y;
    return *this;
}
Vector2i& Vector2i::operator*=(const Vector2i& other) {
    x *= other.x;
    y *= other.y;
    return *this;
}
Vector2i& Vector2i::operator/=(const Vector2i& other) {
    x /= other.x;
    y /= other.y;
    return *this;
}

Vector2i& Vector2i::operator*=(float other) {
    x *= other;
    y *= other;
    return *this;
}
Vector2i& Vector2i::operator/=(float other) {
    x /= other;
    y /= other;
    return *this;
}

bool Vector2i::operator==(const Vector2i& other) const {
    return x == other.x
        && y == other.y;
}
bool Vector2i::operator!=(const Vector2i& other) const {
    return !(*this == other);
}


Vector2i& Vector2i::Normalize() {
    // normalization is diving by length

    // its also the same as multiplying by the inverse length

    // also the same as multiplying by the inverse sqrt length
    auto sqmagnitude = SqMagnitude();

    if (sqmagnitude > 1E-05f * 1E-05f) {
        *this *= VUtils::Math::FISQRT(sqmagnitude);
    }
    else {
        *this = Vector2i::ZERO;
    }
    return *this;
}

Vector2i Vector2i::Normalized() {
    Vector2i vec(x, y);
    vec.Normalize();
    return vec;
}

std::ostream& operator<<(std::ostream& st, const Vector2i& vec) {
    return st << "(" << vec.x << ", " << vec.y << ")";
}










const Vector3 Vector3::ZERO(0, 0, 0);
const Vector3 Vector3::UP(0, 1, 0);
const Vector3 Vector3::DOWN(0, -1, 0);
const Vector3 Vector3::FORWARD(0, 0, 1);

Vector3& Vector3::operator=(const Vector3& other) {
    x = other.x;
    y = other.y;
    z = other.z;
    return *this;
}

Vector3 Vector3::operator+(const Vector3& other) const {
    return Vector3(x + other.x, y + other.y, z + other.z);
}
Vector3 Vector3::operator-(const Vector3& other) const {
    return Vector3(x - other.x, y - other.y, z - other.z);
}
Vector3 Vector3::operator*(const Vector3& other) const {
    return Vector3(x * other.x, y * other.y, z * other.z);
}
Vector3 Vector3::operator/(const Vector3& other) const {
    return Vector3(x / other.x, y / other.y, z / other.z);
}

Vector3 Vector3::operator*(float other) const {
    return Vector3(x * other, y * other, z * other);
}
Vector3 Vector3::operator/(float other) const {
    return Vector3(x / other, y / other, z / other);
}

Vector3& Vector3::operator+=(const Vector3& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}
Vector3& Vector3::operator-=(const Vector3& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}
Vector3& Vector3::operator*=(const Vector3& other) {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return *this;
}
Vector3& Vector3::operator/=(const Vector3& other) {
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return *this;
}

Vector3& Vector3::operator*=(float other) {
    x *= other;
    y *= other;
    z *= other;
    return *this;
}
Vector3& Vector3::operator/=(float other) {
    x /= other;
    y /= other;
    z /= other;
    return *this;
}

bool Vector3::operator==(const Vector3& other) const {
    return x == other.x
        && y == other.y
        && z == other.z;
}
bool Vector3::operator!=(const Vector3& other) const {
    return !(*this == other);
}


Vector3& Vector3::Normalize() {
    // normalization is dividing by length

    // its also the same as multiplying by the inverse length

    // also the same as multiplying by the inverse sqrt length

    //double sqmagnitude = ((double)x * (double)x) + ((double)y * (double)y) + ((double)z * (double)z);
    //if (sqmagnitude > std::numeric_limits<double>::epsilon()) {
    //    //*this *= VUtils::Math::FISQRT(sqmagnitude);
    //
    //    //if (sqmagnitude + std::numeric_limits<float>::epsilon() > )
    //    *this *= (1.0 / std::sqrt(sqmagnitude));
    //
    //    if (std::abs(x) <= std::numeric_limits<float>::epsilon() * 2.f)
    //        x = 0;
    //    if (std::abs(y) <= std::numeric_limits<float>::epsilon() * 2.f)
    //        y = 0;
    //    if (std::abs(z) <= std::numeric_limits<float>::epsilon() * 2.f)
    //        z = 0;
    //}
    //else {
    //    *this = Vector3::ZERO;
    //}

    auto sqmagnitude = SqMagnitude();
    if (sqmagnitude > 1E-05f * 1E-05f * 1E-05f) {
        *this *= VUtils::Math::FISQRT(sqmagnitude);
    }
    else {
        *this = Vector3::ZERO;
        throw std::runtime_error("cannot normalize zero vector");
    }
    return *this;
}

Vector3 Vector3::Normalized() const {
    Vector3 vec(x, y, z);
    vec.Normalize();
    return vec;
}

std::ostream& operator<<(std::ostream& st, const Vector3& vec) {
    return st << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
}
