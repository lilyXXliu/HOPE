#ifndef SYMBOL_SELECTOR_FACTORY_H
#define SYMBOL_SELECTOR_FACTORY_H

#include "ALMImproved_ss.hpp"
#include "double_char_ss.hpp"
#include "ALM_ss.hpp"
#include "ngram_ss.hpp"
#include "single_char_ss.hpp"
#include "symbol_selector.hpp"

namespace hope {

class SymbolSelectorFactory {
 public:
  static SymbolSelector *createSymbolSelector(const int type) {
    if (type == 1)
      return new SingleCharSS();
    else if (type == 2)
      return new DoubleCharSS();
    else if (type == 3)
      return new NGramSS(3);
    else if (type == 4)
      return new NGramSS(4);
    else if (type == 5)
      return new ALMSS();
    else if (type == 6)
      return new ALMImprovedSS();
    else
      return new SingleCharSS();
  }
};

}  // namespace hope

#endif  // SYMBOL_SELECTOR_FACTORY_H
