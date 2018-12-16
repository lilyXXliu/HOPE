#include <sys/time.h>
#include <time.h>

#include <algorithm>
#include <fstream>
#include <iostream>

#include "encoder_factory.hpp"

namespace microbench {

static const std::string file_email = "datasets/emails.txt";
static const std::string file_wiki = "datasets/wikis.txt";
static const std::string file_url = "datasets/urls.txt";

static const int kLongestCodeLen = 4096;

//-------------------------------------------------------------
// Workload IDs
//-------------------------------------------------------------
static const int kEmail = 0;
static const int kWiki = 1;
static const int kUrl = 2;

static const std::string output_dir = "results/microbench/";

//-------------------------------------------------------------
// Sample Size Sweep
//-------------------------------------------------------------
static const std::string sample_size_sweep_subdir = "sample_size_sweep/";
static const std::string file_cpr_email_sample_size_sweep
= output_dir + sample_size_sweep_subdir + "cpr_email_sample_size_sweep.csv";
std::ofstream output_cpr_email_sample_size_sweep;
static const std::string file_bt_email_sample_size_sweep
= output_dir + sample_size_sweep_subdir + "bt_email_sample_size_sweep.csv";
std::ofstream output_bt_email_sample_size_sweep;
static const std::string file_cpr_wiki_sample_size_sweep
= output_dir + sample_size_sweep_subdir + "cpr_wiki_sample_size_sweep.csv";
std::ofstream output_cpr_wiki_sample_size_sweep;
static const std::string file_bt_wiki_sample_size_sweep
= output_dir + sample_size_sweep_subdir + "bt_wiki_sample_size_sweep.csv";
std::ofstream output_bt_wiki_sample_size_sweep;
static const std::string file_cpr_url_sample_size_sweep
= output_dir + sample_size_sweep_subdir + "cpr_url_sample_size_sweep.csv";
std::ofstream output_cpr_url_sample_size_sweep;
static const std::string file_bt_url_sample_size_sweep
= output_dir + sample_size_sweep_subdir + "bt_url_sample_size_sweep.csv";
std::ofstream output_bt_url_sample_size_sweep;

//-------------------------------------------------------------
// CPR and Latency
//-------------------------------------------------------------
static const std::string cpr_latency_subdir = "cpr_latency/";
static const std::string file_cpr_email_dict_size
= output_dir + cpr_latency_subdir + "cpr_email_dict_size.csv";
std::ofstream output_cpr_email_dict_size;
static const std::string file_lat_email_dict_size
= output_dir + cpr_latency_subdir + "lat_email_dict_size.csv";
std::ofstream output_lat_email_dict_size;
static const std::string file_cpr_wiki_dict_size
= output_dir + cpr_latency_subdir + "cpr_wiki_dict_size.csv";
std::ofstream output_cpr_wiki_dict_size;
static const std::string file_lat_wiki_dict_size
= output_dir + cpr_latency_subdir + "lat_wiki_dict_size.csv";
std::ofstream output_lat_wiki_dict_size;
static const std::string file_cpr_url_dict_size
= output_dir + cpr_latency_subdir + "cpr_url_dict_size.csv";
std::ofstream output_cpr_url_dict_size;
static const std::string file_lat_url_dict_size
= output_dir + cpr_latency_subdir + "lat_url_dict_size.csv";
std::ofstream output_lat_url_dict_size;

//-------------------------------------------------------------
// Build Time
//-------------------------------------------------------------

double getNow() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + tv.tv_usec / 1000000.0;
}

int64_t loadKeys(const std::string& file_name,
		 std::vector<std::string> &keys,
		 std::vector<std::string> &keys_shuffle) {
    std::ifstream infile(file_name);
    std::string key;
    int64_t total_len = 0;
    while (infile.good()) {
	infile >> key;
	keys.push_back(key);
	keys_shuffle.push_back(key);
	total_len += key.length();
    }
    std::random_shuffle(keys_shuffle.begin(), keys_shuffle.end());
    return total_len;
}

