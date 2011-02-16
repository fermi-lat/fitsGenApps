/**
 * @file PsfCut.cxx
 * @brief Functor class for energy-dependent PSF cut for LLE data selection.
 * @author J. Chiang
 *
 * $Header$
 */

#include <stdexcept>

#include "fitsGen/MeritFile.h"

#include "PsfCut.h"

namespace fitsGenApps {

PsfCut::PsfCut(const std::string & ft2file, 
               double src_ra, double src_dec) 
   : m_ft2(new fitsGen::MeritFile(ft2file, "SC_DATA")),
     m_srcdir(src_ra, src_dec), m_theta(0) {
   m_tstart = m_ft2->row()["START"].get();
}

PsfCut::~PsfCut() throw() {
   delete m_ft2;
}

bool PsfCut::operator()(double energy, double time, 
                        double ra, double dec) const {
// Update off-axis angle as a function of time.
   if (time < m_tstart) {
      std::cout << "FT2 start: " << m_tstart << "\n"
                << "event time: " << time << std::endl;
      throw std::runtime_error("Event time not covered by FT2 file");
   }
   if (time > m_ft2->row()["STOP"].get()) {
      while (time > m_ft2->row()["STOP"].get() && 
             m_ft2->itor() != m_ft2->end()) {
         m_ft2->next();
      }
      if (m_ft2->itor() == m_ft2->end()) {
         throw std::runtime_error("Event time not covered by FT2 file");
      }
      astro::SkyDir zAxis(m_ft2->row()["RA_SCZ"].get(), 
                          m_ft2->row()["DEC_SCZ"].get());
      m_theta = zAxis.difference(m_srcdir)*180./M_PI;
   }

   // astro::SkyDir dir(ra, dec);
   // double theta = dir.difference(m_srcdir)*180./M_PI;

   /// This unfortunate calculation of the photon-source offset angle
   /// is from one of the original LLE macros and probably was used in
   /// deriving the theta68 parameters.
   double theta = 
      std::sqrt(std::pow(std::cos(dec*0.0174533)*(ra - m_srcdir.ra()), 2.) 
                + std::pow((dec - m_srcdir.dec()), 2.));
   return theta <= theta68(energy);
}

double PsfCut::theta68(double energy) const {
   double Eb, Nb, low_sl, hi_sl;
   if (m_theta > 40) {
      Eb = 100.;
      Nb = 10.5;
      low_sl = -0.65;
      hi_sl = -0.81;
   } else {
      Eb = 59.;
      Nb = 11.5;
      low_sl = -0.55;
      hi_sl = -0.87;
   }
   return Nb*std::min(std::pow(energy/Eb, low_sl), std::pow(energy/Eb, hi_sl));
}

} // namespace fitsGenApps
