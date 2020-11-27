#ifndef LBO_H
#define LBO_H
// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the LBO_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// LBO_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
//#ifdef LBO_EXPORTS
//#define LBO_API __declspec(dllexport)
//#else
//#define LBO_API __declspec(dllimport)
//#endif

#define LBO_API

#include <vector>
#include "lp_openmesh.h"

#include "Mesh.h"
//#include <Eigen/Eigen/Sparse>

// This class is exported from the LBO.dll
class LBO_API LBO {
public:
//	typedef Eigen::SparseMatrix<double> SparseMatrix;
//	typedef Eigen::Triplet<double> Triplet;

    LBO();
    void AssignMesh(CMesh *mesh_);
    void Construct();

    const std::vector<double>& LMatrix() { return mL; }
    const std::vector<double>& AMatrix() { return mA; }

private:
    double CalcCotWeight(unsigned edgeIndex);
    void ConstructLaplacian();
    void ConstructAreaDiag();

private:
    CMesh *mMesh;
    std::vector<double> mL, mA;
//	SparseMatrix L, A;
};

// extern LBO_API int nLBO;
//
// LBO_API int fnLBO(void);

#endif // LBO_H
