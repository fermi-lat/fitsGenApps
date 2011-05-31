/**
 * @file lle2drm.cxx
 * @brief Filter merit or LLE input and output as LLE format.
 * @author J. Chiang
 *
 * $Header$
 */

#include <cctype>

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

#include "st_facilities/Util.h"

#include "evtbin/LogBinner.h"

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
   void promptForParameters();
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

void LLE2DRM::promptForParameters() {
   m_pars.Prompt("infile");
   m_pars.Prompt("specfile");
   m_pars.Prompt("outfile");
   m_pars.Prompt("ngen");
   m_pars.Prompt("ra");
   m_pars.Prompt("dec");
   m_pars.Prompt("theta");
   m_pars.Prompt("zmax");
   try {
      m_pars.Prompt("tmin");
      m_pars.Prompt("tmax");
      m_tmin = m_pars["tmin"];
      m_tmax = m_pars["tmax"];
   } catch (const hoops::Hexception &) {
      // Assume INDEF is given as one of the parameter values,
      // so use default of applying no time cut.
      m_tmin = 0;
      m_tmax = 0;
   }
   m_pars.Save();
}

void LLE2DRM::run() {
   promptForParameters();
   buildFilterString();

// Create the MCResponse object
   std::string spec_file = m_pars["specfile"];
   double emin = m_pars["emin"];
   double emax = m_pars["emax"];
   long nmc = m_pars["enumbins"];
   evtbin::LogBinner true_en_binner(emin, emax, nmc, "true energies");
   double phindex = m_pars["phindex"];
   fitsGenApps::MCResponse drm(spec_file, &true_en_binner, phindex);

// Ingest the merit data
   std::string infile = m_pars["infile"];
   std::vector<std::string> meritFiles;
   st_facilities::Util::readLines(infile, meritFiles);
   double ngen = m_pars["ngen"];
   drm.ingestMeritData(meritFiles, m_filter, ngen, m_tmin, m_tmax);
   
// Write the rsp file
   std::string outfile = m_pars["outfile"];
   std::string resp_tpl(commonUtilities::joinPath(commonUtilities::getDataPath("rspgen"), "LatResponseTemplate"));
   drm.writeOutput("lle2drm", outfile, resp_tpl);
}

void LLE2DRM::buildFilterString() {
   std::string newFilter = m_pars["TCuts"];
   ::toLower(newFilter);
   if (newFilter != "none" && newFilter != "") {
      // Standard filter string for LLE, allowing for non-default option.
      m_filter = std::string("(FswGamState==0) && (TkrNumTracks>0) && " 
                          "(GltGemEngine==6 || GltGemEngine==7) && "
                          "(EvtEnergyCorr>0)");
   } else {
      m_filter = newFilter;
   }

// Zenith angle cut
   double zmax = m_pars["zmax"];
   std::ostringstream zenithCut;
   zenithCut << "(FT1ZenithTheta < " << zmax << ")";

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

   m_filter += " && " + zenithCut.str() + " && " + psfCut.str();

// Time cut (if specified)
   if (m_tmin != 0 && m_tmax != 0) {
      std::ostringstream timeCut;
      timeCut << "((EvtElapsedTime >= " << m_tmin << ") && "
              << "(EvtElapsedTime <= " << m_tmax << "))";
      m_filter += "&& " + timeCut.str();
   }

   st_stream::StreamFormatter formatter("LLE2DRM", "run", 2);
   formatter.info() << "applying TCut: " << m_filter << std::endl;
}
