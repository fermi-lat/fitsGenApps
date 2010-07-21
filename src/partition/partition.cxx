/**
 * @brief Application that partitions an input FT1 file into smaller
 * files of a given number of rows.
 * @author J. Chiang
 * 
 * $Header$
 */

#include <cstdlib>

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>

#include "dataSubselector/Gti.h"

#include "fitsGen/Ft1File.h"
#include "fitsGen/MeritFile.h"

int main(int iargc, char * argv[]) {
   if (iargc != 3 && iargc != 4) {
      std::cout << "usage: " << argv[0] 
                << " <event file> <outfile prefix> [partition size=10000]"
                << std::endl;
      return 0;
   }
   try {
      fitsGen::MeritFile infile(argv[1], "EVENTS");
      std::string prefix(argv[2]);
      long nevts(10000);
      if (iargc == 4) {
         nevts = std::atoi(argv[3]);
      }
      int nfiles(int(infile.nrows()/nevts) + 1);
      dataSubselector::Gti gti(argv[1]);
      double startTime(gti.minValue());
      for (int ifile=0; ifile < nfiles; ifile++) {
         if (ifile == nfiles - 1) {
            nevts = infile.nrows() % nevts;
         }
         std::ostringstream outfilename;
         outfilename << prefix << "_" << std::setw(4) << std::setfill('0') 
                     << ifile << ".fits";
         fitsGen::Ft1File outfile(outfilename.str(), nevts, "EVENTS", argv[1]);
         double stopTime;
         for (int i = 0; i < nevts && outfile.itor() != outfile.end() &&
                 infile.itor() != infile.end();
              i++, outfile.next(), infile.next()) {
            *(outfile.itor()) = *(infile.itor());
            if (i == nevts - 1) {
               outfile["TIME"].get(stopTime);
            }
         }
         if (ifile == nfiles-1) {
            stopTime = gti.maxValue();
         }
         outfile.close();
         dataSubselector::Gti new_gti = 
            gti.applyTimeRangeCut(startTime, stopTime);
         new_gti.writeExtension(outfilename.str());
         startTime = stopTime;
      }
   } catch (std::exception & eObj) {
      std::cout << eObj.what() << std::endl;
      std::exit(1);
   }
}
