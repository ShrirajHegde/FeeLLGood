#ifndef pt3D_h
#define pt3D_h

/** \file pt3D.h
 * \brief class pt3D and its algebra
 * header containing pt3D class, some functions to operate algebric operations;  << >> and
 * a template to write text files from std::vector<>
 */

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <math.h>

#include "vecND.h"
/** \namespace Pt
 * to grab altogether some dedicated functions and some enum for class pt2D and pt3D
 */
namespace Pt
    {
const int DIM = 3; /**< space dimension, usefull for many tables */

/**
 *convenient enum mainly to avoid direct indices values to specify coordinates in calculations
 */
enum index
    {
    IDX_UNDEF = -1,
    IDX_X = 0,
    IDX_Y = 1,
    IDX_Z = 2
    };

/**
sign(x) = 1.0 if x>0, otherwise -1.0
*/
inline double sign(const double x)
    {
    if (x > 0)
        return 1.0;
    else
        return -1.0;
    }

/**
\return \f$ x^2 \f$
*/
inline double sq(const double x) { return x * x; }

/** \class pt3D
 \brief pt3D is a class to perform algebric operations on \f$ \mathbb{R}^3 \f$ points
 all usefull operators are defined : += ; -= ; + - * <br>
 \f$ \mathbb{R}^3 \f$ scalar product and norm are defined, return double
 */
class pt3D : public vecND<DIM>
    {
public:
    /**
     * default constructor : initialization by zero values
     */
    pt3D(): vecND<DIM>() {}

    /** constructor by values, cartesian coordinates */
    inline pt3D(const double a, const double b, const double c): vecND<DIM>()
        { set({a,b,c}); }

    /** unit vector constructor by values, spherical coordinates \f$ (r=1,\theta \in [0,\pi],\phi
     * \in [0,2 \pi]) \f$ */
    inline pt3D(const double theta, const double phi): vecND<DIM>()
        {
        const double si_t = sin(theta);
        set({si_t*cos(phi), si_t*sin(phi), cos(theta)});
        }

    /**
     * returns a unit vector built by coordinate index, usefull to build basis <br>
     * example : pt3D p = pt3D(IDX_Y); <br>
     p will contain \f$ (x,y,z) = (0.0,1.0,0.0) \f$
     */
    inline pt3D(const enum index idx): vecND<DIM>()
        {
        if (idx != IDX_UNDEF) _x[idx] = 1.0;
        }

    /**
     * getter for x coordinate : example : double x0 = pt.x();
     */
    inline double x(void) const { return _x[IDX_X]; }

    /**
     * getter for y coordinate : example : double y0 = pt.y();
     */
    inline double y(void) const { return _x[IDX_Y]; }

    /**
     * getter for z coordinate : example : double z0 = pt.z();
     */
    inline double z(void) const { return _x[IDX_Z]; }

    /**
     * \return \f$ \rho \f$ in cylindrical coordinates \f$ (\rho,\theta,z) \f$
     */
    inline double rho(void) const { return sqrt(_x[IDX_X] * _x[IDX_X] + _x[IDX_Y] * _x[IDX_Y]); }

    /**
     \return \f$ \theta \f$ in cylindrical cordinates \f$ (\rho,\theta,z) \f$
     */
    inline double theta(void) const { return atan2(_x[IDX_Y], _x[IDX_X]); }

    /**
     * setter for x coordinate : example : pt.x(0.707);
     */
    inline void x(double a) { _x[IDX_X] = a; }

    /**
     * setter for y coordinate : example : pt.y(0.707);
     */
    inline void y(double b) { _x[IDX_Y] = b; }

    /**
     * setter for z coordinate : example : pt.z(0.707);
     */
    inline void z(double c) { _x[IDX_Z] = c; }

    /**
     * normalization : divide each components x,y and z by the norm \f$ \mathcal{R}^3 \f$ in place
     * without safety. Return a reference to this pt3D.
     */
    inline pt3D &normalize(void)
        {
        double inv_r = 1.0 / norm();
        *this *= inv_r;
        return *this;
        }

    /**
     * printing function, called by operator<<
     */
    inline void print(std::ostream &flux) const
        {
        flux << _x[IDX_X] << '\t' << _x[IDX_Y] << '\t' << _x[IDX_Z];
        }

    /**
     scaling factor mainly for writing files in user units
     */

    inline void rescale(const double scaling)
        {
        *this *= scaling;
        }

    /** \return max length coordinate */
    inline double maxLength(void) { return std::max(_x[IDX_X], std::max(_x[IDX_Y], _x[IDX_Z])); }

    }; // end class pt3D

/** operator<< for pt3D, coordinates are tab separated */
inline std::ostream &operator<<(std::ostream &flux, pt3D const &p)
    {
    p.print(flux);
    return flux;
    }

/** operator>> for pt3D */
inline std::istream &operator>>(std::istream &flux, pt3D &p)
    {
    flux >> p(IDX_X) >> p(IDX_Y) >> p(IDX_Z);
    return flux;
    }

/**
* algebra : +
*/
inline pt3D operator+(pt3D const &a, pt3D const &b)
    {
    pt3D r = pt3D(a);
    r += b;
    return r;
    }

/**
* algebra : -
*/
inline pt3D operator-(pt3D const &a, pt3D const &b)
    {
    pt3D r = pt3D(a);
    r -= b;
    return r;
    }

/**
 * algebra :  vector product
 */
inline pt3D operator*(pt3D const &a, pt3D const &b)
    {
    return pt3D(a.y() * b.z() - a.z() * b.y(), a.z() * b.x() - a.x() * b.z(),
                a.x() * b.y() - a.y() * b.x());
    }

/**
 * algebra : left scalar product
 */
inline pt3D operator*(double const &a, pt3D const &b)
    {
    return pt3D(a * b.x(), a * b.y(), a * b.z());
    }

/**
 * algebra : right scalar product
 */
inline pt3D operator*(pt3D const &a, double const &b)
    {
    return pt3D(b * a.x(), b * a.y(), b * a.z());
    }

/**
 * algebra : division components by components by a scalar to the right
 */
inline pt3D operator/(pt3D const &a, double const &b)
    {
    return pt3D(a.x() / b, a.y() / b, a.z() / b);
    }

/**
algebra : R^3 scalar product
 */

inline double pScal(pt3D const &a, pt3D const &b)
    {
    return a.pScal(b);
    }

/**
algebra : R^3 direct (component to component) product
 */
inline pt3D pDirect(pt3D const &a, pt3D const &b)
    {
    return pt3D(a.x() * b.x(), a.y() * b.y(), a.z() * b.z());
    }

/**
algebra : R^3 direct (component to component) cube
 */
inline pt3D directCube(pt3D const &a)
    {
    return pt3D(a.x() * a.x() * a.x(), a.y() * a.y() * a.y(), a.z() * a.z() * a.z());
    }

/**
algebra : R^3 scalar triple product
 */
inline double pTriple(pt3D const &a, pt3D const &b, pt3D const &c) { return pScal(a * b, c); }

/**
algebra : R^3 euclidian distance (only used by some unit tests)
 */
inline double dist(pt3D const &a, pt3D const &b) { return sqrt((a-b).norm2()); }

/** check orthogonality of three vectors */
inline bool isOrthogonal(pt3D const &a, pt3D const &b, pt3D const &c, const double precision)
    {
    bool val = (fabs(pScal(a, b)) < precision);
    val &= (fabs(pScal(b, c)) < precision);
    val &= (fabs(pScal(c, a)) < precision);
    return val;
    }

/**
\return \f$ \det M \f$
*/
inline double det(const double M[DIM][DIM])
    {
    return (M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1])
            - M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0])
            + M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0]));
    }

/** compute in place \f$ M^{-1} \f$, detM must be non zero  */
inline void inverse(double M[DIM][DIM], double detM)
    {
    double m00 = M[0][0];
    double m01 = M[0][1];
    double m02 = M[0][2];
    double m10 = M[1][0];
    double m11 = M[1][1];
    double m12 = M[1][2];
    double m20 = M[2][0];
    double m21 = M[2][1];
    double m22 = M[2][2];

    M[0][0] = (m11 * m22 - m12 * m21) / detM;
    M[0][1] = (m02 * m21 - m01 * m22) / detM;
    M[0][2] = (m01 * m12 - m02 * m11) / detM;
    M[1][0] = (m12 * m20 - m10 * m22) / detM;
    M[1][1] = (m00 * m22 - m02 * m20) / detM;
    M[1][2] = (m02 * m10 - m00 * m12) / detM;
    M[2][0] = (m10 * m21 - m11 * m20) / detM;
    M[2][1] = (m01 * m20 - m00 * m21) / detM;
    M[2][2] = (m00 * m11 - m01 * m10) / detM;
    }

    }  // end namespace Pt

#endif /* pt3D_h */