void exec(const int expt_id, const int wkld_id,
	  const int encoder_type, const int64_t dict_size_limit,
	  const int sample_percent,
	  const std::vector<std::string>& keys_shuffle,
	  const int64_t total_len) {
    std::vector<std::string> sample_keys;
    int step_size = 100 / sample_percent;
    for (int i = 0; i < (int)keys_shuffle.size(); i += step_size) {
	sample_keys.push_back(keys_shuffle[i]);
    }
    
    ope::Encoder* encoder = ope::EncoderFactory::createEncoder(encoder_type);
    double time_start = getNow();
    encoder->build(sample_keys, dict_size_limit);
    double time_end = getNow();
    double bt = time_end - time_start;

    uint8_t* buffer = new uint8_t[kLongestCodeLen];
    uint64_t total_enc_len = 0;
    time_start = getNow();
    for (int i = 0; i < (int)keys_shuffle.size(); i++) {
	total_enc_len += encoder->encode(keys_shuffle[i], buffer);
    }
    time_end = getNow();
    double time_diff = time_end - time_start;
    double tput = keys_shuffle.size() / time_diff / 1000000; // in Mops/s
    double lat = time_diff * 1000000000 / total_len; // in ns
    double cpr = (total_len * 8.0) / total_enc_len;

    if (wkld_id < 0 || wkld_id > 2)
	std::cout << "ERROR: INVALID WKLD ID!" << std::endl;

    if (expt_id == 0) {
	if (wkld_id == kEmail) {
	    output_cpr_email_sample_size_sweep << cpr << "\n";
	    output_bt_email_sample_size_sweep << bt << "\n";
	} else if (wkld_id == kWiki) {
	    output_cpr_wiki_sample_size_sweep << cpr << "\n";
	    output_bt_wiki_sample_size_sweep << bt << "\n";
	} else if (wkld_id == kUrl) {
	    output_cpr_url_sample_size_sweep << cpr << "\n";
	    output_bt_url_sample_size_sweep << bt << "\n";
	}
    } else if (expt_id == 1) {
	if (wkld_id == kEmail) {
	    output_cpr_email_dict_size << cpr << "\n";
	    output_lat_email_dict_size << lat << "\n";
	} else if (wkld_id == kWiki) {
	    output_cpr_wiki_dict_size << cpr << "\n";
	    output_lat_wiki_dict_size << lat << "\n";
	} else if (wkld_id == kUrl) {
	    output_cpr_url_dict_size << cpr << "\n";
	    output_lat_url_dict_size << lat << "\n";
	}
    } else {
	std::cout << "ERROR: INVALID EXPT ID!" << std::endl;
    }
    
    std::cout << "Throughput = " << tput << " Mops/s" << std::endl;
    std::cout << "Latency = " << lat << " ns/char" << std::endl;
    std::cout << "CPR = " << cpr << std::endl;
}
} // namespace microbench

using namespace microbench;

