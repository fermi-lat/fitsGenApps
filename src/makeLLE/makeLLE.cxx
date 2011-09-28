/**
 * @file makeLLE.cxx
 * @brief Filter merit or LLE input and output as LLE format.
 * @author J. Chiang
 *
 * $Header$
 */

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "facilities/Util.h"
#include "facilities/commonUtilities.h"

#include "astro/SkyDir.h"

#include "st_stream/StreamFormatter.h"

#include "st_app/AppParGroup.h"
#include "st_app/StApp.h"
#include "st_app/StAppFactory.h"

#include "dataSubselector/Gti.h"
#include "dataSubselector/Cuts.h"

#include "st_facilities/Env.h"
#include "st_facilities/FitsUtil.h"
#include "st_facilities/Util.h"

#include "fitsGen/Ft1File.h"
#include "fitsGen/MeritFile2.h"

#include "PsfCut.h"

using namespace fitsGen;

namespace {
   class LLEEntry {
   public:
      LLEEntry() : m_lleName(""), m_meritName(""), m_lleType("") {}
      LLEEntry(const std::string & line) {
         std::vector<std::string> tokens;
         facilities::Util::stringTokenize(line, " \t", tokens);
         m_lleName = tokens.at(0);
         m_meritName = tokens.at(1);
         if (tokens.size() == 2) {
            m_lleType = "E";
         } else {
            m_lleType = tokens.at(2);
         }
      }
      const std::string & lleName() const {
         return m_lleName;
      }
      const std::string & meritName() const {
         return m_meritName;
      }
      const std::string & lleType() const {
         return m_lleType;
      }
      void write_TLMIN_TLMAX(Ft1File & lle) const {
         int fieldIndex(lle.fieldIndex(m_lleName));
         std::ostringstream tlmin, tlmax;
         tlmin << "TLMIN" << fieldIndex;
         tlmax << "TLMAX" << fieldIndex;
         if (m_lleType == "I") {
            lle.header()[tlmin.str()].set("-32768");
            lle.header()[tlmax.str()].set("32767");
         } else if (m_lleType == "J") {
            lle.header()[tlmin.str()].set("-2147483648");
            lle.header()[tlmax.str()].set("2147483647");
         } else if (m_lleType == "E") {
            lle.header()[tlmin.str()].set("-1.0E+10");
            lle.header()[tlmax.str()].set("1.0E+10");
         } else if (m_lleType == "D") {
            lle.header()[tlmin.str()].set("-1.0D+10");
            lle.header()[tlmax.str()].set("1.0D+10");
         }
      }
   private:
      std::string m_lleName;
      std::string m_meritName;
      std::string m_lleType;
   };

   typedef std::map<std::string, LLEEntry> LLEMap_t;

   void toLower(std::string & name) {
      for (std::string::iterator it = name.begin(); it != name.end(); ++it) {
         *it = std::tolower(*it);
      }
   }

   void getLLEDict(const std::string & inputFile, LLEMap_t & lleDict) {
      lleDict.clear();
      std::vector<std::string> lines;
      st_facilities::Util::readLines(inputFile, lines, "#", true);
      for (size_t i(0); i < lines.size(); i++) {
         LLEEntry entry(lines.at(i));
         lleDict[entry.lleName()] = entry;
      }
   }

   std::string filterString(const std::string & filterFile) {
      std::ostringstream filter;
      std::vector<std::string> lines;
      st_facilities::Util::readLines(filterFile, lines, "#", true);
      for (size_t i = 0; i < lines.size(); i++) {
         filter << lines.at(i);
      }
      return filter.str();
   }

   void addNeededFields(Ft1File & lle, const LLEMap_t & lleDict) {
      const std::vector<std::string> & validFields(lle.getFieldNames());
      LLEMap_t::const_iterator lle_entry;
      for (lle_entry = lleDict.begin(); lle_entry != lleDict.end();
           ++lle_entry) {
         std::string candidate(lle_entry->first);
         toLower(candidate);
         if (std::find(validFields.begin(), validFields.end(), 
                       candidate) == validFields.end()) {
            lle.appendField(lle_entry->first, lle_entry->second.lleType());
         }
      }
   }
} // anonymous namespace

class MakeLLE : public st_app::StApp {
public:
   MakeLLE() : st_app::StApp(),
               m_pars(st_app::StApp::getParGroup("makeLLE")) {
      try {
         setVersion(s_cvs_id);
      } catch (std::exception & eObj) {
         std::cerr << eObj.what() << std::endl;
         std::exit(1);
      } catch (...) {
         std::cerr << "Caught unknown exception in MakeLLE constructor." 
                   << std::endl;
         std::exit(1);
      }
   }
   virtual ~MakeLLE() throw() {
      try {
      } catch (std::exception &eObj) {
         std::cerr << eObj.what() << std::endl;
      } catch (...) {
      }
   }
   virtual void run();
   virtual void banner() const;

private:
   st_app::AppParGroup & m_pars;
   static std::string s_cvs_id;
};

