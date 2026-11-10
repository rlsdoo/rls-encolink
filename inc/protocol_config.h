#ifndef _ENCO_REGISTER_ACCESS_PROTOCOL_CONFIG_H_
#define _ENCO_REGISTER_ACCESS_PROTOCOL_CONFIG_H_

/*
 * Select exactly one protocol version. It must match the product:
 *   AksIM-4 -> ENCO_PROTOCOL_4
 *   AksIM-2 -> ENCO_PROTOCOL_1
 * Define via compiler flag: -DENCO_PROTOCOL_4 or -DENCO_PROTOCOL_1,
 * or change the default below. Protocol 4 is used when nothing is defined.
 */
#if defined(ENCO_PROTOCOL_2)
#error "ENCO_PROTOCOL_2 is not supported. AksIM-2 encoders use EncoLink protocol 1: define ENCO_PROTOCOL_1 instead."
#endif

#if !defined(ENCO_PROTOCOL_4) && !defined(ENCO_PROTOCOL_1)
#define ENCO_PROTOCOL_4
#endif

#if defined(ENCO_PROTOCOL_4) && defined(ENCO_PROTOCOL_1)
#error "Define only one of ENCO_PROTOCOL_4 or ENCO_PROTOCOL_1, not both"
#endif

#if defined(ENCO_PROTOCOL_4)
#define ENCOLINK_PROTOCOL_VERSION 4u
#elif defined(ENCO_PROTOCOL_1)
#define ENCOLINK_PROTOCOL_VERSION 1u
#endif

#endif // _ENCO_REGISTER_ACCESS_PROTOCOL_CONFIG_H_

