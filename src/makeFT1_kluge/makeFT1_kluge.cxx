/**
 * @file makeFT1.cxx
 * @brief Convert merit ntuple to FT1 format.
 * @author J. Chiang
 *
 * $Header$
 */

#include <cctype>
#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "facilities/Util.h"

#include "dataSubselector/Gti.h"
#include "dataSubselector/Cuts.h"

#include "astro/SkyDir.h"

#include "st_facilities/Env.h"
#include "st_facilities/FitsUtil.h"
#include "st_facilities/Util.h"

#include "facilities/commonUtilities.h"

#include "st_stream/StreamFormatter.h"

#include "st_app/AppParGroup.h"
#include "st_app/StApp.h"
#include "st_app/StAppFactory.h"

#include "fitsGen/Ft1File.h"
#include "fitsGen/MeritFile.h"
#include "fitsGen/EventClassifier.h"

using namespace fitsGen;

namespace {
   class Ft1Entry {
   public:
      Ft1Entry() : m_ft1Name(""), m_meritName(""), m_ft1Type("") {}
      Ft1Entry(const std::string & line) {
         std::vector<std::string> tokens;
         facilities::Util::stringTokenize(line, " \t", tokens);
         m_ft1Name = tokens.at(0);
         m_meritName = tokens.at(1);
         if (tokens.size() == 2) {
            m_ft1Type = "E";
         } else {
            m_ft1Type = tokens.at(2);
         }
      }
      const std::string & ft1Name() const {
         return m_ft1Name;
      }
      const std::string & meritName() const {
         return m_meritName;
      }
      const std::string & ft1Type() const {
         return m_ft1Type;
      }
      void write_TLMIN_TLMAX(Ft1File & ft1) const {
         int fieldIndex(ft1.fieldIndex(m_ft1Name));
         std::ostringstream tlmin, tlmax;
         tlmin << "TLMIN" << fieldIndex;
         tlmax << "TLMAX" << fieldIndex;
         if (m_ft1Type == "I") {
            ft1.header()[tlmin.str()].set("-32768");
            ft1.header()[tlmax.str()].set("32767");
         } else if (m_ft1Type == "J") {
            ft1.header()[tlmin.str()].set("-2147483648");
            ft1.header()[tlmax.str()].set("2147483647");
         } else if (m_ft1Type == "E") {
            ft1.header()[tlmin.str()].set("-1.0E+10");
            ft1.header()[tlmax.str()].set("1.0E+10");
         } else if (m_ft1Type == "D") {
            ft1.header()[tlmin.str()].set("-1.0D+10");
            ft1.header()[tlmax.str()].set("1.0D+10");
         }
      }
   private:
      std::string m_ft1Name;
      std::string m_meritName;
      std::string m_ft1Type;
   };

   typedef std::map<std::string, Ft1Entry> Ft1Map_t;

   void toLower(std::string & name) {
      for (std::string::iterator it = name.begin(); it != name.end(); ++it) {
         *it = std::tolower(*it);
      }
   }

