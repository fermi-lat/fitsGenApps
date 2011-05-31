/**
 * @file MCResponse.h
 * @brief Class to generate a rsp file from Monte Carlo data (via
 * merit files).
 *
 * @author J. Chiang
 */

#ifndef fitsGenApps_MCResponse_h
#define fitsGenApps_MCResponse_h

#include <string>
#include <vector>

#include "rspgen/IResponse.h"

namespace evtbin {
   class Binner;
}

namespace fitsGenApps {

class MCResponse : public rspgen::IResponse {

public:

   MCResponse(const std::string & spec_file,
              const evtbin::Binner * true_en_binner,
              double index=-1);

   virtual ~MCResponse() throw();

   virtual void compute(double true_energy, std::vector<double> & response);

   void ingestMeritData(const std::vector<std::string> & meritFiles,
                        const std::string & filter,
                        double ngenerated,
                        double tmin=0, double tmax=0,
                        const std::string & efield="EvtEnergyCorr");

   void setResponseData(size_t k, const std::vector<double> & response);

   void setResponseData(double true_energy, 
                        const std::vector<double> & response);

   void setArea(double area);

private:

   long m_nmc;
   double m_emin_mc;
   double m_emax_mc;
   double m_index;
   double m_area;

   std::vector< std::vector<double> > m_responses;

   double energyBinScale(size_t k) const;

};

} // namespace fitsGenApps

#endif // fitsGenApps_MCResponse_h
