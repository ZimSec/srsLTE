/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2017 Software Radio Systems Limited
 *
 * \section LICENSE
 *
 * This file is part of srsLTE.
 *
 * srsUE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsUE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

#include <iostream>
#include <fstream>
#include <string>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>

#include "enb.h"
#include "metrics_stdout.h"

using namespace std;
using namespace srsenb;
namespace bpo = boost::program_options;

/**********************************************************************
 *  Program arguments processing
 ***********************************************************************/
string config_file;

void parse_args(all_args_t *args, int argc, char* argv[]) {

  string enb_id;
  string cell_id;
  string tac;
  string mcc;
  string mnc;

  // Command line only options
  bpo::options_description general("General options");
  general.add_options()
      ("help,h", "Produce help message")
      ("version,v", "Print version information and exit")
      ;

  // Command line or config file options
  bpo::options_description common("Configuration options");
  common.add_options()

    ("enb.enb_id",        bpo::value<string>(&enb_id)->default_value("0x0"),                       "eNodeB ID")
    ("enb.name",          bpo::value<string>(&args->enb.s1ap.enb_name)->default_value("srsenb01"), "eNodeB Name")
    ("enb.cell_id",       bpo::value<string>(&cell_id)->default_value("0x0"),                      "Cell ID")
    ("enb.tac",           bpo::value<string>(&tac)->default_value("0x0"),                          "Tracking Area Code")
    ("enb.mcc",           bpo::value<string>(&mcc)->default_value("001"),                          "Mobile Country Code")
    ("enb.mnc",           bpo::value<string>(&mnc)->default_value("01"),                           "Mobile Network Code")
    ("enb.mme_addr",      bpo::value<string>(&args->enb.s1ap.mme_addr)->default_value("127.0.0.1"),"IP address of MME for S1 connnection")
    ("enb.gtp_bind_addr", bpo::value<string>(&args->enb.s1ap.gtp_bind_addr)->default_value("192.168.3.1"), "Local IP address to bind for GTP connection")
    ("enb.phy_cell_id",   bpo::value<uint32_t>(&args->enb.pci)->default_value(0),               "Physical Cell Identity (PCI)")
    ("enb.n_prb",         bpo::value<uint32_t>(&args->enb.n_prb)->default_value(25),               "Number of PRB")
    
    ("enb_files.sib_config", bpo::value<string>(&args->enb_files.sib_config)->default_value("sib.conf"),      "SIB configuration files")
    ("enb_files.rr_config",  bpo::value<string>(&args->enb_files.rr_config)->default_value("rr.conf"),      "RR configuration files")
    ("enb_files.drb_config", bpo::value<string>(&args->enb_files.drb_config)->default_value("drb.conf"),      "DRB configuration files")

    ("rf.dl_earfcn",      bpo::value<uint32_t>(&args->rf.dl_earfcn)->default_value(3400),  "Downlink EARFCN")
    ("rf.ul_earfcn",      bpo::value<uint32_t>(&args->rf.ul_earfcn)->default_value(0),     "Uplink EARFCN (Default based on Downlink EARFCN)")
    ("rf.rx_gain",        bpo::value<float>(&args->rf.rx_gain)->default_value(50),           "Front-end receiver gain")
    ("rf.tx_gain",        bpo::value<float>(&args->rf.tx_gain)->default_value(70),           "Front-end transmitter gain")
    ("rf.dl_freq",        bpo::value<float>(&args->rf.dl_freq)->default_value(-1),      "Downlink Frequency (if positive overrides EARFCN)")
    ("rf.ul_freq",        bpo::value<float>(&args->rf.ul_freq)->default_value(-1),      "Uplink Frequency (if positive overrides EARFCN)")

    ("rf.device_name",       bpo::value<string>(&args->rf.device_name)->default_value("auto"),   "Front-end device name")
    ("rf.device_args",       bpo::value<string>(&args->rf.device_args)->default_value("auto"),   "Front-end device arguments")
    ("rf.time_adv_nsamples", bpo::value<string>(&args->rf.time_adv_nsamples)->default_value("auto"),    "Transmission time advance")
    ("rf.burst_preamble_us", bpo::value<string>(&args->rf.burst_preamble)->default_value("auto"), "Transmission time advance")

    ("pcap.enable",       bpo::value<bool>(&args->pcap.enable)->default_value(false),           "Enable MAC packet captures for wireshark")
    ("pcap.filename",     bpo::value<string>(&args->pcap.filename)->default_value("ue.pcap"),   "MAC layer capture filename")

    ("gui.enable",        bpo::value<bool>(&args->gui.enable)->default_value(false),            "Enable GUI plots")

    ("log.phy_level",     bpo::value<string>(&args->log.phy_level),   "PHY log level")
    ("log.phy_hex_limit", bpo::value<int>(&args->log.phy_hex_limit),  "PHY log hex dump limit")
    ("log.mac_level",     bpo::value<string>(&args->log.mac_level),   "MAC log level")
    ("log.mac_hex_limit", bpo::value<int>(&args->log.mac_hex_limit),  "MAC log hex dump limit")
    ("log.rlc_level",     bpo::value<string>(&args->log.rlc_level),   "RLC log level")
    ("log.rlc_hex_limit", bpo::value<int>(&args->log.rlc_hex_limit),  "RLC log hex dump limit")
    ("log.pdcp_level",    bpo::value<string>(&args->log.pdcp_level),  "PDCP log level")
    ("log.pdcp_hex_limit",bpo::value<int>(&args->log.pdcp_hex_limit), "PDCP log hex dump limit")
    ("log.rrc_level",     bpo::value<string>(&args->log.rrc_level),   "RRC log level")
    ("log.rrc_hex_limit", bpo::value<int>(&args->log.rrc_hex_limit),  "RRC log hex dump limit")
    ("log.gtpu_level",    bpo::value<string>(&args->log.gtpu_level),  "GTPU log level")
    ("log.gtpu_hex_limit",bpo::value<int>(&args->log.gtpu_hex_limit), "GTPU log hex dump limit")
    ("log.s1ap_level",    bpo::value<string>(&args->log.s1ap_level),  "S1AP log level")
    ("log.s1ap_hex_limit",bpo::value<int>(&args->log.s1ap_hex_limit), "S1AP log hex dump limit")

    ("log.all_level",     bpo::value<string>(&args->log.all_level)->default_value("info"),   "ALL log level")
    ("log.all_hex_limit", bpo::value<int>(&args->log.all_hex_limit)->default_value(32),  "ALL log hex dump limit")

    ("log.filename",      bpo::value<string>(&args->log.filename)->default_value("/tmp/ue.log"),"Log filename")

    /* MCS section */
    ("scheduler.pdsch_mcs",
        bpo::value<int>(&args->expert.mac.sched.pdsch_mcs)->default_value(-1),
        "Optional fixed PDSCH MCS (ignores reported CQIs if specified)")
    ("scheduler.pdsch_max_mcs",
        bpo::value<int>(&args->expert.mac.sched.pdsch_max_mcs)->default_value(-1),
        "Optional PDSCH MCS limit")
    ("scheduler.pusch_mcs",
        bpo::value<int>(&args->expert.mac.sched.pusch_mcs)->default_value(-1),
        "Optional fixed PUSCH MCS (ignores reported CQIs if specified)")
    ("scheduler.pusch_max_mcs",
        bpo::value<int>(&args->expert.mac.sched.pusch_max_mcs)->default_value(-1),
        "Optional PUSCH MCS limit")
    ("scheduler.nof_ctrl_symbols",
        bpo::value<int>(&args->expert.mac.sched.nof_ctrl_symbols)->default_value(3),
        "Number of control symbols")

    
    /* Expert section */

    ("expert.metrics_period_secs",
        bpo::value<float>(&args->expert.metrics_period_secs)->default_value(1.0),
        "Periodicity for metrics in seconds")

    ("expert.pregenerate_signals",
        bpo::value<bool>(&args->expert.phy.pregenerate_signals)->default_value(false),
        "Pregenerate uplink signals after attach. Improves CPU performance.")

    ("expert.pusch_max_its",
        bpo::value<int>(&args->expert.phy.pusch_max_its)->default_value(4),
        "Maximum number of turbo decoder iterations")

    ("expert.tx_amplitude",
        bpo::value<float>(&args->expert.phy.tx_amplitude)->default_value(0.6),
        "Transmit amplitude factor")

    ("expert.nof_phy_threads",
        bpo::value<int>(&args->expert.phy.nof_phy_threads)->default_value(2),
        "Number of PHY threads")

    ("expert.link_failure_nof_err",
        bpo::value<int>(&args->expert.mac.link_failure_nof_err)->default_value(50),
        "Number of PUSCH failures after which a radio-link failure is triggered")

    ("expert.max_prach_offset_us",
        bpo::value<float>(&args->expert.phy.max_prach_offset_us)->default_value(30),
        "Maximum allowed RACH offset (in us)")

    ("expert.equalizer_mode",
        bpo::value<string>(&args->expert.phy.equalizer_mode)->default_value("mmse"),
        "Equalizer mode")

    ("expert.estimator_fil_w",
        bpo::value<float>(&args->expert.phy.estimator_fil_w)->default_value(0.1),
        "Chooses the coefficients for the 3-tap channel estimator centered filter.")

    ("expert.rrc_inactivity_timer",
        bpo::value<uint32_t>(&args->expert.rrc_inactivity_timer)->default_value(10000),
        "Inactivity timer in ms")


    ("rf_calibration.tx_corr_dc_gain",  bpo::value<float>(&args->rf_cal.tx_corr_dc_gain)->default_value(0.0),  "TX DC offset gain correction")
    ("rf_calibration.tx_corr_dc_phase", bpo::value<float>(&args->rf_cal.tx_corr_dc_phase)->default_value(0.0), "TX DC offset phase correction")
    ("rf_calibration.tx_corr_iq_i",     bpo::value<float>(&args->rf_cal.tx_corr_iq_i)->default_value(0.0),     "TX IQ imbalance inphase correction")
    ("rf_calibration.tx_corr_iq_q",     bpo::value<float>(&args->rf_cal.tx_corr_iq_q)->default_value(0.0),     "TX IQ imbalance quadrature correction")

  ;

  // Positional options - config file location
  bpo::options_description position("Positional options");
  position.add_options()
  ("config_file", bpo::value< string >(&config_file), "eNodeB configuration file")
  ;
  bpo::positional_options_description p;
  p.add("config_file", -1);

  // these options are allowed on the command line
  bpo::options_description cmdline_options;
  cmdline_options.add(common).add(position).add(general);

  // parse the command line and store result in vm
  bpo::variables_map vm;
  bpo::store(bpo::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
  bpo::notify(vm);

  // help option was given - print usage and exit
  if (vm.count("help")) {
      cout << "Usage: " << argv[0] << " [OPTIONS] config_file" << endl << endl;
      cout << common << endl << general << endl;
      exit(0);
  }

  // print version number and exit
    // print version number and exit
    if (vm.count("version")) {
        cout << "Version " <<
                srslte_get_version_major() << "." <<
                srslte_get_version_minor() << "." <<
                srslte_get_version_patch() << endl;
        exit(0);
    }

  // no config file given - print usage and exit
  if (!vm.count("config_file")) {
      cout << "Error: Configuration file not provided" << endl;
      cout << "Usage: " << argv[0] << " [OPTIONS] config_file" << endl << endl;
      exit(0);
  } else {
      cout << "Reading configuration file " << config_file << "..." << endl;
      ifstream conf(config_file.c_str(), ios::in);
      if(conf.fail()) {
        cout << "Failed to read configuration file " << config_file << " - exiting" << endl;
        exit(1);
      }
      bpo::store(bpo::parse_config_file(conf, common), vm);
      bpo::notify(vm);
  }

  // Convert hex strings
  {
    std::stringstream sstr;
    sstr << std::hex << vm["enb.enb_id"].as<std::string>();
    sstr >> args->enb.s1ap.enb_id;
  }
  {
    std::stringstream sstr;
    sstr << std::hex << vm["enb.cell_id"].as<std::string>();
    uint16_t tmp; // Need intermediate uint16_t as uint8_t is treated as char
    sstr >> tmp;
    args->enb.s1ap.cell_id = tmp;
  }
  {
    std::stringstream sstr;
    sstr << std::hex << vm["enb.tac"].as<std::string>();
    sstr >> args->enb.s1ap.tac;
  }

  // Convert MCC/MNC strings
  if(!srslte::string_to_mcc(mcc, &args->enb.s1ap.mcc)) {
    cout << "Error parsing enb.mcc:" << mcc << " - must be a 3-digit string." << endl;
  }
  if(!srslte::string_to_mnc(mnc, &args->enb.s1ap.mnc)) {
    cout << "Error parsing enb.mnc:" << mnc << " - must be a 2 or 3-digit string." << endl;
  }


  // Apply all_level to any unset layers
  if (vm.count("log.all_level")) {
    if(!vm.count("log.phy_level")) {
      args->log.phy_level = args->log.all_level;
    }
    if(!vm.count("log.mac_level")) {
      args->log.mac_level = args->log.all_level;
    }
    if(!vm.count("log.rlc_level")) {
      args->log.rlc_level = args->log.all_level;
    }
    if(!vm.count("log.pdcp_level")) {
      args->log.pdcp_level = args->log.all_level;
    }
    if(!vm.count("log.rrc_level")) {
      args->log.rrc_level = args->log.all_level;
    }
    if(!vm.count("log.gtpu_level")) {
      args->log.gtpu_level = args->log.all_level;
    }
    if(!vm.count("log.s1ap_level")) {
      args->log.s1ap_level = args->log.all_level;
    }
  }

  // Apply all_hex_limit to any unset layers
  if (vm.count("log.all_hex_limit")) {
    if(!vm.count("log.phy_hex_limit")) {
      args->log.phy_hex_limit = args->log.all_hex_limit;
    }
    if(!vm.count("log.mac_hex_limit")) {
      args->log.mac_hex_limit = args->log.all_hex_limit;
    }
    if(!vm.count("log.rlc_hex_limit")) {
      args->log.rlc_hex_limit = args->log.all_hex_limit;
    }
    if(!vm.count("log.pdcp_hex_limit")) {
      args->log.pdcp_hex_limit = args->log.all_hex_limit;
    }
    if(!vm.count("log.rrc_hex_limit")) {
      args->log.rrc_hex_limit = args->log.all_hex_limit;
    }
    if(!vm.count("log.gtpu_hex_limit")) {
      args->log.gtpu_hex_limit = args->log.all_hex_limit;
    }
    if(!vm.count("log.s1ap_hex_limit")) {
      args->log.s1ap_hex_limit = args->log.all_hex_limit;
    }
  }
}

static bool running    = true;
static bool do_metrics = false;

void sig_int_handler(int signo)
{
  running = false;
}

void *input_loop(void *m)
{
  metrics_stdout *metrics = (metrics_stdout*) m;
  char key;
  while(running) {
    cin >> key;
    if('t' == key) {
      do_metrics = !do_metrics;
      if(do_metrics) {
        cout << "Enter t to stop trace." << endl;
      } else {
        cout << "Enter t to restart trace." << endl;
      }
      metrics->toggle_print(do_metrics);
    }
  }
  return NULL;
}

int main(int argc, char *argv[])
{
  signal(SIGINT,    sig_int_handler);
  all_args_t        args;
  metrics_stdout    metrics;
  enb              *enb = enb::get_instance();

  cout << "---  Software Radio Systems LTE eNodeB  ---" << endl << endl;

  parse_args(&args, argc, argv);
  if(!enb->init(&args)) {
    exit(1);
  }
  metrics.init(enb, args.expert.metrics_period_secs);

  pthread_t input;
  pthread_create(&input, NULL, &input_loop, &metrics);

  bool plot_started         = false; 
  bool signals_pregenerated = false; 
  while(running) {
    if (!plot_started && args.gui.enable) {
      enb->start_plot();
      plot_started = true; 
    }
    sleep(1);
  }
  pthread_cancel(input);
  metrics.stop();
  enb->stop();
  enb->cleanup();
  cout << "---  exiting  ---" << endl;
  exit(0);
}
