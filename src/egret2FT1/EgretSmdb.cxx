/**
 * @file EgretSmdb.cxx
 * @brief Implementation for EGRET summary database file interface class.
 * @author J. Chiang
 *
 * $Header$
 */

#include <cmath>

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "facilities/Util.h"

#include "tip/IFileSvc.h"

#include "st_facilities/Env.h"
#include "st_facilities/Environment.h"
#include "st_facilities/Util.h"

#include "facilities/commonUtilities.h"
#include "dataSubselector/Gti.h"

#include "EgretSmdb.h"

double EgretSmdb::s_mjdref(48361.);

EgretSmdb::EgretSmdb(const std::string & smdbfile) :
   m_table(tip::IFileSvc::instance().readTable(smdbfile, "EGRET_SMDB")),
   m_it(m_table->begin()),
   m_row(*m_it),
   m_nrows(m_table->getNumRecords()),
   m_gti(0) {
   
   dataSubselector::Gti my_gti;
   readEgretGtis(my_gti);

   double start(arrivalTime());
   m_it = end();
   --m_it;
   double stop(arrivalTime());
   m_it = begin();
   m_gti = new dataSubselector::Gti(my_gti.applyTimeRangeCut(start, stop));
}

EgretSmdb::~EgretSmdb() {
   delete m_gti;
   delete m_table;
}

void EgretSmdb::next() {
   ++m_it;
}

double EgretSmdb::operator[](const std::string & fieldname) const {
   return m_row[fieldname].get();
}

tip::Table::ConstIterator EgretSmdb::begin() const {
   return m_table->begin();
}

tip::Table::ConstIterator EgretSmdb::end() const {
   return m_table->end();
}

tip::Table::ConstIterator & EgretSmdb::itor() {
   return m_it;
}

double EgretSmdb::arrivalTime() const {
   double tjd = arrivalTjd();
   double met = (tjd + 4e4 - s_mjdref)*8.64e4;
   return met;
}

double EgretSmdb::arrivalTjd() const {
   double tjd(m_row["trunc_julian_day"].get());
   double msec(m_row["milliseconds"].get());
   double musec(m_row["microseconds"].get());

   return tjd + (msec + musec/1e3)/8.64e7;
}

bool EgretSmdb::tascIn() const {
   char tasc1, tasc2;
   m_row["TASC_#1_FLAGS"].get(tasc1);
   m_row["TASC_#2_FLAGS"].get(tasc2);
   int val = static_cast<int>(tasc1 & 32) + static_cast<int>(tasc2 & 32);
   if (!val) {
      return true;
   }
   return false;
}

bool EgretSmdb::zenAngleCut(double nsigma) const {
   static const double sig0(5.85);
   static const double indx(-0.534);
   double energy(m_row["gamma_ray_energy"].get());
   double maxZenAngle(110. - nsigma*sig0*std::pow(energy/100., indx));
   maxZenAngle = std::min(maxZenAngle, 105.);
   return m_row["zenith_direction"].get()*180./M_PI <= maxZenAngle;
}

int EgretSmdb::eventClass() const {
   char evt_class;
   m_row["ENERGY"].get(evt_class);
   return static_cast<int>(evt_class & 3);
}

void EgretSmdb::setMjdRef(double mjdref) {
   s_mjdref = mjdref;
}

const dataSubselector::Gti & EgretSmdb::gti() const {
   return *m_gti;
}

void EgretSmdb::readEgretGtis(dataSubselector::Gti & gti) {
   std::string infile = facilities::commonUtilities::joinPath(
      st_facilities::Environment::dataPath("fitsGen"), "egret_gtis_tjd.dat");
   std::vector<std::string> lines;
   st_facilities::Util::file_ok(infile);
   st_facilities::Util::readLines(infile, lines);
   std::vector<std::string> tokens;
   for (size_t i = 0; i < lines.size(); i++) {
      facilities::Util::stringTokenize(lines.at(i), " ", tokens);
      double t0((std::atof(tokens.at(0).c_str()) + 4e4 - s_mjdref)*8.64e4);
      double t1((std::atof(tokens.at(1).c_str()) + 4e4 - s_mjdref)*8.64e4);
      gti.insertInterval(t0, t1);
   }
}
