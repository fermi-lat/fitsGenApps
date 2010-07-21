/**
 * @file irfTuple.cxx
 * @brief Convert the relevant parts (CTB and Mc variables) of merit
 * ntuple to a FITS file
 * @author J. Chiang
 *
 * $Header$
 */

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "dataSubselector/Gti.h"
#include "dataSubselector/Cuts.h"

#include "astro/SkyDir.h"

#include "st_facilities/Env.h"
#include "st_facilities/FitsUtil.h"
#include "st_facilities/Util.h"

#include "facilities/commonUtilities.h"

#include "fitsGen/Ft1File.h"
#include "fitsGen/MeritFile.h"

using namespace fitsGen;

int main(int iargc, char * argv[]) {
   if (iargc < 3) {
      std::cout << "usage: " << argv[0] 
                << " <merit file> <output FITS file>"
                << " [<filter_string> [<irfTupleNameFile>]]";
   }
   std::string rootFile(argv[1]);
   std::string fitsFile(argv[2]);

   std::ostringstream filter;
   if (iargc >= 4) {
      if (st_facilities::Util::fileExists(argv[3])) {
         std::vector<std::string> lines;
         st_facilities::Util::readLines(argv[3], lines, "#", true);
         for (size_t i = 0; i < lines.size(); i++) {
            filter << lines.at(i);
         }
      } else {
         filter << argv[3];
      }
      std::cout << "applying TCut: " << filter.str() << std::endl;
   }
   std::string irfTupleNameFile = 
      facilities::commonUtilities::joinPath(
         facilities::commonUtilities::getDataPath("fitsGen"), "irfTupleNames");
   if (iargc == 5) {
      irfTupleNameFile = argv[4];
   }

   std::vector<std::string> variableNames;
   st_facilities::Util::readLines(irfTupleNameFile, variableNames, "#", true);

   try {
      fitsGen::MeritFile merit(rootFile, "MeritTuple", filter.str());
      if (st_facilities::Util::fileExists(fitsFile)) {
         std::remove(fitsFile.c_str());
      }
      fitsGen::Ft1File ft1(fitsFile, 0, "EVENTS", "");
   
      ft1.header().addHistory("Input merit file: " + rootFile);
      ft1.header().addHistory("Filter string: " + filter.str());

      ft1.appendField("TIME", "1D");
      std::vector<std::string>::const_iterator variable(variableNames.begin());
      for ( ; variable != variableNames.end(); ++variable) {
         ft1.appendField(*variable, "1E");
      }
      ft1.setNumRows(merit.nrows());
      int ncount(0);
      for ( ; merit.itor() != merit.end(); merit.next(), ft1.next()) {
         ft1["TIME"].set(merit["EvtElapsedTime"]);
         for (variable = variableNames.begin();
              variable != variableNames.end(); ++variable) {
            ft1[*variable].set(merit[*variable]);
         }
         ncount++;
      }
      std::cout << "number of rows processed: " << ncount << std::endl;

      ft1.setNumRows(ncount);
   } catch (std::exception & eObj) {
      std::cout << eObj.what() << std::endl;
      return 1;
   }
   st_facilities::FitsUtil::writeChecksums(fitsFile);
   if (st_facilities::Util::fileExists("dummy.root")) {
      std::remove("dummy.root");
   }
}

