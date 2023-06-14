#ifndef element_h
#define element_h

#include <execution>
#include "node.h"

/** \class element
template class, mother class for tetraedrons and facettes
*/

template <int N,int NPI>
class element
    {
    /** constructor */
    public:
    element(const std::vector<Nodes::Node> &_p_node /**< vector of nodes */) : refNode(_p_node)
        {
        ind.resize(N);
        }

    /** indices to the nodes */
    std::vector<int> ind;
    
    /** matrix for integrales */
    double Kp[2*N][2*N];

    /** vector for integrales */
    double Lp[2*N];

    /** index setter */
    inline void set_ind(std::initializer_list<int> & _i)
        {
        ind.assign(_i.begin(),_i.end());
        }
    
    /** zeroBasing : index convention Matlab/msh (one based) -> C++ (zero based) */
    inline void zeroBasing(void)
        {
        std::for_each(ind.begin(), ind.end(), [] (int & idx) {idx--;});
        /* does not link for some obscure tbb reason ??
        std::transform(std::execution::par, ind.begin(), ind.end(), ind.begin(),
                         [](int idx) -> int {return idx-1;} );
        */
        }

    /** getter for N */
    inline constexpr int getN(void) const { return N; }

    /** getter for NPI */
    inline constexpr int getNPI(void) const { return NPI; }

    /** getter for node */
    inline const Nodes::Node &getNode(const int i)
        { return refNode[ind[i]]; }
    
    /** set all indices to zero */
    inline void indicesToZero(void)
        { std::fill(ind.begin(),ind.end(),0); }

    /** print the node indices of the element */
    void print_indices(void) const
        {
        std::cout << '(';
        for(unsigned int i = 0; i < N-1; i++)
            { std::cout << ind[i] << ", "; }
        std::cout << ind[N-1] << ")\n";
        }

    protected:
        /** vector of nodes */
        const std::vector<Nodes::Node> &refNode;
    
    private:
        /** a method to orientate the element must be provided in derived class */
        virtual void orientate() = 0;
    };
    
#endif