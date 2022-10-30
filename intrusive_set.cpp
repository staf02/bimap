//
// Created by staf02 on 30/10/22.
//

#include "intrusive-set.h"

namespace intrusive {
std::default_random_engine set_element_base::engine =
    std::default_random_engine();
std::uniform_int_distribution<uint32_t> set_element_base::random_elem =
    std::uniform_int_distribution<uint32_t>(52);
}