std::string MakeLLE::s_cvs_id("$Name$");

st_app::StAppFactory<MakeLLE> myAppFactory("makeLLE");

void MakeLLE::banner() const {
   int verbosity = m_pars["chatter"];
   if (verbosity > 2) {
      st_app::StApp::banner();
   }
}

void MakeLLE::run() {
   m_pars.Prompt();
   m_pars.Save();
   std::string infile = m_pars["infile"];
   std::string outfile = m_pars["outfile"];
   std::string newFilter = m_pars["TCuts"];
   double zmax = m_pars["zmax"];
//   ::toLower(newFilter);
   double t0 = m_pars["t0"];
   double dtstart = m_pars["dtstart"];
   double dtstop = m_pars["dtstop"];
   bool apply_psf = m_pars["apply_psf"];

   double tmin(t0 + dtstart);
   double tmax(t0 + dtstop);

   std::string dataDir(facilities::commonUtilities::getDataPath("fitsGen"));

// Standard filter string for LLE, allowing for non-default option.
   std::string filter("(TkrNumTracks>0) && " 
                      "(GltEngine==6 || GltEngine==7) && "
                      "(EvtEnergyCorr>0)");
   bool mc_data = m_pars["mc_data"];
   if (mc_data) {
      filter = "(ObfGamState==0) && " + filter;
   } else {
      filter = "(FswGamState==0) && " + filter;
   }
   if (newFilter != "none" && newFilter != "") {
      filter = newFilter;
   }

// Add zenith angle cut.
   std::ostringstream zenangle_cut;
   zenangle_cut << " && (FT1ZenithTheta<" << zmax << ")";
   filter += zenangle_cut.str();

// Add time cut.
   std::ostringstream time_cut;
// // Round lower bound downwards, upper bound upwards.
//    tmin = static_cast<double>(static_cast<long>(tmin));
//    tmax = static_cast<double>(static_cast<long>(tmax)) + 1.;
   time_cut << std::setprecision(14);
   time_cut << " && (EvtElapsedTime >= " << tmin << ") "
            << " && (EvtElapsedTime <= " << tmax << ")";
   filter += time_cut.str();

   st_stream::StreamFormatter formatter("MakeLLE", "run", 2);
   formatter.info() << "applying TCut: " << filter << std::endl;

   std::string dictFile = m_pars["dict_file"];

   ::LLEMap_t lleDict;
   ::getLLEDict(dictFile, lleDict);

   std::string ft2file = m_pars["scfile"];
   double ra = m_pars["ra"];
   double dec = m_pars["dec"];
   fitsGenApps::PsfCut psf_cut(ft2file, ra, dec);

   dataSubselector::Cuts my_cuts;
   Ft1File lle(outfile, 0, "EVENTS", "lle.tpl");

   MeritFile2 * merit_ptr(0);
   if (infile.find("@") == 0) {
      std::vector<std::string> merit_files;
      st_facilities::Util::readLines(infile.substr(1), merit_files);
      merit_ptr = new MeritFile2(merit_files, "MeritTuple", filter);
   } else {
      merit_ptr = new MeritFile2(infile, "MeritTuple", filter);
   }
   MeritFile2 & merit(*merit_ptr);
      
   lle.setObsTimes(tmin, tmax);
   dataSubselector::Gti gti;
   gti.insertInterval(tmin, tmax);
   
   ::addNeededFields(lle, lleDict);
   
   lle.setNumRows(merit.nrows());
      
   lle.header().addHistory("Input file: " + infile);
   lle.header().addHistory("Filter string: " + filter);
   
   int ncount(0);
   do {
      double time = merit["EvtElapsedTime"];
      double event_id = merit["EvtEventId"];
      double energy = merit["EvtEnergyCorr"];
      double ra = merit["FT1Ra"];
      double dec = merit["FT1Dec"];
      if (gti.accept(time) && (!apply_psf || psf_cut(energy, time, ra, dec))) {
         for (::LLEMap_t::const_iterator variable = lleDict.begin();
              variable != lleDict.end(); ++variable) {
            lle[variable->first].set(merit[variable->second.meritName()]);
         }
         lle.next();
         ncount++;
      }
   } while (merit.next() != merit.nrows());
   formatter.info() << "Number of events accepted: " << ncount << std::endl;
   lle.setNumRows(ncount);
   my_cuts.addGtiCut(gti);
   my_cuts.writeDssKeywords(lle.header());

   std::ostringstream creator;
   creator << "makeLLE " << getVersion();
   lle.setPhduKeyword("CREATOR", creator.str());
   std::string version = m_pars["file_version"];
   lle.setPhduKeyword("VERSION", version);
   std::string filename(facilities::Util::basename(outfile));
   lle.setPhduKeyword("FILENAME", filename);
   unsigned int proc_ver = m_pars["proc_ver"];
   lle.setPhduKeyword("PROC_VER", proc_ver);
   
   my_cuts.writeGtiExtension(outfile);
   st_facilities::FitsUtil::writeChecksums(outfile);

   delete merit_ptr;
}
