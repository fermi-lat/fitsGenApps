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
#include "fitsGen/MeritFile2.h"

#include "PsfCut.h"

namespace fitsGenApps {

PsfCut::PsfCut(const std::string & ft2file, 
               double src_ra, double src_dec) 
   : m_ft2(new fitsGen::MeritFile(ft2file, "SC_DATA")),
     m_srcdir(src_ra, src_dec), m_theta(0), 
     m_ra("FT1Ra"), m_dec("FT1Dec"),
     m_time("EvtElapsedTime"), m_energy("EvtEnergyCorr") {
   m_tstart = m_ft2->row()["START"].get();
}

PsfCut::~PsfCut() throw() {
   delete m_ft2;
}

bool PsfCut::operator()(const fitsGen::MeritFile2 & merit) const {
   double time = merit[m_time];
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

//   std::cout << time << "  " << m_theta << std::endl;
   astro::SkyDir dir(merit[m_ra], merit[m_dec]);
   double theta = dir.difference(m_srcdir)*180./M_PI;
   
   // double ra = merit[m_ra];
   // double dec = merit[m_dec];
   // double theta = 
   //    std::sqrt(std::pow(std::cos(dec*0.0174533)*(ra - m_srcdir.ra()), 2.) 
   //              + std::pow((dec - m_srcdir.dec()), 2.));

   return theta < theta68(merit[m_energy]);
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
