#pragma once
#include <cmath>
#include <algorithm>

class vec2_t {
public:
	float x, y;
};

class vec3_t {
public:
	float x, y, z;

	vec3_t( ) {
		init( );
	}

	vec3_t( float x, float y, float z = 0.0f ) {
		this->x = x;
		this->y = y;
		this->z = z;
	}

	void init( ) {
		this->x = this->y = this->z = 0.0f;
	}

	void init( float x, float y, float z ) {
		this->x = x;
		this->y = y;
		this->z = z;
	}

	bool is_valid( ) {
		return std::isfinite( this->x ) && std::isfinite( this->y ) && std::isfinite( this->z );
	}

	bool is_zero( ) {
		return vec3_t( this->x, this->y, this->z ) == vec3_t( 0.0f, 0.0f, 0.0f );
	}

	void invalidate( ) {
		this->x = this->y = this->z = std::numeric_limits< float >::infinity( );
	}

	void clear( ) {
		this->x = this->y = this->z = 0.0f;
	}

	float& operator[]( int i ) {
		return ( ( float* ) this ) [ i ];
	}

	float operator[]( int i ) const {
		return ( ( float* ) this ) [ i ];
	}

	void zero( ) {
		this->x = this->y = this->z = 0.0f;
	}

	bool operator==( const vec3_t& src ) const {
		return ( src.x == this->x ) && ( src.y == y ) && ( src.z == z );
	}

	bool operator!=( const vec3_t& src ) const {
		return ( src.x != this->x ) || ( src.y != y ) || ( src.z != z );
	}

	vec3_t& operator+=( const vec3_t& v ) {
		this->x += v.x; this->y += v.y; this->z += v.z;

		return *this;
	}

	vec3_t& operator-=( const vec3_t& v ) {
		this->x -= v.x; this->y -= v.y; this->z -= v.z;

		return *this;
	}

	vec3_t& operator*=( float fl ) {
		this->x *= fl;
		this->y *= fl;
		this->z *= fl;

		return *this;
	}

	vec3_t& operator*=( const vec3_t& v ) {
		this->x *= v.x;
		this->y *= v.y;
		this->z *= v.z;

		return *this;
	}

	vec3_t& operator/=( const vec3_t& v ) {
		this->x /= v.x;
		this->y /= v.y;
		this->z /= v.z;

		return *this;
	}

	vec3_t& operator+=( float fl ) {
		this->x += fl;
		this->y += fl;
		this->z += fl;

		return *this;
	}

	vec3_t& operator/=( float fl ) {
		this->x /= fl;
		this->y /= fl;
		this->z /= fl;

		return *this;
	}

	vec3_t& operator-=( float fl ) {
		this->x -= fl;
		this->y -= fl;
		this->z -= fl;

		return *this;
	}

	void normalize( ) {
		*this = normalized( );
	}

	vec3_t normalized( ) const {
		auto res = *this;
		auto l = res.length( );

		if ( l != 0.0f )
			res /= l;
		else
			res.x = res.y = res.z = 0.0f;

		return res;
	}

	void normalize_place( ) {
		auto res = *this;
		auto radius = std::sqrtf( x * x + y * y + z * z );
		auto iradius = 1.0f / ( radius + FLT_EPSILON );

		res.x *= iradius;
		res.y *= iradius;
		res.z *= iradius;
	}

	float dist_to( const vec3_t& vec ) const {
		vec3_t delta;

		delta.x = this->x - vec.x;
		delta.y = this->y - vec.y;
		delta.z = this->z - vec.z;

		return delta.length( );
	}

	float dist_to_sqr( const vec3_t& vec ) const {
		vec3_t delta;

		delta.x = this->x - vec.x;
		delta.y = this->y - vec.y;
		delta.z = this->z - vec.z;

		return delta.length_sqr( );
	}

	float dot_product( const vec3_t& vec ) const {
		return this->x * vec.x + this->y * vec.y + this->z * vec.z;
	}

	vec3_t cross_product( const vec3_t& vec ) const {
		return vec3_t( this->y * vec.z - this->z * vec.y, this->z * vec.x - this->x * vec.z, this->x * vec.y - this->y * vec.x );
	}

	float length( ) const {
		return std::sqrtf( this->x * this->x + this->y * this->y + this->z * this->z );
	}

	float length_sqr( ) const {
		return this->x * this->x + this->y * this->y + this->z * this->z;
	}

	float length_2d_sqr( ) const {
		return this->x * this->x + this->y * this->y;
	}

	float length_2d( ) const {
		return std::sqrtf( this->x * this->x + this->y * this->y );
	}

	vec3_t& operator=( const vec3_t& vec ) {
		this->x = vec.x; this->y = vec.y; this->z = vec.z;

		return *this;
	}

	vec3_t operator-( ) const {
		return vec3_t( -this->x, -this->y, -this->z );
	}

	vec3_t operator+( const vec3_t& v ) const {
		return vec3_t( this->x + v.x, this->y + v.y, this->z + v.z );
	}

	vec3_t operator-( const vec3_t& v ) const {
		return vec3_t( this->x - v.x, this->y - v.y, this->z - v.z );
	}

	vec3_t operator*( float fl ) const {
		return vec3_t( this->x * fl, this->y * fl, this->z * fl );
	}

	vec3_t operator*( const vec3_t& v ) const {
		return vec3_t( this->x * v.x, this->y * v.y, this->z * v.z );
	}

	vec3_t operator/( float fl ) const {
		return vec3_t( this->x / fl, this->y / fl, this->z / fl );
	}

	vec3_t operator/( const vec3_t& v ) const {
		return vec3_t( this->x / v.x, this->y / v.y, this->z / v.z );
	}
};

__forceinline vec3_t operator*( float lhs, const vec3_t& rhs ) {
	return rhs * lhs;
}

__forceinline vec3_t operator/( float lhs, const vec3_t& rhs ) {
	return rhs / lhs;
}

class __declspec( align( 16 ) ) vec_aligned_t : public vec3_t {
public:
	inline vec_aligned_t ( ) { }
	inline vec_aligned_t ( float x, float y, float z ) {
		init ( x, y, z );
	}

	explicit vec_aligned_t ( const vec3_t& other ) {
		init ( other.x, other.y, other.z );
	}

	vec_aligned_t& operator=( const vec3_t& other ) {
		init ( other.x, other.y, other.z );
		return *this;
	}

	float w;
};