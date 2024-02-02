/** \file electrostatSolver.h
  \brief solver for electrostatic problem when STT is required
  header containing electrostatSolver class. It uses biconjugate stabilized gradient with diagonal
  preconditioner. The solver is only called once to compute voltages V for each nodes of the mesh,
  when STT computation is involved.
 */

#include <iostream>
#include <map>

#include "config.h"
#include "fem.h"
#include "mesh.h"

#include "facette.h"
#include "tetra.h"

#include "spinTransferTorque.h"

/** assemble the matrix K from tet and Ke inputs */
inline void assembling_mat(Tetra::Tet const &tet, double Ke[Tetra::N][Tetra::N], std::vector<Eigen::Triplet<double>> &K)
    {
    for (int ie = 0; ie < Tetra::N; ie++)
        {
        for (int je = 0; je < Tetra::N; je++)
            {
            double val = Ke[ie][je];
            if (val != 0)
                { K.push_back( Eigen::Triplet(tet.ind[ie], tet.ind[je], val) ); }
            }
        }
    }

/** assemble the vector L from fac and Le inputs */
inline void assembling_vect(Facette::Fac const &fac, std::vector<double> const &Le, Eigen::Ref<Eigen::VectorXd> L)
    {
    for (int ie = 0; ie < Facette::N; ie++)
        { L(fac.ind[ie]) += Le[ie]; }
    }

/** compute side problem (electrostatic potential on the nodes) integrales for matrix
 * coefficients,input from tet ; sigma is the region conductivity */
void integrales(Tetra::Tet const &tet, double sigma, double AE[Tetra::N][Tetra::N])
    {
    for (int npi = 0; npi < Tetra::NPI; npi++)
        {
        double w = tet.weight[npi];

        for (int ie = 0; ie < Tetra::N; ie++)
            {
            double dai_dx = tet.dadx(ie,npi);
            double dai_dy = tet.dady(ie,npi);
            double dai_dz = tet.dadz(ie,npi);

            for (int je = 0; je < Tetra::N; je++)
                {
                double daj_dx = tet.dadx(je,npi);
                double daj_dy = tet.dady(je,npi);
                double daj_dz = tet.dadz(je,npi);
                AE[ie][je] += sigma * (dai_dx * daj_dx + dai_dy * daj_dy + dai_dz * daj_dz) * w;
                }
            }
        }
    }

/** compute integrales for vector coefficients, input from facette */
void integrales(Facette::Fac const &fac, double pot_val, std::vector<double> &BE)
    {
    for (int npi = 0; npi < Facette::NPI; npi++)
        for (int ie = 0; ie < Facette::N; ie++)
            {
            BE[ie] -= Facette::a[ie][npi] * pot_val * fac.weight[npi];
            }
    }

