/**
 * @file makeFT2.cxx
 * @brief Convert Root D2 data from Gleam to FT2 format.
 * @author J. Chiang
 *
 * $Header$
 */

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "facilities/Util.h"

#include "st_stream/StreamFormatter.h"

#include "st_app/AppParGroup.h"
#include "st_app/StApp.h"
#include "st_app/StAppFactory.h"

#include "astro/EarthCoordinate.h"

#include "fitsGen/Ft2File.h"
#include "fitsGen/MeritFile.h"

using namespace fitsGen;

class MakeFt2 : public st_app::StApp {
public:
   MakeFt2() : st_app::StApp(),
               m_pars(st_app::StApp::getParGroup("makeFT2")) {
      try {
         setVersion(s_cvs_id);
      } catch (std::exception & eObj) {
         std::cerr << eObj.what() << std::endl;
         std::exit(1);
      } catch (...) {
         std::cerr << "Caught unknown exception in MakeFt2 constructor." 
                   << std::endl;
         std::exit(1);
      }
   }
   virtual ~MakeFt2() throw() {
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
   double geomag_lat(const std::vector<float> & sc_pos, double met) const;
};

std::string MakeFt2::s_cvs_id("$Name$");

st_app::StAppFactory<MakeFt2> myAppFactory("makeFT2");

void MakeFt2::banner() const {
   int verbosity = m_pars["chatter"];
   if (verbosity > 2) {
      st_app::StApp::banner();
   }
}

void MakeFt2::run() {
   m_pars.Prompt();
   m_pars.Save();
   std::string rootFile = m_pars["rootFile"];
   std::string fitsFile = m_pars["fitsFile"];

   fitsGen::MeritFile pointing(rootFile, "pointing_history");
   if (pointing.nrows() == 0) {
      throw std::runtime_error("There are zero rows in the pointing_history "
                               "tree of the input root file.");
   }
   fitsGen::Ft2File ft2(fitsFile, pointing.nrows());

   ft2.header().addHistory("Input merit file: " + rootFile);
   for ( ; pointing.itor() != pointing.end(); pointing.next(), ft2.next()) {
      ft2["start"].set(pointing["start"]);
      ft2["stop"].set(pointing["stop"]);
      std::vector<float> scPosition;
      pointing.row()["sc_position"].get(scPosition);
      ft2["sc_position"].set(scPosition);
      ft2["lat_geo"].set(pointing["lat_geo"]);
      ft2["lon_geo"].set(pointing["lon_geo"]);
      ft2["rad_geo"].set(pointing["rad_geo"]*1e3); // convert to meters
      ft2["ra_zenith"].set(pointing["ra_zenith"]);
      ft2["dec_zenith"].set(pointing["dec_zenith"]);
      ft2["b_mcilwain"].set(pointing["B_McIlwain"]);
      ft2["l_mcilwain"].set(pointing["L_McIlwain"]);
      double met((pointing["start"] + pointing["stop"])/2.);
      ft2["geomag_lat"].set(geomag_lat(scPosition, met));
      ft2["in_saa"].set(static_cast<bool>(pointing["in_saa"]));
      ft2.setScAxes(pointing["ra_scz"], pointing["dec_scz"], 
                    pointing["ra_scx"], pointing["dec_scx"]);
      ft2["livetime"].set(pointing["livetime"]);
   }
   ft2.itor() = ft2.begin();
   double start_time(ft2["start"].get());
   ft2.itor() = ft2.end();
   ft2.prev();
   double stop_time(ft2["stop"].get());
   ft2.setObsTimes(start_time, stop_time);

   std::ostringstream creator;
   creator << "makeFT2 " << getVersion();
   ft2.setPhduKeyword("CREATOR", creator.str());
   std::string version = m_pars["file_version"];
   ft2.setPhduKeyword("VERSION", version);
   std::string filename(facilities::Util::basename(fitsFile));
   ft2.setPhduKeyword("FILENAME", filename);
}

double MakeFt2::geomag_lat(const std::vector<float> & sc_pos,
                           double met) const {
   CLHEP::Hep3Vector pos(sc_pos.at(0)/1e3, sc_pos.at(1)/1e3, sc_pos.at(2)/1e3);
   astro::EarthCoordinate coord(pos, met);
   return coord.geolat();
}
