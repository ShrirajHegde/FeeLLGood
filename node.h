#ifndef node_h
#define node_h

/** \file node.h
\brief header to define struct Node
*/

//#include <memory>
#include <iostream>
#include <eigen3/Eigen/Dense>
#include "config.h"

/**
 \namespace Nodes
 to grab altogether some dedicated functions and data of the nodes
 */
namespace Nodes
    {
     /** space dimension */
    const int DIM = 3;

    /** convenient enum to specify what coordinate in calculations */
    enum index
        {
        IDX_UNDEF = -1,
        IDX_X = 0,
        IDX_Y = 1,
        IDX_Z = 2
        };

/** \return \f$ x^2 \f$ */
inline double sq(const double x) { return x * x; }

/** \struct Node
Node is containing physical point of coordinates \f$ p = (x,y,z) \f$, magnetization value at \f$
m(p,t) \f$. Many other values for the computation of the scalar potential \f$ \phi \f$
*/
struct Node
    {
    Eigen::Vector3d p;  /**< Physical position p=(x,y,z)  of the node */
    Eigen::Vector3d u0; /**< magnetization at the start of the current time step */
    Eigen::Vector3d v0; /**< magnetization speed at the start of the current time step */
    Eigen::Vector3d u;  /**< magnetization after the current time step */
    Eigen::Vector3d v;  /**< magnetization speed after the current time step */

    Eigen::Vector3d ep; /**< local vector basis : \f$ e_p = \vec{rand} \times u0 \f$ , then normalized */
    Eigen::Vector3d eq; /**< local vector basis : \f$ e_q = u0 \times e_p \f$ , then normalized */

    double phi0;  /**< scalar potential at the start of the current time step */
    double phi;   /**< scalar potential after the current time step */
    double phiv0; /**< scalar potential of velocity at the start of the current time step */
    double phiv;  /**< scalar potential of velocity after the current time step */

    /** setter for the local basis vector */
    inline void setBasis(const double r)
        {
        // Choose for an initial ep the direction, among (X, Y, Z), which is further away from u0.
        // devNote: Eigen documentation recommends NOT to use ternary operations (see General Topics/common pitfalls)
        double abs_x = fabs(u0.x()), abs_y = fabs(u0.y()), abs_z = fabs(u0.z());
        if (abs_x < abs_y)
            {
            if (abs_x < abs_z)
                { ep = Eigen::Vector3d::UnitX(); }
            else
                { ep = Eigen::Vector3d::UnitZ(); }
            }
        else
            {
            if (abs_y < abs_z)
                { ep = Eigen::Vector3d::UnitY(); }
            else
                { ep = Eigen::Vector3d::UnitZ(); }
            }

        // Gram-Schmidt orthonormalization of (u0, ep).
        ep -= ep.dot(u0) * u0;
        ep.normalize();

        // Complete the basis with a vector product.
        eq = u0.cross(ep);

        // Rotate (ep, eq) by the random angle.
        Eigen::Vector3d new_ep = cos(r) * ep - sin(r) * eq;
        eq = sin(r) * ep + cos(r) * eq;
        ep = new_ep;

        // The basis (u0, ep, eq) should already be orthonormal. An extra orthonormalization could
        // reduce the rounding errors.
        if (PARANOID_ORTHONORMALIZATION)
            {
            // Modified Gram-Schmidt orthonormalization.
            ep -= ep.dot(u0) * u0;
            ep.normalize();
            eq -= eq.dot(u0) * u0;
            eq -= eq.dot(ep) * ep;
            eq.normalize();
            }
        }

    /**
    preparation of the quantities u0,v0,phi0,phiv0 for incomming time-step
    */
    inline void evolution(void)
        {
        u0 = u;
        v0 = v;
        phi0 = phi;
        phiv0 = phiv;
        }

    /**
    integration of the evolution of the magnetization for time step dt
    in a base composed of u0,ep,eq = u0*ep we have
    \f$ v = v_p e_p + v_q e_q \f$
    and new magnetization value is : \f$ u = u_0 + v dt \f$ after normalization
    */

    inline void make_evol(const double vp /**< [in] */, const double vq /**< [in] */,
                          const double dt /**< [in] */)
        {
        v = vp * ep + vq * eq;
        u = u0 + dt * v;
        u.normalize();
        }

    };  // end struct node

/** getter for p */
inline Eigen::Vector3d get_p(Node const &n /**< [in] */) { return n.p; }

/** getter for u0*/
inline const Eigen::Vector3d get_u0(Node const &n /**< [in] */) { return n.u0; }

/** getter for v0*/
inline const Eigen::Vector3d get_v0(Node const &n /**< [in] */) { return n.v0; }

/** getter for u */
inline const Eigen::Vector3d get_u(Node const &n /**< [in] */) { return n.u; }

/** getter for v */
inline const Eigen::Vector3d get_v(Node const &n /**< [in] */) { return n.v; }

/** getter for u component */
inline double get_u_comp(Node const &n /**< [in] */, index idx /**< [in] */)
    {
    return n.u(idx);
    }

/** getter for v component */
inline double get_v_comp(Node const &n /**< [in] */, index idx /**< [in] */)
    {
    return n.v(idx);
    }

/** getter for v0 component */
inline double get_v0_comp(Node const &n /**< [in] */, index idx /**< [in] */)
    {
    return n.v0(idx);
    }

/** getter for phi */
inline double get_phi(Node const &n /**< [in] */) { return n.phi; }

/** getter for phi0 */
inline double get_phi0(Node const &n /**< [in] */) { return n.phi0; }

/** getter for phiv0 */
inline double get_phiv0(Node const &n /**< [in] */) { return n.phiv0; }

/** setter for phi */
inline void set_phi(Node &n, double val) { n.phi = val; }

/** setter for phi_v */
inline void set_phiv(Node &n, double val) { n.phiv = val; }

    }  // end namespace Nodes

#endif /* node_h */