int main(int argc, char *argv[]) {
    int expt_id = (int)atoi(argv[1]);
    
    std::vector<std::string> emails;
    std::vector<std::string> emails_shuffle;
    int64_t total_len_email = loadKeys(file_email, emails, emails_shuffle);

    std::vector<std::string> wikis;
    std::vector<std::string> wikis_shuffle;
    int64_t total_len_wiki = loadKeys(file_wiki, wikis, wikis_shuffle);

    std::vector<std::string> urls;
    std::vector<std::string> urls_shuffle;
    int64_t total_len_url = loadKeys(file_url, urls, urls_shuffle);

    if (expt_id == 0) {
	//-------------------------------------------------------------
	// Sample Size Sweep; Expt ID = 0
	//-------------------------------------------------------------
	std::cout << "------------------------------------------------" << std::endl;
	std::cout << "Sample Size Sweep; Expt ID = 0" << std::endl;
	std::cout << "------------------------------------------------" << std::endl;
	output_cpr_email_sample_size_sweep.open(file_cpr_email_sample_size_sweep);
	output_bt_email_sample_size_sweep.open(file_bt_email_sample_size_sweep);
	output_cpr_wiki_sample_size_sweep.open(file_cpr_wiki_sample_size_sweep);
	output_bt_wiki_sample_size_sweep.open(file_bt_wiki_sample_size_sweep);
	output_cpr_url_sample_size_sweep.open(file_cpr_url_sample_size_sweep);
	output_bt_url_sample_size_sweep.open(file_bt_url_sample_size_sweep);

	int dict_size_limit = 65536;
	int percent_list[3] = {100, 10, 1};
	int expt_num = 1;
	int total_num_expts = 36;
	for (int p = 0; p < 3; p++) {
	    int percent = percent_list[p];
	    for (int et = 1; et < 5; et++) {
		std::cout << "Sample Size Sweep (" << expt_num << "/" << total_num_expts << ")" << std::endl;
		exec(expt_id, kEmail, et, dict_size_limit, percent, emails_shuffle, total_len_email);
		expt_num++;
		std::cout << "Sample Size Sweep (" << expt_num << "/" << total_num_expts << ")" << std::endl;
		exec(expt_id, kWiki, et, dict_size_limit, percent, wikis_shuffle, total_len_wiki);
		expt_num++;
		std::cout << "Sample Size Sweep (" << expt_num << "/" << total_num_expts << ")" << std::endl;
		exec(expt_id, kUrl, et, dict_size_limit, percent, urls_shuffle, total_len_url);
		expt_num++;
	    }
	}

	output_cpr_email_sample_size_sweep.close();
	output_bt_email_sample_size_sweep.close();
	output_cpr_wiki_sample_size_sweep.close();
	output_bt_wiki_sample_size_sweep.close();
	output_cpr_url_sample_size_sweep.close();
	output_bt_url_sample_size_sweep.close();
    }
    else if (expt_id == 1) {
	//-------------------------------------------------------------
	// CPR and Latency; Expt ID = 1
	//-------------------------------------------------------------
	std::cout << "------------------------------------------------" << std::endl;
	std::cout << "CPR and Latency; Expt ID = 1" << std::endl;
	std::cout << "------------------------------------------------" << std::endl;
	output_cpr_email_dict_size.open(file_cpr_email_dict_size);
	output_lat_email_dict_size.open(file_lat_email_dict_size);
	output_cpr_wiki_dict_size.open(file_cpr_wiki_dict_size);
	output_lat_wiki_dict_size.open(file_lat_wiki_dict_size);
	output_cpr_url_dict_size.open(file_cpr_url_dict_size);
	output_lat_url_dict_size.open(file_lat_url_dict_size);

	int percent = 10;
	int dict_size_list[7] = {1024, 2048, 4096, 8192, 16384, 32768, 65536};
	int expt_num = 1;
	int total_num_expts = 48;
	// Single-Char
	std::cout << "CPR and Latency (" << expt_num << "/" << total_num_expts << ")" << std::endl;
	exec(expt_id, kEmail, 1, 1000, percent, emails_shuffle, total_len_email);
	expt_num++;
	std::cout << "CPR and Latency (" << expt_num << "/" << total_num_expts << ")" << std::endl;
	exec(expt_id, kWiki, 1, 1000, percent, wikis_shuffle, total_len_wiki);
	expt_num++;
	std::cout << "CPR and Latency (" << expt_num << "/" << total_num_expts << ")" << std::endl;
	exec(expt_id, kUrl, 1, 1000, percent, urls_shuffle, total_len_url);
	expt_num++;
	
	// Double-Char
	std::cout << "CPR and Latency (" << expt_num << "/" << total_num_expts << ")" << std::endl;
	exec(expt_id, kEmail, 2, 65536, percent, emails_shuffle, total_len_email);
	expt_num++;
	std::cout << "CPR and Latency (" << expt_num << "/" << total_num_expts << ")" << std::endl;
	exec(expt_id, kWiki, 2, 65536, percent, wikis_shuffle, total_len_wiki);
	expt_num++;
	std::cout << "CPR and Latency (" << expt_num << "/" << total_num_expts << ")" << std::endl;
	exec(expt_id, kUrl, 2, 65536, percent, urls_shuffle, total_len_url);
	expt_num++;
	
	for (int ds = 0; ds < 7; ds++) {
	    int dict_size_limit = dict_size_list[ds];
	    for (int et = 3; et < 5; et++) {
		std::cout << "CPR and Latency (" << expt_num << "/" << total_num_expts << ")" << std::endl;
		exec(expt_id, kEmail, et, dict_size_limit, percent, emails_shuffle, total_len_email);
		expt_num++;
		std::cout << "CPR and Latency (" << expt_num << "/" << total_num_expts << ")" << std::endl;
		exec(expt_id, kWiki, et, dict_size_limit, percent, wikis_shuffle, total_len_wiki);
		expt_num++;
		std::cout << "CPR and Latency (" << expt_num << "/" << total_num_expts << ")" << std::endl;
		exec(expt_id, kUrl, et, dict_size_limit, percent, urls_shuffle, total_len_url);
		expt_num++;
	    }
	}

	output_cpr_email_dict_size.close();
	output_lat_email_dict_size.close();
	output_cpr_wiki_dict_size.close();
	output_lat_wiki_dict_size.close();
	output_cpr_url_dict_size.close();
	output_lat_url_dict_size.close();
    }
    else if (expt_id == 2) {
	//-------------------------------------------------------------
	// Trie vs Array; Expt ID = 2
	//-------------------------------------------------------------
	std::cout << "------------------------------------------------" << std::endl;
	std::cout << "Trie vs Array; Expt ID = 2" << std::endl;
	std::cout << "------------------------------------------------" << std::endl;

	int percent = 10;
	int dict_size_limit = 65536;
	std::cout << "Trie vs Array (1/2)" << std::endl;
	exec(expt_id, kEmail, 3, dict_size_limit, percent, emails_shuffle, total_len_email);
	std::cout << "Trie vs Array (2/2)" << std::endl;
	exec(expt_id, kEmail, 4, dict_size_limit, percent, emails_shuffle, total_len_email);
    }
    else if (expt_id == 3) {
	//-------------------------------------------------------------
	// Build Time Breakdown; Expt ID = 3
	//-------------------------------------------------------------
	std::cout << "------------------------------------------------" << std::endl;
	std::cout << "Build Time Breakdown; Expt ID = 2" << std::endl;
	std::cout << "------------------------------------------------" << std::endl;

	int percent = 10;
	int dict_size_list[7] = {1024, 2048, 4096, 8192, 16384, 32768, 65536};
	int expt_num = 1;
	int total_num_expts = 7;
	for (int ds = 0; ds < 7; ds++) {
	    int dict_size_limit = dict_size_list[ds];
	    std::cout << "Build Time Breakdown (" << expt_num << "/" << total_num_expts << ")" << std::endl;
	    exec(expt_id, kEmail, 3, dict_size_limit, percent, emails_shuffle, total_len_email);
	    expt_num++;
	}
    }
    
    return 0;
}