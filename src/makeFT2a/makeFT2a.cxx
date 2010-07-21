/**
 * @file makeFT2.cxx
 * @brief Convert ascii D2 data from Gleam to FT2.
 * @author J. Chiang
 *
 * $Header$
 */

#include <cmath>
#include <cstdlib>

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "facilities/Timestamp.h"
#include "facilities/Util.h"

#include "astro/EarthCoordinate.h"

#include "fitsGen/Ft2File.h"

using namespace fitsGen;

namespace {

void getFileNames(int iargc, char * argv[], std::string & rootFile,
                  std::string & fitsFile) {
   if (iargc != 3 && iargc != 4) {
      std::cout << "usage: " 
                << facilities::Util::basename(argv[0]) << " "
                << "<pointing history file> " 
                << "<FITS output file> [<start_date>]" << std::endl;
      std::exit(0);
   } else {
      rootFile = std::string(argv[1]);
      fitsFile = std::string(argv[2]);     
   }
}

unsigned long count_lines(const std::string & infile) {
   std::ifstream file(infile.c_str());
   unsigned long nlines(0);
   std::string line;
   while (std::getline(file, line, '\n')) {
      nlines++;
   }
   file.close();
   return nlines;
}

double startDate(const std::string & start_date) {
   facilities::Timestamp start(start_date);
   double offset((astro::JulianDate(start.getJulian()) 
                  - fitsGen::FtFileBase::missionStart())
                 *astro::JulianDate::secondsPerDay);
   return offset;
}

double geomag_lat(const std::vector<float> & sc_pos, double met) {
   CLHEP::Hep3Vector pos(sc_pos.at(0)/1e3, sc_pos.at(1)/1e3, sc_pos.at(2)/1e3);
   astro::EarthCoordinate coord(pos, met);
   return coord.geolat();
}

bool inside_saa(const std::vector<float> & sc_pos, double met) {
   CLHEP::Hep3Vector pos(sc_pos.at(0)/1e3, sc_pos.at(1)/1e3, sc_pos.at(2)/1e3);
   astro::EarthCoordinate coord(pos, met);
   return coord.insideSAA();
}

} // unnamed namespace

int main(int iargc, char * argv[]) {
   try {
      std::string pointingFile;
      std::string fitsFile;
      
      ::getFileNames(iargc, argv, pointingFile, fitsFile);
      unsigned long nrows = ::count_lines(pointingFile);

      double time_offset(0);
      if (iargc == 4) {
         std::cout << "Using launch date: " 
                   << argv[3] << std::endl;
         time_offset = ::startDate(argv[3]);
      }

      fitsGen::Ft2File ft2(fitsFile, nrows);

      ft2.header().addHistory("Input pointing history file: " + pointingFile);

      std::ifstream d2(pointingFile.c_str());
      std::string line;
      std::vector<std::string> dataFields;
      std::vector<float> scPosition(3);
      while (std::getline(d2, line, '\n')) {
         facilities::Util::stringTokenize(line, "\t ", dataFields);
         ft2["start"].set(std::atof(dataFields[0].c_str()) + time_offset);
// Convert the spacecraft position from km to meters.
         scPosition[0] = std::atof(dataFields[1].c_str())*1e3;
         scPosition[1] = std::atof(dataFields[2].c_str())*1e3;
         scPosition[2] = std::atof(dataFields[3].c_str())*1e3;
         ft2["sc_position"].set(scPosition);
         double ra_scz(std::atof(dataFields[4].c_str()));
         double dec_scz(std::atof(dataFields[5].c_str()));
         double ra_scx(std::atof(dataFields[6].c_str()));
         double dec_scx(std::atof(dataFields[7].c_str()));
         ft2.setScAxes(ra_scz, dec_scz, ra_scx, dec_scx);
         ft2["ra_zenith"].set(std::atof(dataFields[8].c_str()));
         ft2["dec_zenith"].set(std::atof(dataFields[9].c_str()));
         double lonGeo = std::atof(dataFields[10].c_str());
         double latGeo = std::atof(dataFields[11].c_str());
         double radGeo = std::atof(dataFields[12].c_str());
         ft2["lon_geo"].set(lonGeo);
         ft2["lat_geo"].set(latGeo);
         ft2["rad_geo"].set(radGeo);
         ft2.next();
      }

// Get stop times. Table::Iterator does not have random access.
// Also fill the GEOMAG_LAT and IN_SAA columns.
      ft2.itor() = ft2.begin();
      ft2.next();
      double stop_time;
      for ( ; ft2.itor() != ft2.end(); ft2.next()) {
         stop_time = ft2["start"].get();
         ft2.prev();
         ft2["stop"].set(stop_time);
         double met = (ft2["start"].get() + stop_time)/2.;
         ft2["sc_position"].get(scPosition);
         ft2["geomag_lat"].set(::geomag_lat(scPosition, met));
         ft2["in_saa"].set(::inside_saa(scPosition, met));
         ft2.next();
      }
      ft2.itor() = ft2.end();
      ft2.prev();
      ft2.prev();
      double time_step = stop_time - ft2["start"].get();
      ft2.next();
      stop_time += time_step;
      ft2["stop"].set(stop_time);
      double met = (ft2["start"].get() + stop_time)/2.;
      ft2["sc_position"].get(scPosition);
      ft2["geomag_lat"].set(::geomag_lat(scPosition, met));
      ft2["in_saa"].set(::inside_saa(scPosition, met));

// Fill livetime.
      ft2.itor() = ft2.begin();
      for ( ; ft2.itor() != ft2.end(); ft2.next()) {
         double full_interval(ft2["stop"].get() - ft2["start"].get());
         double fraction(0.90);
         ft2["livetime"].set(fraction*full_interval);
         bool in_saa;
         ft2["in_saa"].get(in_saa);
         if (true == in_saa) {
            ft2["livetime"].set(0);
         }
      }

      ft2.itor() = ft2.begin();
      double start(ft2["start"].get());
      ft2.itor() = ft2.end();
      ft2.prev();
      double stop(ft2["stop"].get());
      ft2.setObsTimes(start, stop);

      ft2.setPhduKeyword("CREATOR", "makeFT2a");
   } catch (std::exception & eObj) {
      std::cout << eObj.what() << std::endl;
      std::exit(1);
   }
}
