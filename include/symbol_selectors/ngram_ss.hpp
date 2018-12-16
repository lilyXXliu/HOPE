#ifndef NGRAM_SS_H
#define NGRAM_SS_H

//#define USE_CUCKOO 1

#include <algorithm>
#ifdef USE_CUCKOO
#include "cuckoohash_map.hh"
#else
#include <unordered_map>
#endif

#include "symbol_selector.hpp"

namespace ope {

class NGramSS : public SymbolSelector {
public:
    NGramSS(int n) : n_(n) {};
    ~NGramSS() {};

    bool selectSymbols (const std::vector<std::string>& key_list,
                        const int64_t num_limit,
                        std::vector<SymbolFreq>* symbol_freq_list);

private:
    void countSymbolFreq (const std::vector<std::string>& key_list);
    void pickMostFreqSymbols (const int64_t num_limit,
			      std::vector<std::string>* most_freq_symbols);
    void fillInGap (const std::vector<std::string>& most_freq_symbols);
    void fillInSingleChar (const int first, const int last);
    std::string commonPrefix (const std::string& str1, const std::string& str2);
    void countIntervalFreq (const std::vector<std::string>& key_list);
    int binarySearch(const std::string& key);

    int n_;
#ifdef USE_CUCKOO
    cuckoohash_map<std::string, int64_t> freq_map_;
#else
    std::unordered_map<std::string, int64_t> freq_map_;
#endif
    std::vector<std::string> interval_prefixes_;
    std::vector<std::string> interval_boundaries_;
    std::vector<int64_t> interval_freqs_;
};

bool NGramSS::selectSymbols (const std::vector<std::string>& key_list,
			     const int64_t num_limit,
			     std::vector<SymbolFreq>* symbol_freq_list) {
    if (key_list.empty())
        return false;
#ifdef USE_CUCKOO
    freq_map_.reserve(key_list.size());
#endif
    countSymbolFreq(key_list);
    std::vector<std::string> most_freq_symbols;
    //std::cout << freq_map_.size() << std::endl;
    pickMostFreqSymbols((num_limit / 2), &most_freq_symbols);
    fillInGap(most_freq_symbols);
    assert(interval_prefixes_.size() == interval_boundaries_.size());
    countIntervalFreq(key_list);
    assert(interval_prefixes_.size() == interval_freqs_.size());
    for (int i = 0; i < (int)interval_boundaries_.size(); i++) {
	symbol_freq_list->push_back(std::make_pair(interval_boundaries_[i],
						   interval_freqs_[i]));
    }
    return true;
}

void NGramSS::countSymbolFreq (const std::vector<std::string>& key_list) {
#ifdef PRINT_BUILD_TIME_BREAKDOWN
    double time_start = getNow();
#endif
#ifdef USE_CUCKOO
    auto updatefn = [](int64_t &num) { ++num; };
#else
    std::unordered_map<std::string, int64_t>::iterator iter;
#endif
    for (int i = 0; i < (int)key_list.size(); i++) {
        //for (int j = 0; j < (int)key_list[i].length() - 2; j++) {
	for (int j = 0; j < (int)key_list[i].length() - n_ + 1; j++) {
	    std::string ngram = key_list[i].substr(j, n_);
#ifdef USE_CUCKOO
	    freq_map_.upsert(ngram, updatefn, 1);
#else
	    iter = freq_map_.find(ngram);
	    if (iter == freq_map_.end())
		freq_map_.insert(std::pair<std::string, int64_t>(ngram, 1));
	    else
		iter->second += 1;
#endif
        }
    }
#ifdef PRINT_BUILD_TIME_BREAKDOWN
    double time_end = getNow();
    double time_diff = time_end - time_start;
    std::cout << "count symbol freq time = " << time_diff << std::endl;
#endif
}

void NGramSS::pickMostFreqSymbols (const int64_t num_limit,
				   std::vector<std::string>* most_freq_symbols) {
#ifdef PRINT_BUILD_TIME_BREAKDOWN
    double time_start = getNow();
#endif
    std::vector<SymbolFreq> symbol_freqs;
#ifdef USE_CUCKOO
    auto lt = freq_map_.lock_table();
    for (const auto &it : lt) {
	symbol_freqs.push_back(std::make_pair(it.first, it.second));
    }
#else
    std::unordered_map<std::string, int64_t>::iterator iter;
    for (iter = freq_map_.begin(); iter != freq_map_.end(); ++iter) {
	symbol_freqs.push_back(std::make_pair(iter->first, iter->second));
    }
#endif
    std::sort(symbol_freqs.begin(), symbol_freqs.end(),
	      [](const SymbolFreq& x, const SymbolFreq& y) {
		  if (x.second != y.second)
		      return x.second > y.second;
		  return (x.first.compare(y.first) > 0);
	      });
    for (int i = 0; i < num_limit; i++) {
	most_freq_symbols->push_back(symbol_freqs[i].first);
    }
    std::sort(most_freq_symbols->begin(), most_freq_symbols->end());
#ifdef PRINT_BUILD_TIME_BREAKDOWN
    double time_end = getNow();
    double time_diff = time_end - time_start;
    std::cout << "pick most freq symbols time = " << time_diff << std::endl;
#endif
}

void NGramSS::fillInGap (const std::vector<std::string>& most_freq_symbols) {
#ifdef PRINT_BUILD_TIME_BREAKDOWN
    double time_start = getNow();
#endif
    fillInSingleChar(0, (int)most_freq_symbols[0][0]);
    
    int num_symbols = most_freq_symbols.size();
    for (int i = 0; i < num_symbols - 1; i++) {
	std::string str1 = most_freq_symbols[i];
	std::string str2 = most_freq_symbols[i + 1];
	interval_prefixes_.push_back(str1);
	interval_boundaries_.push_back(str1);

	std::string str1_right_bound = str1;
	str1_right_bound[n_ - 1] += 1;
	if (str1_right_bound.compare(str2) != 0) {
	    interval_boundaries_.push_back(str1_right_bound);
	    if (str1[0] != str2[0]) {
		interval_prefixes_.push_back(std::string(1, str1[0]));
		fillInSingleChar((int)(uint8_t)(str1[0] + 1), (int)(uint8_t)str2[0]);
	    } else {
		std::string common_str = commonPrefix(str1, str2);
		interval_prefixes_.push_back(common_str);
	    }
	}
    }

    std::string last_str = most_freq_symbols[num_symbols - 1];
    interval_prefixes_.push_back(last_str);
    interval_boundaries_.push_back(last_str);
    std::string last_str_right_bound = last_str;
    last_str_right_bound[n_ - 1] += 1;
    interval_boundaries_.push_back(last_str_right_bound);
    interval_prefixes_.push_back(std::string(1, last_str[0]));

    fillInSingleChar((int)(uint8_t)(last_str[0] + 1), 255);
#ifdef PRINT_BUILD_TIME_BREAKDOWN
    double time_end = getNow();
    double time_diff = time_end - time_start;
    std::cout << "fill in gap time = " << time_diff << std::endl;
#endif
}

void NGramSS::fillInSingleChar (const int first, const int last) {
    for (int c = first; c <= last; c++) {
	interval_prefixes_.push_back(std::string(1, (char)c));
	interval_boundaries_.push_back(std::string(1, (char)c));
    }
}	

std::string NGramSS::commonPrefix(const std::string& str1,
				  const std::string& str2) {
    if (str1[0] != str2[0])
	return std::string();
    for (int i = 1; i < n_; i++) {
	if (str1[i] != str2[i])
	    return str1.substr(0, i);
    }
    assert(false);
    return std::string();
}

void NGramSS::countIntervalFreq (const std::vector<std::string>& key_list) {
#ifdef PRINT_BUILD_TIME_BREAKDOWN
    double time_start = getNow();
#endif
    for (int i = 0; i < (int)interval_prefixes_.size(); i++) {
	interval_freqs_.push_back(1);
    }
    
    for (int i = 0; i < (int)key_list.size(); i++) {
	int pos = 0;
	while (pos < (int)key_list[i].length()) {
	    std::string cur_str = key_list[i].substr(pos, n_ + 1);
	    int idx = binarySearch(cur_str);
	    interval_freqs_[idx]++;
	    pos += (int)interval_prefixes_[idx].length();
	}
    }
#ifdef PRINT_BUILD_TIME_BREAKDOWN
    double time_end = getNow();
    double time_diff = time_end - time_start;
    std::cout << "count interval freq time = " << time_diff << std::endl;
#endif
}

int NGramSS::binarySearch(const std::string& key) {
    int l = 0;
    int r = interval_boundaries_.size();
    int m = 0;
    while (r - l > 1) {
	m = (l + r) >> 1;
	std::string cur_key = interval_boundaries_[m];
	int cmp = key.compare(cur_key);
	if (cmp < 0) {
	    r = m;
	} else if (cmp == 0) {
	    return m;
	} else {
	    l = m;
	}
    }
    return l;
}

} // namespace ope

#endif // NGRAM_SS_H