/** \class electrostatSolver
this class is containing both data and a solver to compute potential from dirichlet boundary
conditions problem for the current density flowing in the sample.
*/
class electrostatSolver
    {
public:
    /** constructor */
    inline electrostatSolver(
            Mesh::mesh const &_msh /**< [in] reference to the mesh */,
            STT const &_p_stt /**< all spin transfer torque parameters */,
            const double _tol /**< [in] tolerance for solvers */,
            const bool v /**< [in] verbose bool */,
            const int max_iter /**< [in] maximum number of iteration */,
            const std::string
                    _fileName /**<  [in] output .sol file name for electrostatic potential */)
        : msh(_msh), p_stt(_p_stt), verbose(v), MAXITER(max_iter), precision(PRECISION_STT),
          fileName(_fileName)
        {
        ksi = Nodes::sq(p_stt.lJ / p_stt.lsf);
        D0 = 2.0 * p_stt.sigma / (Nodes::sq(CHARGE_ELECTRON) * p_stt.N0);
        pf = Nodes::sq(p_stt.lJ) / (D0 * (1. + ksi * ksi)) * BOHRS_MUB * p_stt.beta / CHARGE_ELECTRON;

        if (verbose)
            {
            std::cout << "Dirichlet boundary conditions..." << std::endl;
            infos();
            }
        bool has_converged = solve(_tol);
        if (has_converged)
            {
            if (p_stt.V_file)
                {
                if (verbose)
                    {
                    std::cout << "writing electrostatic potential solutions to file " << fileName
                              << std::endl;
                    }
                bool iznogood = msh.savesol(precision, fileName, "## columns: index\tV\n", V);
                if (verbose && iznogood)
                    {
                    std::cout << "file " << fileName << " status : " << iznogood << std::endl;
                    }
                }
            std::for_each(msh.tet.begin(), msh.tet.end(),
                          [this](Tetra::Tet const &tet)
                          {
                              std::array<Eigen::Vector3d, Tetra::NPI> _gradV;
                              calc_gradV(tet, _gradV);
                              gradV.push_back(_gradV);

                              std::array<Eigen::Vector3d, Tetra::NPI> _Hm;
                              calc_Hm(tet, _gradV, _Hm);
                              Hm.push_back(_Hm);
                          });
            prepareExtras();
            }
        else
            {
            std::cerr << "Solver (STT) has not converged" << std::endl;
            exit(1);
            }
        }

    /** computes the gradient(V) for tetra tet */
    void calc_gradV(Tetra::Tet const &tet, std::array<Eigen::Vector3d, Tetra::NPI> &_gradV) const
        {
        for (int npi = 0; npi < Tetra::NPI; npi++)
            {
            double vx(0), vy(0), vz(0);
            for (int i = 0; i < Tetra::N; i++)
                {
                const double V_tet = V[tet.ind[i]];
                vx += V_tet * tet.dadx(i,npi);
                vy += V_tet * tet.dady(i,npi);
                vz += V_tet * tet.dadz(i,npi);
                }
            _gradV[npi] = Eigen::Vector3d(vx, vy, vz);
            }
        }

    /** computes Hm contributions for each npi for tetrahedron tet */
    void calc_Hm(Tetra::Tet const &tet, std::array<Eigen::Vector3d, Tetra::NPI> const &_gradV,
                 std::array<Eigen::Vector3d, Tetra::NPI> &_Hm) const
        {
        Eigen::Matrix<double,Nodes::DIM,Tetra::NPI> p_g;
        tet.getPtGauss(p_g);

        for (int npi = 0; npi < Tetra::NPI; npi++)
            { _Hm[npi] = -p_stt.sigma * _gradV[npi].cross(p_g.col(npi)); }
        }

private:
    /** affect extraField function and extraCoeffs_BE function for all the tetrahedrons */
    void prepareExtras(void)
        {
        std::for_each(
                msh.tet.begin(), msh.tet.end(),
                [this](Tetra::Tet &tet)
                {
                    const int _idx = tet.idx;
                    tet.extraField = [this, _idx](Eigen::Ref<Eigen::Matrix<double,Nodes::DIM,Tetra::NPI>> H)
                    {
                    for(int npi = 0; npi<Tetra::NPI; npi++) { H.col(npi) += Hm[_idx][npi]; }
                    };

                    tet.extraCoeffs_BE = [this, &tet](int npi, double Js, Eigen::Ref<Eigen::Vector3d> U,
                                                      Eigen::Ref<Eigen::Vector3d> dUdx,
                                                      Eigen::Ref<Eigen::Vector3d> dUdy,
                                                      Eigen::Ref<Eigen::Vector3d> dUdz,
                                                      Eigen::Ref<Eigen::Matrix<double,Nodes::DIM,Tetra::N>> BE)
                    {
                        const double prefactor = D0 / Nodes::sq(p_stt.lJ) / (gamma0 * nu0 * Js); //nu0*Js shall be replaced by tet.Ms
                        Eigen::Vector3d const &_gV = gradV[tet.idx][npi];
                        Eigen::Vector3d j_grad_u =
                                -p_stt.sigma
                                * Eigen::Vector3d(_gV.dot( Eigen::Vector3d(dUdx(Nodes::IDX_X), dUdy(Nodes::IDX_X), dUdz(Nodes::IDX_X))),
                                                  _gV.dot( Eigen::Vector3d(dUdx(Nodes::IDX_Y), dUdy(Nodes::IDX_Y), dUdz(Nodes::IDX_Y))),
                                                  _gV.dot( Eigen::Vector3d(dUdx(Nodes::IDX_Z), dUdy(Nodes::IDX_Z), dUdz(Nodes::IDX_Z))));

                        Eigen::Vector3d m = pf * (ksi * j_grad_u + U.cross(j_grad_u));
                        Eigen::Vector3d interim[Tetra::N];
                        for (int i = 0; i < Tetra::N; i++)
                            {
                            const double ai_w = tet.weight[npi] * Tetra::a[i][npi];
                            interim[i] = ai_w*(Hm[tet.idx][npi] + prefactor*m);
                            }
                        for (int i = 0; i < Tetra::N; i++)
                            { BE.col(i) += interim[i]; }
                    };
                });
        }

    /** ksi is in Thiaville notations beta_DW */
    double ksi;

    /** density of states */
    double D0;

    /** a prefactor for BE coefficient coefficients*/
    double pf;

    /** mesh object to store nodes, fac, tet, and others geometrical values related to the mesh (
     * const ref ) */
    Mesh::mesh msh;

    /** spin transfer torque parameters */
    STT p_stt;

    /** if verbose set to true, some printing are sent to terminal */
    const bool verbose;

    /** maximum number of iteration for biconjugate stabilized gradient */
    const unsigned int MAXITER;  // fixed to 5000 in ref code

    /** number of digits in the optional output file */
    const int precision;

    /** output .sol file name for electrostatic problem */
    const std::string fileName;

    /** electrostatic potential values for boundary conditions, V.size() is the size of the vector
     * of nodes */
    std::vector<double> V;

    /** table of the gradients of the potential, gradV.size() is the number of tetra */
    std::vector<std::array<Eigen::Vector3d, Tetra::NPI>> gradV;

    /** table of the Hm vectors (contribution of the STT to the tet::integrales) ; Hm.size() is the
     * number of tetra */
    std::vector<std::array< Eigen::Vector3d, Tetra::NPI>> Hm;

    /** basic informations on boundary conditions */
    inline void infos(void)
        {
        std::cout << "sigma: " << p_stt.sigma << std::endl;

        std::for_each(p_stt.boundaryCond.begin(), p_stt.boundaryCond.end(),
                      [](std::pair<std::string, double> const &p)
                      { std::cout << "regName: " << p.first << "\tV :" << p.second << std::endl; });
        }

    /** fill matrix and vector to solve potential values on each node */
    void prepareData(std::vector<Eigen::Triplet<double>> &Kw, Eigen::Ref<Eigen::VectorXd> Lw)
        {
        const double sigma = p_stt.sigma;

        std::for_each(msh.tet.begin(), msh.tet.end(),
                      [this, sigma, &Kw](Tetra::Tet const &tet)
                      {
                          double K[Tetra::N][Tetra::N] = {{0}};
                          integrales(tet, sigma, K);
                          assembling_mat(tet, K, Kw);
                      });

        double pot_val = 0;  // we initialize pot_val to the average of the potentials set by the
                             // boundary conditions (does not seem to change convergence speed
                             // whatever value it is ..)
        std::for_each(p_stt.boundaryCond.begin(), p_stt.boundaryCond.end(),
                      [&pot_val](auto const &it) { pot_val += it.second; });
        pot_val /= p_stt.boundaryCond.size();

        std::for_each(msh.fac.begin(), msh.fac.end(),
                      [this, pot_val, &Lw](Facette::Fac const &fac)
                      {
                          std::vector<double> L(Facette::N);
                          integrales(fac, pot_val, L);
                          assembling_vect(fac, L, Lw);
                      });
        }

    /** solver, using biconjugate stabilized gradient, with diagonal preconditionner and Dirichlet
     * boundary conditions */
    int solve(const double _tol)
        {
        const int NOD = msh.getNbNodes();

        std::vector<Eigen::Triplet<double>> Kw;
        Eigen::VectorXd Lw = Eigen::VectorXd::Zero(NOD);
        prepareData(Kw, Lw);

        Eigen::VectorXd Xw(NOD);

        std::for_each(
                p_stt.boundaryCond.begin(), p_stt.boundaryCond.end(),
                [this, &Kw, &Lw, &Xw](auto const &it)
                {
                    double V = it.second;
                    std::string name = it.first;

                    auto surf_it = std::find_if(msh.s.begin(), msh.s.end(),
                                                [name](Mesh::Surf const &_s)
                                                { return (_s.getName() == name); });

                    if (surf_it != msh.s.end())
                        {
                        std::for_each(
                                surf_it->elem.begin(), surf_it->elem.end(),
                                [V, &Kw, &Lw, &Xw](Mesh::Triangle const &tri)
                                {
                                    for (int ie = 0; ie < Facette::N; ie++)  // should be Triangle::N
                                        {
                                        const int i = tri.ind[ie];
                                        
                                        // first we suppress all diagonal elements
                                        Kw.erase(std::remove_if(Kw.begin(),Kw.end(),[i]( Eigen::Triplet<double> &t)
                                                { return (t.row() == t.col()); } ));
                                        
                                        //then remove all the element we want to be zero
                                        Kw.erase(std::remove_if(Kw.begin(),Kw.end(),[i]( Eigen::Triplet<double> &t)
                                                { return (t.row() == i); } ));
                                        
                                        // then reinsert diagonal with big value
                                        Kw.push_back(Eigen::Triplet<double>(i, i, 1e9));
                                        Lw[i] = V * 1e9;
                                        Xw[i] = V;
                                        }
                                });
                        }
                });

        if (verbose)
            { std::cout << "line weighting..." << std::endl; }

        Eigen::SparseMatrix<double> Kr(NOD,NOD);
        Kr.setFromTriplets(Kw.begin(),Kw.end());
        std::vector<double> maxRow(NOD,0);

        //here we fill the vector maxRow with infinity norm : maxRow[i] = max{|Kr.row(i)|}
        for(int i=0;i<NOD;i++)
            {
            for(int k=0;k<Kr.outerSize();++k)
                for(Eigen::SparseMatrix<double>::InnerIterator it(Kr,k); it; ++it)
                    { if((it.row() == i)&&(fabs(it.value()) > maxRow[i])) { maxRow[i] = fabs(it.value()); } }
            }

        for (int i = 0; i < NOD; i++)
            {
            double norme = maxRow[i];
            
            Lw[i] /= norme;
            Kr.row(i) /= norme;
            }
        
        if (verbose)
            { std::cout << "solving ..." << std::endl; }

        Eigen::BiCGSTAB<Eigen::SparseMatrix<double>> _solver;

        _solver.setTolerance(_tol);
        _solver.setMaxIterations(MAXITER);
        _solver.compute(Kr);
        
        Eigen::VectorXd sol = _solver.solve(Lw); 

        V.resize(NOD);
        for (int i=0;i<NOD;i++)
            { V[i]= sol(i); }
        return (_solver.iterations() < MAXITER);
        }

    };  // end class electrostatSolver
