#include "fem.h"

void savesol(Fem &fem,string baseName,double s, int nt, string *filename)
{
string str;

if (filename) { str = *filename; }
else{
    str = baseName + "_" + to_string(fem.SEQ) +"_B" + to_string(fem.Bext) + "_iter" + to_string(nt) + ".sol";
 //<< boost::format("_%d_B%6f_iter%d.sol") % fem.SEQ % fem.Bext % nt;
    }
IF_VERBOSE(fem) cout << " " << str << endl;

ofstream fout(str, ios::out);
if (!fout){
   IF_VERBOSE(fem) cerr << "pb ouverture fichier " << str << "en ecriture" << endl;
   SYSTEM_ERROR;}
//fout << boost::format("#time : %+20.10e ") % fem.t << endl;
fout << "#time : " << fem.t <<endl;

const int    NOD   = fem.NOD;

for (int i=0; i<NOD; i++){
    Node &node = fem.node[i];
    double x   = node.x / s;
    double y   = node.y / s;
    double z   = node.z / s;
    double u1  = node.u[0];
    double u2  = node.u[1];
    double u3  = node.u[2];
    double phi = node.phi;
 
//   fout << boost::format("%8d %+20.10f %+20.10f %+20.10f %+20.10f %+20.10f %+20.10f %+20.10e") 
//                   % i % x % y % z % u1 % u2 % u3 % phi << endl;}
	fout << i << "\t" << x << "\t" << y << "\t" << z << "\t";
	fout << u1 << "\t" << u2 << "\t" << u3 << "\t" << phi << endl;
	}

fout.close();
}
