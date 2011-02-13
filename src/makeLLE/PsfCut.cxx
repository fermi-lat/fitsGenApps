/**
 * @file PsfCut.cxx
 * @brief Functor class for energy-dependent PSF cut for LLE data selection.
 * @author J. Chiang
 *
 * $Header$
 */

#include <stdexcept>

#include "tip/Table.h"

#include "fitsGen/MeritFile.h"

#include "PsfCut.h"

namespace fitsGenApps {

PsfCut::PsfCut(const std::string & ft2file, 
               double src_ra, double src_dec) 
   : m_ft2(new fitsGen::MeritFile(ft2file, "SC_DATA")),
     m_srcdir(src_ra, src_dec), m_theta(0), 
     m_ra("FT1Ra"), m_dec("FT1Dec"),
     m_time("EvtElapsedTime"), m_energy("EvtEnergyCorr") {}

PsfCut::~PsfCut() throw() {
   delete m_ft2;
}

bool PsfCut::operator()(tip::ConstTableRecord & row) const {
   double time = row[m_time].get();
   if (time > m_ft2->row()["STOP"].get()) {
      while (time > m_ft2->row()["STOP"].get() && 
             m_ft2->itor() != m_ft2->end()) {
         m_ft2->next();
      }
      if (m_ft2->itor() == m_ft2->end()) {
         throw std::runtime_error("Event time not covered by FT2 file");
      }
      astro::SkyDir zAxis(row["RA_SCZ"].get(), row["DEC_SCZ"].get());
      m_theta = zAxis.difference(m_srcdir)*180./M_PI;
   }

   astro::SkyDir dir(row[m_ra].get(), row[m_dec].get());
   double theta = dir.difference(m_srcdir)*180./M_PI;

   return theta < theta68(row[m_energy].get());
}

double PsfCut::theta68(double energy) const {
   double Eb = 59.;
   double Nb = 11.5;
   double low_sl = -0.55;
   double hi_sl = -0.87;
   if (m_theta > 40) {
      Eb = 100.;
      Nb = 10.5;
      low_sl = -0.65;
      hi_sl = -0.81;
   }
   return Nb*std::min(pow(energy/Eb, low_sl), pow(energy/Eb, hi_sl));
}

} // namespace fitsGenApps
