/**********************************************************************
Copyright (C) 2004 by Chris Morley

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
***********************************************************************/

/* This is a heavily commented template for a OpenBabel format class.

Format classes are plugins: no modifications are needed to existing code
to indroduce a new format. The code just needs to be compiled and linked
with the rest of the OpenBabel code.
Alternatively, they can be built (either singly or in groups) as DLLs
or shared libraries. [**Extra build info**]

Each file may contain more than one format.

This compilable, but non-functional example is for a format which
converts a molecule to and from OpenBabel's internal format OBMol.
The conversion framework can handle other types of object, provided
they are derived from OBBase, such as OBReaction, OBText.

For XML formats, extra support for the parsing is provided, see pubchem.cpp
as an example.
*/

#include <openbabel/babelconfig.h>
#include <openbabel/obmolecformat.h>
#include <openbabel/math/align.h>

using namespace std;
namespace OpenBabel
{

class ConfabReport : public OBMoleculeFormat
// Derive directly from OBFormat for objects which are not molecules.
{
public:
  //Register this format type ID in the constructor
  ConfabReport()
  {
    OBConversion::RegisterFormat("confabreport",this);
    const double binvals_arr[8] = {0.2, 0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 100.0};
    binvals = vector<double>(binvals_arr, binvals_arr+8);
    cutoff_passed = 0;
    N = 0;
    oldtitle = "";
    rmsd_cutoff = 0.5;
  }

  virtual const char* Description() //required
  {
    return
    "Confab report format\n"
    "Some comments here, on as many lines as necessay\n"
    "Write Options e.g. -xf3 \n"
    "  r <rmsd> RMSD cutoff (default 0.5)\n"
    "  f <filename> Reference file name\n\n"
    ;
  };

  virtual unsigned int Flags()
  {
    return NOTREADABLE;
  };

  ////////////////////////////////////////////////////
  /// Declarations for the "API" interface functions. Definitions are below
  virtual bool WriteMolecule(OBBase* pOb, OBConversion* pConv);

private:
  void WriteOutput(ostream&);
  ifstream rfs;
  const char *rfilename;
  OBConversion rconv;
  vector<double> binvals;
  OBAlign align;
  OBMol rmol;
  unsigned int cutoff_passed;
  unsigned int N;
  string oldtitle;
  vector<double> rmsd;
  double rmsd_cutoff;
};
  ////////////////////////////////////////////////////

//Make an instance of the format class
ConfabReport theConfabReport;

/////////////////////////////////////////////////////////////////

void ConfabReport::WriteOutput(ostream& ofs)
{
  if (!rmsd.empty()) {
    sort(rmsd.begin(), rmsd.end());
    ofs << "..minimum rmsd = " << rmsd.at(0) << "\n";

    int bin_idx = 0;
    vector<int> bins(binvals.size());
    for(vector<double>::iterator it=rmsd.begin(); it!=rmsd.end(); ++it) {
      while (*it > binvals[bin_idx])
        bin_idx++;
      bins[bin_idx]++;
    }

    vector<int> cumbins(bins);
    for(int i=1; i<8; ++i)
      cumbins[i] += cumbins[i-1];

    ofs << "..confs less than cutoffs: ";
    ofs << binvals[0];
    for (int i=1; i < binvals.size(); i++)
      ofs << " " << binvals[i];
    ofs << "\n";

    ofs << ".." << cumbins[0];
    for (int i=1; i < cumbins.size(); i++)
      ofs << " " << cumbins[i];
    ofs << "\n";

    ofs << "..cutoff (" << rmsd_cutoff << ") passed = ";
    if (rmsd.at(0) <= rmsd_cutoff) {
      ofs << " Yes\n";
      cutoff_passed++;
    }
    else
      ofs << " No\n";
    ofs << "\n";
  }
}

bool ConfabReport::WriteMolecule(OBBase* pOb, OBConversion* pConv)
{
  OBMol* pmol = dynamic_cast<OBMol*>(pOb);
  if(pmol==NULL)
      return false;

  ostream& ofs = *pConv->GetOutStream();
  bool firstMol = pConv->GetOutputIndex()==1;
  if(firstMol) {
    rfilename = pConv->IsOption("f");
    if (!rfilename) {
      cerr << "Need to specify a reference file\n";
      return false;
    }
    OBFormat *prFormat = NULL;
    if (!prFormat) {
      prFormat = rconv.FormatFromExt(rfilename);
      if (!prFormat) {
        cerr << "Cannot read reference format!" << endl;
        return false;
      }
    }
    rfs.open(rfilename);
    if (!rfs) {
      cerr << "Cannot read reference file!" << endl;
      return false;
    }
    rconv.SetInStream(&rfs);
    rconv.SetInFormat(prFormat);
    ofs << "**Generating Confab Report " << endl;
    ofs << "..Reference file = " << rfilename << endl;
    ofs << "..Conformer file = " << pConv->GetInFilename() << "\n\n";

    //rconv.Read(&rmol);
  }

  string title = pmol->GetTitle();
  if (oldtitle != title) {
    
    if (!firstMol)
      ofs << "..number of confs = " << rmsd.size() << "\n";
    WriteOutput(ofs);
    bool success = rconv.Read(&rmol);
    if (!success) return false;
    N++;
    while (rmol.GetTitle() != title) {
      ofs << "..Molecule " << N
           << "\n..title = " << rmol.GetTitle()
           << "\n..number of confs = 0\n";
      N++;
      success = rconv.Read(&rmol);
      if (!success) return false;
    }
    align.SetRefMol(rmol);
    ofs << "..Molecule " << N << "\n..title = " << rmol.GetTitle() << "\n";
    rmsd.clear();
  }

  align.SetTargetMol(*pmol);
  align.Align();
  rmsd.push_back(align.GetRMSD());

  oldtitle = title;

  if (pConv->IsLast()) {
    ofs << "..number of confs = " << rmsd.size() << "\n";
    WriteOutput(ofs);
    ofs << "\n**Summary\n..number of molecules = " << N
        << "\n..less than cutoff(" << rmsd_cutoff << ") = " << cutoff_passed << "\n";
  }

  return true;
}

} //namespace OpenBabel

