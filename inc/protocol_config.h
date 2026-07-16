#ifndef _ENCO_REGISTER_ACCESS_PROTOCOL_CONFIG_H_
#define _ENCO_REGISTER_ACCESS_PROTOCOL_CONFIG_H_

/*
 * Select exactly one protocol version.
 * Define via compiler flag: -DENCO_PROTOCOL_4 or -DENCO_PROTOCOL_2,
 * or change the default below. Protocol 4 is used when nothing is defined.
 */
#if !defined(ENCO_PROTOCOL_4) && !defined(ENCO_PROTOCOL_2)
#define ENCO_PROTOCOL_4
#endif

#if defined(ENCO_PROTOCOL_4) && defined(ENCO_PROTOCOL_2)
#error "Define only one of ENCO_PROTOCOL_4 or ENCO_PROTOCOL_2, not both"
#endif

#if defined(ENCO_PROTOCOL_4)
#define ENCOLINK_PROTOCOL_VERSION 4u
#elif defined(ENCO_PROTOCOL_2)
#define ENCOLINK_PROTOCOL_VERSION 2u
#endif

#endif // _ENCO_REGISTER_ACCESS_PROTOCOL_CONFIG_H_

