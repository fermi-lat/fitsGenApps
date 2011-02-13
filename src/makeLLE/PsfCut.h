/**
 * @file PsfCut.h
 * @brief Functor class for energy-dependent PSF cut for LLE data selection.
 * @author J. Chiang
 *
 * $Header$
 */

#include <string>

#include "astro/SkyDir.h"

namespace tip {
   class ConstTableRecord;
}

namespace fitsGen {
   class MeritFile;
}

namespace fitsGenApps {

class PsfCut {

public:

   PsfCut(const std::string & ft2file, double ra, double dec);

   ~PsfCut() throw();

   bool operator()(tip::ConstTableRecord & row) const;

private:

   fitsGen::MeritFile * m_ft2;

   astro::SkyDir m_srcdir;
   
   mutable double m_theta;

   std::string m_ra;
   std::string m_dec;
   std::string m_time;
   std::string m_energy;

   double theta68(double energy) const;

};

} // namespace fitsGenApps
