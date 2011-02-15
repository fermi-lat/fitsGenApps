/**
 * @file PsfCut.h
 * @brief Functor class for energy-dependent PSF cut for LLE data selection.
 * @author J. Chiang
 *
 * $Header$
 */

#ifndef fitsGenApps_PsfCut_h
#define fitsGenApps_PsfCut_h

#include <string>

#include "astro/SkyDir.h"

namespace fitsGen {
   class MeritFile;
   class MeritFile2;
}

namespace fitsGenApps {

class PsfCut {

public:

   PsfCut(const std::string & ft2file, double ra, double dec);

   ~PsfCut() throw();

   bool operator()(const fitsGen::MeritFile2 & merit) const;

private:

   fitsGen::MeritFile * m_ft2;

   astro::SkyDir m_srcdir;
   
   mutable double m_theta;

   std::string m_ra;
   std::string m_dec;
   std::string m_time;
   std::string m_energy;

   double m_tstart;

   double theta68(double energy) const;

};

} // namespace fitsGenApps

#endif // fitsGenApps_PsfCut_h
