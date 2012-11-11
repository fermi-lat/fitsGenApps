/**
 * @file lle2drm.cxx
 * @brief Filter merit or LLE input and output as LLE format.
 * @author J. Chiang
 *
 * $Header$
 */

#include <cctype>
#include <cstdlib>

#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "TH2D.h"
#include "TChain.h"

#include "facilities/commonUtilities.h"
#include "facilities/Util.h"

#include "st_stream/StreamFormatter.h"

#include "st_app/AppParGroup.h"
#include "st_app/StApp.h"
#include "st_app/StAppFactory.h"

#include "tip/Header.h"
#include "tip/IFileSvc.h"
#include "tip/Table.h"

#include "st_facilities/Environment.h"
#include "st_facilities/Util.h"

#include "evtbin/LogBinner.h"

#include "irfLoader/Loader.h"

#include "MCResponse.h"

using facilities::commonUtilities;

namespace {
   void toLower(std::string & name) {
      for (std::string::iterator it = name.begin(); it != name.end(); ++it) {
         *it = std::tolower(*it);
      }
   }
} // anonymous namespace

class LLE2DRM : public st_app::StApp {
public:
   LLE2DRM() : st_app::StApp(),
               m_pars(st_app::StApp::getParGroup("lle2drm")) {
      try {
         setVersion(s_cvs_id);
      } catch (std::exception & eObj) {
         std::cerr << eObj.what() << std::endl;
         std::exit(1);
      } catch (...) {
         std::cerr << "Caught unknown exception in LLE2DRM constructor." 
                   << std::endl;
         std::exit(1);
      }
   }
   virtual ~LLE2DRM() throw() {
      try {
      } catch (std::exception &eObj) {
         std::cerr << eObj.what() << std::endl;
      } catch (...) {
      }
   }
   virtual void run();
   virtual void banner() const;

private:
   void read_tbounds();
   void buildFilterString();

   st_app::AppParGroup & m_pars;

   double m_tmin, m_tmax;

   std::string m_filter;

   static std::string s_cvs_id;
};

std::string LLE2DRM::s_cvs_id("$Name$");

st_app::StAppFactory<LLE2DRM> myAppFactory("lle2drm");

void LLE2DRM::banner() const {
   int verbosity = m_pars["chatter"];
   if (verbosity > 2) {
      st_app::StApp::banner();
   }
}

void LLE2DRM::run() {
   m_pars.Prompt();
   m_pars.Save();

   read_tbounds();

   buildFilterString();

// Load IRFs (needed by base class of MCResponse)
   irfLoader::Loader::go();

// Create the MCResponse object
   std::string spec_file = m_pars["specfile"];
   double emin = m_pars["emin"];
   double emax = m_pars["emax"];
   long nmc = m_pars["enumbins"];
   evtbin::LogBinner true_en_binner(emin, emax, nmc, "true energies");
   double phindex = m_pars["phindex"];
   double area = m_pars["area"];
   fitsGenApps::MCResponse drm(spec_file, &true_en_binner, phindex, area);

// Ingest the merit data
   std::string infile = m_pars["infile"];
   std::vector<std::string> meritFiles;
   st_facilities::Util::readLines(infile, meritFiles);
   drm.ingestMeritData(meritFiles, m_filter, m_tmin, m_tmax);
   
// Write the rsp file
   std::string outfile = m_pars["outfile"];
   std::string dataPath(st_facilities::Environment::dataPath("rspgen"));
   std::string resp_tpl(commonUtilities::joinPath(dataPath,
                                                  "LatResponseTemplate"));
   drm.writeOutput("lle2drm", outfile, resp_tpl);
}

void LLE2DRM::read_tbounds() {
   const tip::Table * spectrum = 
      tip::IFileSvc::instance().readTable(m_pars["specfile"], "SPECTRUM");
   spectrum->getHeader().getKeyword("TSTART", m_tmin);
   spectrum->getHeader().getKeyword("TSTOP", m_tmax);
   delete spectrum;
}

void LLE2DRM::buildFilterString() {
   std::string newFilter = m_pars["TCuts"];
   ::toLower(newFilter);
   if (newFilter == "none" || newFilter == "") {
      // Standard filter string for LLE, allowing for non-default option.
      m_filter = std::string("(ObfGamState==0) && (TkrNumTracks>0) && " 
                             "(GltGemEngine==6 || GltGemEngine==7) && "
                             "(EvtEnergyCorr>0)");
   } else {
      m_filter = newFilter;
   }

// Zenith angle cut
   double zmax = m_pars["zmax"];
   std::ostringstream zenithCut;
   zenithCut << "(FT1ZenithTheta < " << zmax << ")";
   m_filter += " && " + zenithCut.str();

// PSF cut
   double ra = m_pars["ra"];
   double dec = m_pars["dec"];
   double theta = m_pars["theta"];
   std::string radius("(11.5*min(pow(EvtEnergyCorr/59., -0.55), "
                      "pow(EvtEnergyCorr/59.,  -0.87)))");
   if (theta > 40.) {
      radius = std::string("(10.5*min(pow(EvtEnergyCorr/100., -0.65), "
                           "pow(EvtEnergyCorr/100., -0.81)))");
   }
   std::ostringstream psfCut;
   psfCut << "(((cos(FT1Dec*0.0174533)*(FT1Ra - ("
          << ra << ")))^2 + (FT1Dec- (" 
          << dec << "))^2)< ("
          << radius << ")^2)";
   m_filter += " && " + psfCut.str();


// Time cut
   std::ostringstream timeCut;
   timeCut << std::setprecision(14)
           << "((EvtElapsedTime >= " << m_tmin << ") && "
           << "(EvtElapsedTime <= " << m_tmax << "))";
   m_filter += "&& " + timeCut.str();

   st_stream::StreamFormatter formatter("LLE2DRM", "run", 2);
   formatter.info() << "Applying TCut: " << m_filter << "\n" << std::endl;
}
