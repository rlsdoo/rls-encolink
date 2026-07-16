#ifndef _ENCO_REGISTER_ACCESS_PRODUCT_CONFIG_H_
#define _ENCO_REGISTER_ACCESS_PRODUCT_CONFIG_H_

/*
 * Select exactly one product.
 * Define via compiler flag: -DENCO_PRODUCT_AKSIM4 or -DENCO_PRODUCT_AKSIM2,
 * or change the default below. AksIM-4 is used when nothing is defined.
 */
#if !defined(ENCO_PRODUCT_AKSIM4) && !defined(ENCO_PRODUCT_AKSIM2)
#define ENCO_PRODUCT_AKSIM4
#endif

#if defined(ENCO_PRODUCT_AKSIM4) && defined(ENCO_PRODUCT_AKSIM2)
#error "Define only one of ENCO_PRODUCT_AKSIM4 or ENCO_PRODUCT_AKSIM2, not both"
#endif

#if defined(ENCO_PRODUCT_AKSIM4)
#include "product_aksim4_map.h"
#elif defined(ENCO_PRODUCT_AKSIM2)
#include "product_aksim2_map.h"
#endif

#endif // _ENCO_REGISTER_ACCESS_PRODUCT_CONFIG_H_

