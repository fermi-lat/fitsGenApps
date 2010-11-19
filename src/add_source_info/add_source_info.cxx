/**
 * @file add_source_info.cxx
 * @brief Add point source information (theta, phi, exposure) to an FT2 file.
 * @author J. Chiang
 *
 * $Header$
 */

#include <cmath>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <fstream>
#include <iostream>

#include "st_stream/StreamFormatter.h"

#include "st_app/AppParGroup.h"
#include "st_app/StApp.h"
#include "st_app/StAppFactory.h"

#include "tip/IFileSvc.h"
#include "tip/Table.h"

#include "astro/SkyDir.h"

// #include "irfInterface/IrfsFactory.h"
// #include "irfInterface/Irfs.h"
// #include "irfLoader/Loader.h"

/**
 * @class SourceInfo
 *
 */

class SourceInfo : public st_app::StApp {

public:

   SourceInfo();

   virtual ~SourceInfo() throw() {
      try {
         delete m_formatter;
      } catch (std::exception & eObj) {
         std::cerr << eObj.what() << std::endl;
      } catch (...) {
         std::cerr << "caught unknown exception in "
                   << "diffuseResponse destructor." 
                   << std::endl;
      }
   }

   virtual void run();
   virtual void banner() const;

private:
   st_stream::StreamFormatter * m_formatter;
   st_app::AppParGroup & m_pars;

   astro::SkyDir m_srcDir;

   static std::string s_cvs_id;

   void promptForParameters();
   void appendField(tip::Table * sctable, const std::string & fieldName) const;
   void addColumns();
};

st_app::StAppFactory<SourceInfo> myAppFactory("add_source_info");

std::string SourceInfo::s_cvs_id("$Name$");

SourceInfo::SourceInfo() 
   : st_app::StApp(), 
     m_formatter(new st_stream::StreamFormatter("add_source_info", "", 2)),
     m_pars(st_app::StApp::getParGroup("add_source_info")) {
   setVersion(s_cvs_id);
}

void SourceInfo::banner() const {
   int verbosity = m_pars["chatter"];
   if (verbosity > 2) {
      st_app::StApp::banner();
   }
}

void SourceInfo::run() {
   promptForParameters();
   double ra = m_pars["ra"];
   double dec = m_pars["dec"];
   m_srcDir = astro::SkyDir(ra, dec);
   addColumns();
}

void SourceInfo::promptForParameters() {
   m_pars.Prompt();
   m_pars.Save();
}

void SourceInfo::appendField(tip::Table * sctable, 
                             const std::string & fieldName) const {
   try {
      sctable->appendField(fieldName, "D");
   } catch (tip::TipException & eObj) {
      bool clobber = m_pars["clobber"];
      if (!clobber) {
         throw;
      }
   }
}

void SourceInfo::addColumns() {
//    irfLoader::Loader::go();
//    irfInterface::IrfsFactory & 
//       irfsFactory(*irfInterface::IrfsFactory::instance());

//    irfInterface::Irfs * irfs(irfsFactory.create(m_pars["irfs"]));

   tip::Table * sctable
      = tip::IFileSvc::instance().editTable(m_pars["scfile"], 
                                            m_pars["sctable"]);

   tip::Table::Iterator it = sctable->begin();
   tip::TableRecord & row = *it;

   std::string srcName = m_pars["srcname"];
   std::string thetaField(srcName + "_THETA");
   std::string phiField(srcName + "_PHI");
//   std::string aeffField(srcName + "_aeff");
   appendField(sctable, thetaField);
   appendField(sctable, phiField);
//   appendField(sctable, aeffField);

   double ra_scz, dec_scz, ra_scx, dec_scx;
   for ( ; it != sctable->end(); ++it) {
      row["ra_scz"].get(ra_scz);
      row["dec_scz"].get(dec_scz);
      row["ra_scx"].get(ra_scx);
      row["dec_scx"].get(dec_scx);
      astro::SkyDir zhat(ra_scz, dec_scz);
      astro::SkyDir xhat(ra_scx, dec_scx);
      astro::SkyDir yhat(-xhat.dir().cross(zhat.dir()));
      double theta(std::acos(m_srcDir.dir().dot(zhat.dir()))*180./M_PI);
      double phi(std::atan2(yhat.dir().dot(m_srcDir.dir()),
                            xhat.dir().dot(m_srcDir.dir()))*180./M_PI);
      if (phi < 0) {
         phi += 360.;
      }
      row[thetaField].set(theta);
      row[phiField].set(phi);
//      row[aeffField].set(irfs->aeff()->value(m_pars["energy"], theta, phi));
   }
   delete sctable;
}