   void getFT1Dict(const std::string & inputFile, Ft1Map_t & ft1Dict) {
      ft1Dict.clear();
      std::vector<std::string> lines;
      st_facilities::Util::readLines(inputFile, lines, "#", true);
      for (size_t i(0); i < lines.size(); i++) {
         Ft1Entry entry(lines.at(i));
         ft1Dict[entry.ft1Name()] = entry;
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

   void addNeededFields(Ft1File & ft1, const Ft1Map_t & ft1Dict) {
      const std::vector<std::string> & validFields(ft1.getFieldNames());
      Ft1Map_t::const_iterator ft1_entry;
      for (ft1_entry = ft1Dict.begin(); ft1_entry != ft1Dict.end();
           ++ft1_entry) {
         std::string candidate(ft1_entry->first);
         toLower(candidate);
         if (std::find(validFields.begin(), validFields.end(), 
                       candidate) == validFields.end()) {
            ft1.appendField(ft1_entry->first, ft1_entry->second.ft1Type());
//            ft1_entry->second.write_TLMIN_TLMAX(ft1);
         }
      }
   }
}

class MakeFt1 : public st_app::StApp {
public:
   MakeFt1() : st_app::StApp(),
               m_pars(st_app::StApp::getParGroup("makeFT1_kluge")) {
      try {
         setVersion(s_cvs_id);
      } catch (std::exception & eObj) {
         std::cerr << eObj.what() << std::endl;
         std::exit(1);
      } catch (...) {
         std::cerr << "Caught unknown exception in MakeFT1 constructor." 
                   << std::endl;
         std::exit(1);
      }
   }
   virtual ~MakeFt1() throw() {
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

std::string MakeFt1::s_cvs_id("$Name$");

st_app::StAppFactory<MakeFt1> myAppFactory("makeFT1_kluge");

void MakeFt1::banner() const {
   int verbosity = m_pars["chatter"];
   if (verbosity > 2) {
      st_app::StApp::banner();
   }
}

void MakeFt1::run() {
   m_pars.Prompt();
   m_pars.Save();
   std::string rootFile = m_pars["rootFile"];
   std::string fitsFile = m_pars["fitsFile"];
   std::string eventClassifier = m_pars["event_classifier"];
   std::string defaultFilter = m_pars["TCuts"];

   double tstart = m_pars["tstart"];
   double tstop = m_pars["tstop"];

   std::string dataDir(facilities::commonUtilities::getDataPath("fitsGen"));

   std::string filter;
   if (!st_facilities::Util::fileExists(defaultFilter)) {
      filter = defaultFilter;
   } else {
      filter = filterString(defaultFilter);
   }

   if (tstart != 0 || tstop != 0) {
      std::ostringstream time_cut;
// Round lower bound down, upper bound upwards.
      tstart = static_cast<double>(static_cast<long>(tstart));
      tstop = static_cast<double>(static_cast<long>(tstop)) + 1.;
      time_cut << std::setprecision(10);
      time_cut << " && (EvtElapsedTime >= " << tstart << ") "
               << " && (EvtElapsedTime <= " << tstop << ")";
      filter += time_cut.str();
   }

   st_stream::StreamFormatter formatter("MakeFt1", "run", 2);
   formatter.info() << "applying TCut: " << filter << std::endl;

   std::string dictFile = m_pars["dict_file"];

   ::Ft1Map_t ft1Dict;
   ::getFT1Dict(dictFile, ft1Dict);

   dataSubselector::Cuts my_cuts;
   fitsGen::Ft1File ft1(fitsFile, 0);
   try {
      fitsGen::MeritFile merit(rootFile, "MeritTuple", filter);
      if (tstart != 0 || tstop != 0) {
         merit.setStartStop(tstart, tstop);
         ft1.setObsTimes(tstart, tstop);
      } else {
         const dataSubselector::Gti & gti = merit.gti();
         ft1.setObsTimes(gti.minValue(), gti.maxValue());
      }

      ::addNeededFields(ft1, ft1Dict);
   
      ft1.setNumRows(merit.nrows());

      ft1.header().addHistory("Input merit file: " + rootFile);
      ft1.header().addHistory("Filter string: " + filter);

      EventClassifier eventClass(eventClassifier);
   
      int ncount(0);
      for ( ; merit.itor() != merit.end(); merit.next(), ft1.next()) {
         if (merit.gti().accept(merit["EvtElapsedTime"])) {
            for (::Ft1Map_t::const_iterator variable = ft1Dict.begin();
                 variable != ft1Dict.end(); ++variable) {
               ft1[variable->first].set(merit[variable->second.meritName()]);
            }
//             ft1["event_class"].set(eventClass(merit.row()));
// This change is temporary so that one can select the real source and
// diffuse classes from the FT1 data (this requires a correct
// implementation in evtClassDefs/Pass6_Classifier.py.
// 
            ft1["ctbclasslevel"].set(eventClass(merit.row()) + 1);
            ft1["event_class"].set(merit.conversionType());
            ft1["conversion_type"].set(merit.conversionType());
            ncount++;
         }
      }
      formatter.info() << "number of rows processed: " << ncount << std::endl;
      
      ft1.setNumRows(ncount);
      my_cuts.addGtiCut(merit.gti());
      my_cuts.writeDssKeywords(ft1.header());
   } catch (tip::TipException & eObj) {
// If there are no events in the merit file passing the cut,
// create an empty FT1 file.
      if (st_facilities::Util::expectedException(eObj, "yielded no events")) {
         formatter.info() << "zero rows passed the TCuts." << std::endl;
         ::addNeededFields(ft1, ft1Dict);
         
         ft1.header().addHistory("Input merit file: " + rootFile);
         ft1.header().addHistory("Filter string: " + filter);

         dataSubselector::Gti gti;
         gti.insertInterval(tstart, tstop);
         ft1.setObsTimes(tstart, tstop);

         my_cuts.addGtiCut(gti);
         my_cuts.writeDssKeywords(ft1.header());
      } else {
         throw;
      }
   }
   std::ostringstream creator;
   creator << "makeFT1_kluge " << getVersion();
   ft1.setPhduKeyword("CREATOR", creator.str());
   std::string version = m_pars["file_version"];
   ft1.setPhduKeyword("VERSION", version);
   std::string filename(facilities::Util::basename(fitsFile));
   ft1.setPhduKeyword("FILENAME", filename);
   
   my_cuts.writeGtiExtension(fitsFile);
   st_facilities::FitsUtil::writeChecksums(fitsFile);

   if (st_facilities::Util::fileExists("dummy.root")) {
      std::remove("dummy.root");
   }
}
