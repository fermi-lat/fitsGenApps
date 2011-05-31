/**
 * @file MCResponse.cxx
 * @brief  Class to generate a rsp file from Monte Carlo data (via
 * merit files).
 *
 * @author J. Chiang
 */

#include <cmath>

#include <iostream>

#include "TCanvas.h"
#include "TChain.h"
#include "TEventList.h"
#include "TFile.h"
#include "TH2D.h"

#include "evtbin/Binner.h"

#include "MCResponse.h"

namespace fitsGenApps {

MCResponse::MCResponse(const std::string & spec_file,
                       const evtbin::Binner * true_en_binner,
                       double index,
                       double area) :
   rspgen::IResponse("P7SOURCE_V6::FRONT", spec_file, true_en_binner),
   m_nmc(m_true_en_binner->getNumBins()),
   m_emin_mc(m_true_en_binner->getInterval(0).begin()),
   m_emax_mc(m_true_en_binner->getInterval(m_nmc-1).end()),
   m_index(index),
   m_area(area) {
}

MCResponse::~MCResponse() throw() {}

void MCResponse::compute(double true_energy, std::vector<double> & response) {
   long k(m_true_en_binner->computeIndex(true_energy));
   response = m_responses.at(k);
}

void MCResponse::ingestMeritData(const std::vector<std::string> & meritFiles,
                                 const std::string & filter,
                                 double tmin, double tmax,
                                 const std::string & efield) {
   st_stream::StreamFormatter formatter("MCResponse", "ingestMeritData", 2);

   TChain * mc_data = new TChain("MeritTuple");
   TChain * job_info = new TChain("jobinfo");

   for (size_t i(0); i < meritFiles.size(); i++) {
      mc_data->Add(meritFiles[i].c_str());
      job_info->Add(meritFiles[i].c_str());
   }

   mc_data->SetBranchStatus("*", 0);
   mc_data->SetBranchStatus("Evt*", 1);
   mc_data->SetBranchStatus("FT1*", 1);
   mc_data->SetBranchStatus("McEnergy", 1);
   mc_data->SetBranchStatus("McLogEnergy", 1);
   mc_data->SetBranchStatus("McZDir", 1);
   mc_data->SetBranchStatus("ObfGamState", 1);
   mc_data->SetBranchStatus("FswGamState", 1);
   mc_data->SetBranchStatus("TkrNumTracks", 1);
   mc_data->SetBranchStatus("Tkr1SSDVeto", 1);
   mc_data->SetBranchStatus("CTB*", 1);
   mc_data->SetBranchStatus("Pt*", 1);
   mc_data->SetBranchStatus("GltEngine", 1);
   mc_data->SetBranchStatus("GltGemEngine", 1);
   mc_data->SetBranchStatus("CalEnergyRaw", 1);
   mc_data->SetBranchStatus("VtxAngle", 1);
   mc_data->SetBranchStatus("Tkr1FirstLayer", 1);

   double tmin_mc = mc_data->GetMinimum("EvtElapsedTime");
   double tmax_mc = mc_data->GetMaximum("EvtElapsedTime");
   if (tmin == 0 && tmax == 0) {
      tmin = tmin_mc;
      tmax = tmax_mc;
   }

   long ngenerated(0);
   Long64_t generated;
   job_info->SetBranchAddress("generated", &generated);
   for (size_t i(0); i < meritFiles.size(); i++) {
      job_info->GetEntry(i);
      ngenerated += generated;
      formatter.info(3) << meritFiles[i] << ": " << generated << std::endl;
   }
   formatter.info() << "Total number of MC-generated events: " 
                    << ngenerated << std::endl;

   int nmeas = m_app_en_binner->getNumBins();
   double emin = m_app_en_binner->getInterval(0).begin();
   double emax = m_app_en_binner->getInterval(nmeas-1).end();

   TH2D DRM("DRM", "DRM", m_nmc, std::log10(m_emin_mc), std::log10(m_emax_mc),
            nmeas, std::log10(emin), std::log10(emax));

   TCanvas canvas("DRM_CANVAS", "DRM_CANVAS");
   canvas.cd(1);
   mc_data->Draw(">>merit_event_list", filter.c_str());
   TEventList * event_list = (TEventList *)gDirectory->Get("merit_event_list");
   formatter.info() << "Number of events in the input merit files: " 
                    << mc_data->GetEntries() << std::endl;
   formatter.info() << "Number of events passing cuts: " 
                    << event_list->GetN() << std::endl;
                    

   std::ostringstream DRM_directive;
   DRM_directive << "log10(" << efield << "):McLogEnergy>>DRM";
   mc_data->Draw(DRM_directive.str().c_str(), filter.c_str(), "colz");

   for (size_t k(0); k < m_nmc; k++) {
      double norm = m_area/static_cast<double>(ngenerated)/energyBinScale(k);
      formatter.info(4) << k << "  "
                        << norm << std::endl;
      std::vector<double> row;
      for (size_t kmeas(0); kmeas < nmeas; kmeas++) {
         formatter.info(4) << kmeas << "  " 
                           << DRM.GetBinContent(k+1, kmeas+1) << "  ";
         row.push_back(norm*DRM.GetBinContent(k+1, kmeas+1));
         formatter.info(4) << row.back() << std::endl;
      }
      m_responses.push_back(row);
   }

   delete mc_data;
}

double MCResponse::energyBinScale(size_t k) const {
   const evtbin::Binner::Interval interval(m_true_en_binner->getInterval(k));
   if (m_index == -1) {
      return (std::log(interval.end()/interval.begin())
              /std::log(m_emax_mc/m_emin_mc));
   }
   return ( (std::pow(interval.end(), 1 + m_index) 
             - std::pow(interval.begin(), 1 + m_index))
            /(std::pow(m_emax_mc, 1 + m_index) 
              - std::pow(m_emin_mc, 1 + m_index)) );
}

void MCResponse::setArea(double area) {
   m_area = area;
}

} //namespace fitsGenApps
