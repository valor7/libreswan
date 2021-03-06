/*
 * Libreswan config file parser (keywords.c)
 * Copyright (C) 2003-2006 Michael Richardson <mcr@xelerance.com>
 * Copyright (C) 2007-2010 Paul Wouters <paul@xelerance.com>
 * Copyright (C) 2012 Paul Wouters <paul@libreswan.org>
 * Copyright (C) 2013-2016 Paul Wouters <pwouters@redhat.com>
 * Copyright (C) 2013 D. Hugh Redelmeier <hugh@mimosa.com>
 * Copyright (C) 2013 David McCullough <ucdevel@gmail.com>
 * Copyright (C) 2013-2016 Antony Antony <antony@phenome.org>
 * Copyright (C) 2016-2017, Andrew Cagney
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 */

#include <sys/queue.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#ifndef _LIBRESWAN_H
#include <libreswan.h>
#include "constants.h"
#include "lmod.h"
#endif

#include "ipsecconf/keywords.h"
#include "ipsecconf/parser.h"	/* includes parser.tab.h generated by bison; requires keywords.h */
#include "ipsecconf/parserlast.h"

#define VALUES_INITIALIZER(t)	{ (t), elemsof(t) }

/*
 * Values for failureshunt={passthrough, drop, reject, none}
 */
static const struct keyword_enum_value kw_failureshunt_values[] = {
	{ "none",        KFS_FAIL_NONE },
	{ "passthrough", KFS_FAIL_PASS },
	{ "drop",        KFS_FAIL_DROP },
	{ "hold",        KFS_FAIL_DROP }, /* alias */
	{ "reject",      KFS_FAIL_REJECT },
};

static const struct keyword_enum_values kw_failureshunt_list = VALUES_INITIALIZER(kw_failureshunt_values);

/*
 * Values for negotiationshunt={passthrough, drop}
 */
static const struct keyword_enum_value kw_negotiationshunt_values[] = {
	{ "passthrough", KNS_FAIL_PASS },
	{ "drop",        KNS_FAIL_DROP },
	{ "hold",        KNS_FAIL_DROP }, /* alias */
};

static const struct keyword_enum_values kw_negotiationshunt_list = VALUES_INITIALIZER(kw_negotiationshunt_values);

/*
 * Values for keyexchange=
 */
static const struct keyword_enum_value kw_keyexchange_values[] = {
	{ "ike",  KE_IKE },
};

static const struct keyword_enum_values kw_keyexchange_list = VALUES_INITIALIZER(kw_keyexchange_values);

/*
 * Values for Four-State options, such as ikev2
 */
static const struct keyword_enum_value kw_fourvalued_values[] = {
	{ "never",     fo_never  },
	{ "permit",    fo_permit },
	{ "propose",   fo_propose },
	{ "insist",    fo_insist },
	{ "yes",       fo_propose },
	{ "always",    fo_insist },
	{ "no",        fo_never  }
};

static const struct keyword_enum_values kw_fourvalued_list = VALUES_INITIALIZER(kw_fourvalued_values);

/*
 * Values for yes/no/force, used by fragmentation=
 */
static const struct keyword_enum_value kw_ynf_values[] = {
	{ "never",     ynf_no },
	{ "no",        ynf_no },
	{ "yes",       ynf_yes },
	{ "insist",    ynf_force },
	{ "force",     ynf_force },
};
static const struct keyword_enum_values kw_ynf_list = VALUES_INITIALIZER(kw_ynf_values);

static const struct keyword_enum_value kw_esn_values[] = {
	{ "yes",     esn_yes  },
	{ "no",    esn_no },
	{ "either",   esn_either },
};
static const struct keyword_enum_values kw_esn_list = VALUES_INITIALIZER(kw_esn_values);

static const struct keyword_enum_value kw_encaps_values[] = {
	{ "yes",     encaps_yes  },
	{ "no",    encaps_no },
	{ "auto",   encaps_auto },
};
static const struct keyword_enum_values kw_encaps_list = VALUES_INITIALIZER(kw_encaps_values);

static const struct keyword_enum_value kw_ddos_values[] = {
	{ "auto",      DDOS_AUTO },
	{ "busy",      DDOS_FORCE_BUSY },
	{ "unlimited", DDOS_FORCE_UNLIMITED },
};
static const struct keyword_enum_values kw_ddos_list = VALUES_INITIALIZER(kw_ddos_values);

#ifdef HAVE_SECCOMP
static const struct keyword_enum_value kw_seccomp_values[] = {
	{ "enabled", SECCOMP_ENABLED },
	{ "disabled", SECCOMP_DISABLED },
	{ "tolerant", SECCOMP_TOLERANT },
};
static const struct keyword_enum_values kw_seccomp_list = VALUES_INITIALIZER(kw_seccomp_values);
#endif

static const struct keyword_enum_value kw_auth_lr_values[] = {
       { "never",     AUTH_NEVER },
       { "secret",    AUTH_PSK },
       { "rsasig",    AUTH_RSASIG },
       { "null",      AUTH_NULL },
 };
static const struct keyword_enum_values kw_auth_lr_list = VALUES_INITIALIZER(kw_auth_lr_values);

/*
 * Values for dpdaction={hold,clear,restart}
 */
static const struct keyword_enum_value kw_dpdaction_values[] = {
	{ "hold",    DPD_ACTION_HOLD },
	{ "clear",   DPD_ACTION_CLEAR },
	{ "restart",   DPD_ACTION_RESTART },
	/* obsoleted keyword - functionality moved into "restart" */
	{ "restart_by_peer",   DPD_ACTION_RESTART },
};

static const struct keyword_enum_values kw_dpdaction_list = VALUES_INITIALIZER(kw_dpdaction_values);

/*
 * Values for sendca={none,issuer,all}
 */

static const struct keyword_enum_value kw_sendca_values[] = {
	{ "none",	CA_SEND_NONE },
	{ "issuer",	CA_SEND_ISSUER },
	{ "all",	CA_SEND_ALL },
};

static const struct keyword_enum_values kw_sendca_list = VALUES_INITIALIZER(kw_sendca_values);

/*
 * Values for auto={add,start,route,ignore}
 */
static const struct keyword_enum_value kw_auto_values[] = {
	{ "ignore", STARTUP_IGNORE },
	/* no keyword for STARTUP_POLICY */
	{ "add",    STARTUP_ADD },
	{ "ondemand",  STARTUP_ONDEMAND },
	{ "route",  STARTUP_ONDEMAND }, /* backwards compatibility alias */
	{ "start",  STARTUP_START },
	{ "up",     STARTUP_START }, /* alias */
};

static const struct keyword_enum_values kw_auto_list = VALUES_INITIALIZER(kw_auto_values);

/*
 * Values for connaddrfamily={ipv4,ipv6}
 */
static const struct keyword_enum_value kw_connaddrfamily_values[] = {
	{ "ipv4",  AF_INET },
	{ "ipv6",  AF_INET6 },
	/* aliases - undocumented on purpose */
	{ "v4",    AF_INET },
	{ "inet",  AF_INET },
	{ "v6",    AF_INET6 },
	{ "inet6", AF_INET6 },
};

static const struct keyword_enum_values kw_connaddrfamily_list = VALUES_INITIALIZER(kw_connaddrfamily_values);

/*
 * Values for type={tunnel,transport,etc}
 */
static const struct keyword_enum_value kw_type_values[] = {
	{ "tunnel",    KS_TUNNEL },
	{ "transport", KS_TRANSPORT },
	{ "pass",      KS_PASSTHROUGH },
	{ "passthrough", KS_PASSTHROUGH },
	{ "reject",    KS_REJECT },
	{ "drop",      KS_DROP },
};

static const struct keyword_enum_values kw_type_list = VALUES_INITIALIZER(kw_type_values);

/*
 * Values for rsasigkey={ %cert, %dnsondemand, %dns, literal }
 */
static const struct keyword_enum_value kw_rsasigkey_values[] = {
	{ "",             PUBKEY_PREEXCHANGED },
	{ "%cert",        PUBKEY_CERTIFICATE },
#ifdef USE_DNSSEC
	{ "%dns",         PUBKEY_DNSONDEMAND },
	{ "%dnsondemand", PUBKEY_DNSONDEMAND },
#endif
};

static const struct keyword_enum_values kw_rsasigkey_list = VALUES_INITIALIZER(kw_rsasigkey_values);

/*
 * Values for protostack={netkey, klips, mast or none }
 */
static const struct keyword_enum_value kw_proto_stack_list[] = {
	{ "none",         NO_KERNEL },
	{ "klips",        USE_KLIPS },
	{ "mast",         USE_MASTKLIPS },
	{ "auto",         USE_NETKEY }, /* auto now means netkey */
	{ "netkey",       USE_NETKEY },
	{ "native",       USE_NETKEY },
	{ "bsd",          USE_BSDKAME },
	{ "kame",         USE_BSDKAME },
	{ "bsdkame",      USE_BSDKAME },
	{ "win2k",        USE_WIN2K },
};

static const struct keyword_enum_values kw_proto_stack = VALUES_INITIALIZER(kw_proto_stack_list);

/*
 * Values for sareftrack={yes, no, conntrack }
 */
static const struct keyword_enum_value kw_sareftrack_values[] = {
	{ "yes",          sat_yes },
	{ "no",           sat_no },
	{ "conntrack",    sat_conntrack },
};

static const struct keyword_enum_values kw_sareftrack_list = VALUES_INITIALIZER(kw_sareftrack_values);

/*
 *  Cisco interop: remote peer type
 */

static const struct keyword_enum_value kw_remote_peer_type_list[] = {
	{ "cisco",         CISCO },
};

static const struct keyword_enum_values kw_remote_peer_type = VALUES_INITIALIZER(kw_remote_peer_type_list);

static const struct keyword_enum_value kw_xauthby_list[] = {
	{ "file",	XAUTHBY_FILE },
#ifdef XAUTH_HAVE_PAM
	{ "pam",	XAUTHBY_PAM },
#endif
	{ "alwaysok",	XAUTHBY_ALWAYSOK },
};

static const struct keyword_enum_values kw_xauthby = VALUES_INITIALIZER(kw_xauthby_list);

static const struct keyword_enum_value kw_xauthfail_list[] = {
	{ "hard",         XAUTHFAIL_HARD },
	{ "soft",         XAUTHFAIL_SOFT },
};

static const struct keyword_enum_values kw_xauthfail = VALUES_INITIALIZER(kw_xauthfail_list);

/*
 * Values for right= and left=
 */

static const struct keyword_enum_value kw_klipsdebug_values[] = {
	{ "all",      LRANGE(KDF_XMIT, KDF_COMP) },
	{ "none",     0 },
	{ "verbose",  LELEM(KDF_VERBOSE) },
	{ "xmit",     LELEM(KDF_XMIT) },
	{ "tunnel-xmit", LELEM(KDF_XMIT) },
	{ "netlink",  LELEM(KDF_NETLINK) },
	{ "xform",    LELEM(KDF_XFORM) },
	{ "eroute",   LELEM(KDF_EROUTE) },
	{ "spi",      LELEM(KDF_SPI) },
	{ "radij",    LELEM(KDF_RADIJ) },
	{ "esp",      LELEM(KDF_ESP) },
	{ "ah",       LELEM(KDF_AH) },
	{ "rcv",      LELEM(KDF_RCV) },
	{ "tunnel",   LELEM(KDF_TUNNEL) },
	{ "pfkey",    LELEM(KDF_PFKEY) },
	{ "comp",     LELEM(KDF_COMP) },
	{ "nat-traversal", LELEM(KDF_NATT) },
	{ "nattraversal", LELEM(KDF_NATT) },
	{ "natt",     LELEM(KDF_NATT) },
};

static const struct keyword_enum_values kw_klipsdebug_list = VALUES_INITIALIZER(kw_klipsdebug_values);

static const struct keyword_enum_value kw_phase2types_values[] = {
	/* note: these POLICY bits happen to fit in an unsigned int */
	/* note2: ah+esp is no longer supported as per RFC-8221 Section 4 */
	{ "esp",      POLICY_ENCRYPT },
	{ "ah",       POLICY_AUTHENTICATE },
	{ "default",  POLICY_ENCRYPT }, /* alias, find it last */
};

static const struct keyword_enum_values kw_phase2types_list = VALUES_INITIALIZER(kw_phase2types_values);

/*
 * Values for {left/right}sendcert={never,sendifasked,always,forcedtype}
 */
static const struct keyword_enum_value kw_sendcert_values[] = {
	{ "never",        cert_neversend },
	{ "sendifasked",  cert_sendifasked },
	{ "alwayssend",   cert_alwayssend },
	{ "always",       cert_alwayssend },
};

static const struct keyword_enum_values kw_sendcert_list = VALUES_INITIALIZER(kw_sendcert_values);

/*
 * Values for nat-ikev1-method={drafts,rfc,both,none}
 */
static const struct keyword_enum_value kw_ikev1natt_values[] = {
	{ "both",       natt_both },
	{ "rfc",        natt_rfc },
	{ "drafts",     natt_drafts },
	{ "none",       natt_none },
};

static const struct keyword_enum_values kw_ikev1natt_list = VALUES_INITIALIZER(kw_ikev1natt_values);

/*
 * Values for ocsp-method={get|post}
 *
 * This sets the NSS forcePost option for the OCSP request.
 * If forcePost is set, OCSP requests will only be sent using the HTTP POST
 * method. When forcePost is not set, OCSP requests will be sent using the
 * HTTP GET method, with a fallback to POST when we fail to receive a response
 * and/or when we receive an uncacheable response like "Unknown".
 */
static const struct keyword_enum_value kw_ocsp_method_values[] = {
	{ "get",      OCSP_METHOD_GET },
	{ "post",     OCSP_METHOD_POST },
};
static const struct keyword_enum_values kw_ocsp_method_list = VALUES_INITIALIZER(kw_ocsp_method_values);

#ifdef USE_NIC_OFFLOAD
static const struct keyword_enum_value kw_nic_offload_values[] = {
	{ "no",   nic_offload_no  },
	{ "yes",  nic_offload_yes },
	{ "auto",    nic_offload_auto },
};

static const struct keyword_enum_values kw_nic_offload_list = VALUES_INITIALIZER(kw_nic_offload_values);
#endif

/* MASTER KEYWORD LIST
 * Note: this table is terminated by an entry with keyname == NULL.
 */

const struct keyword_def ipsec_conf_keywords[] = {
  { "interfaces",  kv_config,  kt_string,  KSF_INTERFACES, NULL, NULL, },
  { "curl-iface",  kv_config,  kt_string,  KSF_CURLIFACE, NULL, NULL, },
  { "curl-timeout",  kv_config,  kt_time,  KBF_CURLTIMEOUT, NULL, NULL, },

  /*
   * These two aliases are needed because "-" versions could not work
   * on openswan shell scripts. They only exist on rhel openwan 6.7+
   * So they are needed to support RHEL openswan -> libreswan migration.
   */
  { "curl_iface",  kv_config | kv_alias,  kt_string,  KSF_CURLIFACE, NULL, NULL, },  /* obsolete _ */
  { "curl_timeout",  kv_config | kv_alias,  kt_time,  KBF_CURLTIMEOUT, NULL, NULL, },  /* obsolete _ */

  { "myvendorid",  kv_config,  kt_string,  KSF_MYVENDORID, NULL, NULL, },
  { "syslog",  kv_config,  kt_string,  KSF_SYSLOG, NULL, NULL, },
  { "klipsdebug",  kv_config,  kt_list,  KBF_KLIPSDEBUG,  &kw_klipsdebug_list, NULL, },
  { "plutodebug",  kv_config,  kt_lset,  KBF_PLUTODEBUG, NULL, &debug_lmod_info, },
  { "logfile",  kv_config,  kt_filename,  KSF_PLUTOSTDERRLOG, NULL, NULL, },
  { "plutostderrlog",  kv_config | kv_alias,  kt_filename,  KSF_PLUTOSTDERRLOG, NULL, NULL, },  /* obsolete */
  { "logtime",  kv_config,  kt_bool,  KBF_PLUTOSTDERRLOGTIME, NULL, NULL, },
  { "plutostderrlogtime",  kv_config | kv_alias,  kt_bool,  KBF_PLUTOSTDERRLOGTIME, NULL, NULL, },  /* obsolete */
  { "logappend",  kv_config,  kt_bool,  KBF_PLUTOSTDERRLOGAPPEND, NULL, NULL, },
  { "logip",  kv_config,  kt_bool,  KBF_PLUTOSTDERRLOGIP, NULL, NULL, },
#ifdef USE_DNSSEC
  { "dnssec-enable",  kv_config,  kt_bool,  KBF_DO_DNSSEC, NULL, NULL, },
  { "dnssec-rootkey-file",  kv_config,  kt_filename, KSF_PLUTO_DNSSEC_ROOTKEY_FILE, NULL, NULL, },
  { "dnssec-anchors",  kv_config,  kt_filename, KSF_PLUTO_DNSSEC_ANCHORS, NULL, NULL, },
#endif
  { "dumpdir",  kv_config,  kt_dirname,  KSF_DUMPDIR, NULL, NULL, },
  { "ipsecdir",  kv_config,  kt_dirname,  KSF_IPSECDIR, NULL, NULL, },
  { "nssdir", kv_config, kt_dirname, KSF_NSSDIR, NULL, NULL, },
  { "secretsfile",  kv_config,  kt_dirname,  KSF_SECRETSFILE, NULL, NULL, },
  { "statsbin",  kv_config,  kt_dirname,  KSF_STATSBINARY, NULL, NULL, },
  { "perpeerlog",  kv_config,  kt_bool,  KBF_PERPEERLOG, NULL, NULL, },
  { "perpeerlogdir",  kv_config,  kt_dirname,  KSF_PERPEERDIR, NULL, NULL, },
  { "fragicmp",  kv_config,  kt_bool,  KBF_FRAGICMP, NULL, NULL, },
  { "hidetos",  kv_config,  kt_bool,  KBF_HIDETOS, NULL, NULL, },
  { "uniqueids",  kv_config,  kt_bool,  KBF_UNIQUEIDS, NULL, NULL, },
  { "shuntlifetime",  kv_config,  kt_time,  KBF_SHUNTLIFETIME, NULL, NULL, },
  { "overridemtu",  kv_config,  kt_number,  KBF_OVERRIDEMTU, NULL, NULL, },

  { "crl-strict",  kv_config,  kt_bool,  KBF_CRL_STRICT, NULL, NULL, },
  { "crl_strict",  kv_config | kv_alias,  kt_bool,  KBF_CRL_STRICT, NULL, NULL, },  /* obsolete _ */
  { "crlcheckinterval",  kv_config,  kt_time,  KBF_CRL_CHECKINTERVAL, NULL, NULL, },
  { "strictcrlpolicy",  kv_config | kv_alias,  kt_bool,  KBF_CRL_STRICT, NULL, NULL, },  /* obsolete used on openswan */

  { "ocsp-strict",  kv_config,  kt_bool,  KBF_OCSP_STRICT, NULL, NULL, },
  { "ocsp_strict",  kv_config | kv_alias,  kt_bool,  KBF_OCSP_STRICT, NULL, NULL, },  /* obsolete _ */
  { "ocsp-enable",  kv_config,  kt_bool,  KBF_OCSP_ENABLE, NULL, NULL, },
  { "ocsp_enable",  kv_config | kv_alias,  kt_bool,  KBF_OCSP_ENABLE, NULL, NULL, },  /* obsolete _ */
  { "ocsp-uri",  kv_config,  kt_string,  KSF_OCSP_URI, NULL, NULL, },
  { "ocsp_uri",  kv_config | kv_alias,  kt_string,  KSF_OCSP_URI, NULL, NULL, },  /* obsolete _ */
  { "ocsp-timeout",  kv_config,  kt_number,  KBF_OCSP_TIMEOUT, NULL, NULL, },
  { "ocsp_timeout",  kv_config | kv_alias,  kt_number,  KBF_OCSP_TIMEOUT, NULL, NULL, },  /* obsolete _ */
  { "ocsp-trustname",  kv_config,  kt_string,  KSF_OCSP_TRUSTNAME, NULL, NULL, },
  { "ocsp_trust_name",  kv_config | kv_alias,  kt_string,  KSF_OCSP_TRUSTNAME, NULL, NULL, },  /* obsolete _ */
  { "ocsp-cache-size",  kv_config,  kt_number,  KBF_OCSP_CACHE_SIZE, NULL, NULL, },
  { "ocsp-cache-min-age",  kv_config,  kt_time,  KBF_OCSP_CACHE_MIN, NULL, NULL, },
  { "ocsp-cache-max-age",  kv_config,  kt_time,  KBF_OCSP_CACHE_MAX, NULL, NULL, },
  { "ocsp-method",  kv_config | kv_processed,  kt_enum,  KBF_OCSP_METHOD,  &kw_ocsp_method_list, NULL, },

  { "ddos-mode",  kv_config | kv_processed ,  kt_enum,  KBF_DDOS_MODE,  &kw_ddos_list, NULL, },
#ifdef HAVE_SECCOMP
  { "seccomp",  kv_config | kv_processed ,  kt_enum,  KBF_SECCOMP,  &kw_seccomp_list, NULL, },
#endif
  { "ddos-ike-threshold",  kv_config,  kt_number,  KBF_DDOS_IKE_THRESHOLD, NULL, NULL, },
  { "max-halfopen-ike",  kv_config,  kt_number,  KBF_MAX_HALFOPEN_IKE, NULL, NULL, },
  { "ikeport",  kv_config,  kt_number,  KBF_IKEPORT, NULL, NULL, },
  { "ike-socket-bufsize",  kv_config,  kt_number,  KBF_IKEBUF, NULL, NULL, },
  { "ike-socket-errqueue",  kv_config,  kt_bool,  KBF_IKE_ERRQUEUE, NULL, NULL, },
  { "nflog-all",  kv_config,  kt_number,  KBF_NFLOG_ALL, NULL, NULL, },
  { "xfrmlifetime",  kv_config,  kt_number,  KBF_XFRMLIFETIME, NULL, NULL, },
  { "virtual_private",  kv_config | kv_alias,  kt_string,  KSF_VIRTUALPRIVATE, NULL, NULL, },  /* obsolete _ */
  { "virtual-private",  kv_config,  kt_string,  KSF_VIRTUALPRIVATE, NULL, NULL, },
  { "nat_ikeport",  kv_config | kv_alias,  kt_number,  KBF_NATIKEPORT, NULL, NULL, },  /* obsolete _ */
  { "nat-ikeport",  kv_config,  kt_number,  KBF_NATIKEPORT, NULL, NULL, },
  { "seedbits",  kv_config,  kt_number,  KBF_SEEDBITS, NULL, NULL, },
  { "keep_alive",  kv_config | kv_alias,  kt_number,  KBF_KEEPALIVE, NULL, NULL, },  /* obsolete _ */
  { "keep-alive",  kv_config,  kt_number,  KBF_KEEPALIVE, NULL, NULL, },

  { "listen",  kv_config,  kt_string,  KSF_LISTEN, NULL, NULL, },
  { "protostack",  kv_config,  kt_string,  KSF_PROTOSTACK,  &kw_proto_stack, NULL, },
  { "nhelpers",  kv_config,  kt_number,  KBF_NHELPERS, NULL, NULL, },
  { "drop-oppo-null",  kv_config,  kt_bool,  KBF_DROP_OPPO_NULL, NULL, NULL, },
#ifdef HAVE_LABELED_IPSEC
  /* ??? AN ATTRIBUTE TYPE, NOT VALUE! */
  { "secctx_attr_value",  kv_config | kv_alias,  kt_number,  KBF_SECCTX, NULL, NULL, },  /* obsolete _ */
  { "secctx-attr-value",  kv_config,  kt_number,  KBF_SECCTX, NULL, NULL, },  /* obsolete: not a value, a type */
  { "secctx-attr-type",  kv_config,  kt_number,  KBF_SECCTX, NULL, NULL, },
#endif

  /* these options are obsoleted (and not old aliases) */
  { "plutorestartoncrash",  kv_config,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },
  { "forwardcontrol",  kv_config,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },
  { "rp_filter",  kv_config,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },
  { "pluto",  kv_config,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },
  { "prepluto",  kv_config,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },
  { "postpluto",  kv_config,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },
  { "plutoopts",  kv_config,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },
  { "plutowait",  kv_config,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },
  { "nocrsend",  kv_config,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },
  { "nat_traversal",  kv_config,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },
  { "disable_port_floating",  kv_config,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },
  { "force_keepalive",  kv_config,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },
  { "plutofork",  kv_config | kv_alias,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },  /* obsolete */
  { "force-busy",  kv_config,  kt_obsolete, KBF_WARNIGNORE, NULL, NULL, },  /* obsoleted for ddos-mode=busy */
  { "force-busy",  kv_config,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },  /* obsoleted for ddos-mode=busy */
  { "oe",  kv_config,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },

  /*
   * This is "left=" and "right="
   */
  { "",  kv_conn | kv_leftright,  kt_loose_enum,  KSCF_IP,  &kw_host_list, NULL, },

  { "ike",  kv_conn,  kt_string,  KSCF_IKE, NULL, NULL, },

  { "subnet",  kv_conn | kv_leftright | kv_processed,  kt_subnet,  KSCF_SUBNET, NULL, NULL, },
  { "subnets",  kv_conn | kv_leftright,  kt_appendlist,  KSCF_SUBNETS, NULL, NULL, },
  { "sourceip",  kv_conn | kv_leftright,  kt_ipaddr,  KSCF_SOURCEIP, NULL, NULL, },
  { "vti",  kv_conn | kv_leftright | kv_processed,  kt_subnet,  KSCF_VTI_IP, NULL, NULL, },
  { "nexthop",  kv_conn | kv_leftright,  kt_ipaddr,  KSCF_NEXTHOP, NULL, NULL, },
  { "updown",  kv_conn | kv_leftright,  kt_filename,  KSCF_UPDOWN, NULL, NULL, },
  { "id",  kv_conn | kv_leftright,  kt_idtype,  KSCF_ID, NULL, NULL, },
  { "rsasigkey",  kv_conn | kv_leftright,  kt_rsakey,  KSCF_RSAKEY1,  &kw_rsasigkey_list, NULL, },
  { "rsasigkey2",  kv_conn | kv_leftright,  kt_rsakey,  KSCF_RSAKEY2,  &kw_rsasigkey_list, NULL, },
  { "cert",  kv_conn | kv_leftright,  kt_filename,  KSCF_CERT, NULL, NULL, },
  { "ckaid",  kv_conn | kv_leftright,  kt_string,  KSCF_CKAID, NULL, NULL, },
  { "sendcert",  kv_conn | kv_leftright,  kt_enum,  KNCF_SENDCERT,  &kw_sendcert_list, NULL, },
  { "ca",  kv_conn | kv_leftright,  kt_string,  KSCF_CA, NULL, NULL, },
  { "xauthserver",  kv_conn | kv_leftright,  kt_bool,  KNCF_XAUTHSERVER, NULL, NULL, },
  { "xauthclient",  kv_conn | kv_leftright,  kt_bool,  KNCF_XAUTHCLIENT, NULL, NULL, },
  { "modecfgserver",  kv_conn | kv_leftright,  kt_bool,  KNCF_MODECONFIGSERVER, NULL, NULL, },
  { "modecfgclient",  kv_conn | kv_leftright,  kt_bool,  KNCF_MODECONFIGCLIENT, NULL, NULL, },
  { "username",  kv_conn | kv_leftright,  kt_string,  KSCF_USERNAME, NULL, NULL, },
  { "xauthusername",  kv_conn | kv_leftright | kv_alias,  kt_string,  KSCF_USERNAME, NULL, NULL, },  /* obsolete name */
  { "xauthname",  kv_conn | kv_leftright | kv_alias,  kt_string,  KSCF_USERNAME, NULL, NULL, },  /* obsolete name */
  { "addresspool",  kv_conn | kv_leftright,  kt_range,  KSCF_ADDRESSPOOL, NULL, NULL, },
  { "auth",  kv_conn | kv_leftright, kt_enum,  KNCF_AUTH,  &kw_auth_lr_list, NULL, },

  /* these are conn statements which are not left/right */
  { "auto",  kv_conn | kv_duplicateok,  kt_enum,  KBF_AUTO,  &kw_auto_list, NULL, },
  { "also",  kv_conn,  kt_appendstring,  KSCF_ALSO, NULL, NULL, },
  { "alsoflip",  kv_conn,  kt_string,  KSCF_ALSOFLIP, NULL, NULL, },
  { "connaddrfamily",  kv_conn,  kt_enum,  KBF_CONNADDRFAMILY,  &kw_connaddrfamily_list, NULL, },
  { "type",  kv_conn,  kt_enum,  KBF_TYPE,  &kw_type_list, NULL, },
  { "authby",  kv_conn,  kt_string,  KSCF_AUTHBY, NULL, NULL, },
  { "keyexchange",  kv_conn,  kt_enum,  KBF_KEYEXCHANGE,  &kw_keyexchange_list, NULL, },
  { "ikev2",  kv_conn | kv_processed,  kt_enum,  KBF_IKEv2,  &kw_fourvalued_list, NULL, },
  { "ppk", kv_conn | kv_processed, kt_enum, KBF_PPK, &kw_fourvalued_list, NULL, },
  { "esn",  kv_conn | kv_processed,  kt_enum,  KBF_ESN,  &kw_esn_list, NULL, },
  { "decap-dscp",  kv_conn | kv_processed,  kt_bool,  KBF_DECAP_DSCP,  NULL, NULL, },
  { "nopmtudisc",  kv_conn | kv_processed,  kt_bool,  KBF_NOPMTUDISC,  NULL, NULL, },
  { "ike_frag",  kv_conn | kv_processed | kv_alias,  kt_enum,  KBF_IKE_FRAG,  &kw_ynf_list, NULL, },  /* obsolete _ */
  { "ike-frag",  kv_conn | kv_processed | kv_alias,  kt_enum,  KBF_IKE_FRAG,  &kw_ynf_list, NULL, },  /* obsolete name */
  { "fragmentation",  kv_conn | kv_processed,  kt_enum,  KBF_IKE_FRAG,  &kw_ynf_list, NULL, },
  { "mobike",  kv_conn,  kt_bool,  KBF_MOBIKE, NULL, NULL, },
  { "narrowing",  kv_conn,  kt_bool,  KBF_IKEv2_ALLOW_NARROWING, NULL, NULL, },
  { "pam-authorize",  kv_conn,  kt_bool,  KBF_IKEv2_PAM_AUTHORIZE, NULL, NULL, },
  { "sareftrack",  kv_conn | kv_processed,  kt_enum,  KBF_SAREFTRACK,  &kw_sareftrack_list, NULL, },
  { "pfs",  kv_conn,  kt_bool,  KBF_PFS, NULL, NULL, },

  { "nat_keepalive",  kv_conn | kv_alias,  kt_bool,  KBF_NAT_KEEPALIVE, NULL, NULL, },  /* obsolete _ */
  { "nat-keepalive",  kv_conn,  kt_bool,  KBF_NAT_KEEPALIVE, NULL, NULL, },

  { "initial_contact",  kv_conn | kv_alias,  kt_bool,  KBF_INITIAL_CONTACT, NULL, NULL, },  /* obsolete _ */
  { "initial-contact",  kv_conn,  kt_bool,  KBF_INITIAL_CONTACT, NULL, NULL, },
  { "cisco_unity",  kv_conn | kv_alias,  kt_bool,  KBF_CISCO_UNITY, NULL, NULL, },  /* obsolete _ */
  { "cisco-unity",  kv_conn,  kt_bool,  KBF_CISCO_UNITY, NULL, NULL, },
  { "send-no-esp-tfc",  kv_conn,  kt_bool,  KBF_NO_ESP_TFC, NULL, NULL, },
  { "fake-strongswan",  kv_conn,  kt_bool,  KBF_VID_STRONGSWAN, NULL, NULL, },
  { "send_vendorid",  kv_conn | kv_alias,  kt_bool,  KBF_SEND_VENDORID, NULL, NULL, },  /* obsolete _ */
  { "send-vendorid",  kv_conn,  kt_bool,  KBF_SEND_VENDORID, NULL, NULL, },
  { "sha2_truncbug",  kv_conn | kv_alias,  kt_bool,  KBF_SHA2_TRUNCBUG, NULL, NULL, },  /* obsolete _ */
  { "sha2-truncbug",  kv_conn,  kt_bool,  KBF_SHA2_TRUNCBUG, NULL, NULL, },
  { "keylife",  kv_conn | kv_alias,  kt_time,  KBF_SALIFETIME, NULL, NULL, },
  { "lifetime",  kv_conn | kv_alias,  kt_time,  KBF_SALIFETIME, NULL, NULL, },
  { "salifetime",  kv_conn,  kt_time,  KBF_SALIFETIME, NULL, NULL, },

  { "retransmit-timeout",  kv_conn,  kt_time,  KBF_RETRANSMIT_TIMEOUT, NULL, NULL, },
  { "retransmit-interval",  kv_conn,  kt_number,  KBF_RETRANSMIT_INTERVAL_MS, NULL, NULL, },

  {"ikepad",  kv_conn,  kt_bool,  KBF_IKEPAD, NULL, NULL, },
  { "nat-ikev1-method",  kv_conn | kv_processed,  kt_enum,  KBF_IKEV1_NATT,  &kw_ikev1natt_list, NULL, },
#ifdef HAVE_LABELED_IPSEC
  { "labeled_ipsec",  kv_conn | kv_alias,  kt_bool,  KBF_LABELED_IPSEC, NULL, NULL, },  /* obsolete _ */
  { "labeled-ipsec",  kv_conn,  kt_bool,  KBF_LABELED_IPSEC, NULL, NULL, },
  { "policy_label",  kv_conn | kv_alias,  kt_string,  KSCF_POLICY_LABEL, NULL, NULL, },  /* obsolete _ */
  { "policy-label",  kv_conn,  kt_string,  KSCF_POLICY_LABEL, NULL, NULL, },
#endif

  /* Cisco interop: remote peer type */
  { "remote_peer_type",  kv_conn | kv_alias,  kt_enum,  KBF_REMOTEPEERTYPE,  &kw_remote_peer_type, NULL, },  /* obsolete _ */
  { "remote-peer-type",  kv_conn,  kt_enum,  KBF_REMOTEPEERTYPE,  &kw_remote_peer_type, NULL, },

  /* Network Manager support */
#ifdef HAVE_NM
  { "nm_configured",  kv_conn | kv_alias,  kt_bool,  KBF_NMCONFIGURED, NULL, NULL, },  /* obsolete _ */
  { "nm-configured",  kv_conn,  kt_bool,  KBF_NMCONFIGURED, NULL, NULL, },
#endif

  { "xauthby",  kv_conn,  kt_enum,  KBF_XAUTHBY,  &kw_xauthby, NULL, },
  { "xauthfail",  kv_conn,  kt_enum,  KBF_XAUTHFAIL,  &kw_xauthfail, NULL, },
  { "modecfgpull",  kv_conn,  kt_invertbool,  KBF_MODECONFIGPULL, NULL, NULL, },
  { "modecfgdns",  kv_conn,  kt_string,  KSCF_MODECFGDNS, NULL, NULL, },
  { "modecfgdns1",  kv_conn | kv_alias, kt_string, KSCF_MODECFGDNS, NULL, NULL, }, /* obsolete */
  { "modecfgdns2",  kv_conn, kt_obsolete, KBF_WARNIGNORE, NULL, NULL, }, /* obsolete */
  { "modecfgdomains",  kv_conn,  kt_string,  KSCF_MODECFGDOMAINS, NULL, NULL, },
  { "modecfgdomain",  kv_conn | kv_alias,  kt_string,  KSCF_MODECFGDOMAINS, NULL, NULL, }, /* obsolete */
  { "modecfgbanner",  kv_conn,  kt_string,  KSCF_MODECFGBANNER, NULL, NULL, },
  { "mark",  kv_conn,  kt_string,  KSCF_CONN_MARK_BOTH, NULL, NULL, },
  { "mark-in",  kv_conn,  kt_string,  KSCF_CONN_MARK_IN, NULL, NULL, },
  { "mark-out",  kv_conn,  kt_string,  KSCF_CONN_MARK_OUT, NULL, NULL, },
  { "vti-interface",  kv_conn,  kt_string,  KSCF_VTI_IFACE, NULL, NULL, },
  { "vti-routing",  kv_conn,  kt_bool,  KBF_VTI_ROUTING, NULL, NULL, },
  { "vti-shared",  kv_conn,  kt_bool,  KBF_VTI_SHARED, NULL, NULL, },

  { "modecfgwins1",  kv_conn,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },
  { "modecfgwins2",  kv_conn,  kt_obsolete,  KBF_WARNIGNORE, NULL, NULL, },

#ifdef USE_NIC_OFFLOAD
  { "nic-offload",  kv_conn,  kt_enum,  KBF_NIC_OFFLOAD,  &kw_nic_offload_list, NULL, },
#endif

  { "encapsulation",  kv_conn,  kt_enum,  KBF_ENCAPS,  &kw_encaps_list, NULL, },
  { "forceencaps",  kv_conn, kt_obsolete, KBF_WARNIGNORE, NULL, NULL, },

  { "cat",  kv_conn | kv_leftright,  kt_bool,  KNCF_CAT, NULL, NULL, },

  { "overlapip",  kv_conn,  kt_bool,  KBF_OVERLAPIP, NULL, NULL, },
  { "rekey",  kv_conn,  kt_bool,  KBF_REKEY, NULL, NULL, },
  { "rekeymargin",  kv_conn,  kt_time,  KBF_REKEYMARGIN, NULL, NULL, },
  { "rekeyfuzz",  kv_conn,  kt_percent,  KBF_REKEYFUZZ, NULL, NULL, },
  { "keyingtries",  kv_conn,  kt_number,  KBF_KEYINGTRIES, NULL, NULL, },
  { "replay-window",  kv_conn,  kt_number,  KBF_REPLAY_WINDOW, NULL, NULL, },
  { "ikelifetime",  kv_conn,  kt_time,  KBF_IKELIFETIME, NULL, NULL, },
  { "disablearrivalcheck",  kv_conn,  kt_invertbool,  KBF_ARRIVALCHECK, NULL, NULL, },
  { "failureshunt",  kv_conn,  kt_enum,  KBF_FAILURESHUNT,  &kw_failureshunt_list, NULL, },
  { "negotiationshunt",  kv_conn,  kt_enum,  KBF_NEGOTIATIONSHUNT,  &kw_negotiationshunt_list, NULL, },
  { "connalias",  kv_conn | kv_processed,  kt_appendstring,  KSCF_CONNALIAS, NULL, NULL, },

  /* attributes of the phase2 policy */
  { "phase2alg",  kv_conn,  kt_string,  KSCF_ESP, NULL, NULL, },
  { "esp",  kv_conn | kv_alias,  kt_string,  KSCF_ESP, NULL, NULL, },
  { "ah",  kv_conn | kv_alias,  kt_string,  KSCF_ESP, NULL, NULL, },
  { "protoport",  kv_conn | kv_leftright | kv_processed,  kt_string,  KSCF_PROTOPORT, NULL, NULL, },

  { "phase2",  kv_conn | kv_policy,  kt_enum,  KBF_PHASE2,  &kw_phase2types_list, NULL, },

  { "compress",  kv_conn,  kt_bool,  KBF_COMPRESS, NULL, NULL, },

  /* route metric */
  { "metric",  kv_conn,  kt_number,  KBF_METRIC, NULL, NULL, },

  /* DPD */
  { "dpddelay",  kv_conn,  kt_time,  KBF_DPDDELAY, NULL, NULL, },
  { "dpdtimeout",  kv_conn,  kt_time,  KBF_DPDTIMEOUT, NULL, NULL, },
  { "dpdaction",  kv_conn,  kt_enum,  KBF_DPDACTION,  &kw_dpdaction_list, NULL, },

  { "sendca",      kv_conn,  kt_enum,  KBF_SEND_CA,  &kw_sendca_list, NULL, },

  { "mtu",  kv_conn,  kt_number,  KBF_CONNMTU, NULL, NULL, },
  { "priority",  kv_conn,  kt_number,  KBF_PRIORITY, NULL, NULL, },
  { "tfc",  kv_conn,  kt_number,  KBF_TFCPAD, NULL, NULL, },
  { "reqid",  kv_conn,  kt_number,  KBF_REQID, NULL, NULL, },
  { "nflog",  kv_conn,  kt_number,  KBF_NFLOG_CONN, NULL, NULL, },

  { "aggressive",  kv_conn,  kt_invertbool,  KBF_AGGRMODE, NULL, NULL, },
  /* alias for compatibility - undocumented on purpose */
  { "aggrmode",  kv_conn | kv_alias,  kt_invertbool,  KBF_AGGRMODE, NULL, NULL, },

  { NULL,  0,  0,  0, NULL, NULL, }
};

/* distinguished keyword */
static const struct keyword_def ipsec_conf_keyword_comment =
	{ "x-comment",      kv_conn,   kt_comment, 0, NULL, NULL, };


/*
 * look for one of the above tokens, and set the value up right.
 *
 * if we don't find it, then strdup() the string and return a string
 *
 */

/* type is really "token" type, which is actually int */
int parser_find_keyword(const char *s, YYSTYPE *lval)
{
	const struct keyword_def *k;
	bool keyleft;
	int keywordtype;

	keyleft = FALSE;
	k = ipsec_conf_keywords;

	while (k->keyname != NULL) {
		if (strcaseeq(s, k->keyname))
			break;

		if (k->validity & kv_leftright) {
			if (strncaseeq(s, "left", 4) &&
			    strcaseeq(s + 4, k->keyname)) {
				keyleft = TRUE;
				break;
			} else if (strncaseeq(s, "right", 5) &&
				   strcaseeq(s + 5, k->keyname)) {
				keyleft = FALSE;
				break;
			}
		}

		k++;
	}

	lval->s = NULL;
	/* if we found nothing */
	if (k->keyname == NULL &&
	    (s[0] == 'x' || s[0] == 'X') && (s[1] == '-' || s[1] == '_')) {
		k = &ipsec_conf_keyword_comment;
		lval->k.string = strdup(s);
	}

	/* if we still found nothing */
	if (k->keyname == NULL) {
		lval->s = strdup(s);
		return STRING;
	}

	switch (k->type) {
	case kt_percent:
		keywordtype = PERCENTWORD;
		break;
	case kt_time:
		keywordtype = TIMEWORD;
		break;
	case kt_comment:
		keywordtype = COMMENT;
		break;
	case kt_bool:
	case kt_invertbool:
		keywordtype = BOOLWORD;
		break;
	default:
		keywordtype = KEYWORD;
		break;
	}

	/* else, set up llval.k to point, and return KEYWORD */
	lval->k.keydef = k;
	lval->k.keyleft = keyleft;
	return keywordtype;
}

unsigned int parser_enum_list(const struct keyword_def *kd, const char *s, bool list)
{
	char complaintbuf[80];

	assert(kd->type == kt_list || kd->type == kt_enum);

	/* strsep(3) is destructive, so give it a safe-to-munge copy of s */
	char *scopy = strdup(s);
	char *scursor = scopy;

	unsigned int valresult = 0;

	/*
	 * split up the string into comma separated pieces, and look each piece up in the
	 * value list provided in the definition.
	 */

	int numfound = 0;
	char *piece;
	while ((piece = strsep(&scursor, ":, \t")) != NULL) {
		/* discard empty strings */
		if (piece[0] == '\0')
			continue;

		assert(kd->validenum != NULL);
		int kevcount = kd->validenum->valuesize;
		const struct keyword_enum_value *kev = kd->validenum->values;
		for ( ; ; kev++, kevcount--) {
			if (kevcount == 0) {
				/* we didn't find anything, complain */
				snprintf(complaintbuf, sizeof(complaintbuf),
					 "%s: %d: keyword %s, invalid value: %s",
					 parser_cur_filename(),
					 parser_cur_lineno(),
					 kd->keyname, piece);

				fprintf(stderr, "ERROR: %s\n", complaintbuf);
				free(scopy);
				exit(1);
			}
			if (strcaseeq(piece, kev->name)) {
				/* found it: count it */
				numfound++;
				valresult |= kev->value;
				break;
			}
		}
	}

	if (numfound > 1 && !list) {
		snprintf(complaintbuf, sizeof(complaintbuf),
			 "%s: %d: keyword %s accepts only one value, not %s",
			 parser_cur_filename(), parser_cur_lineno(),
			 kd->keyname, scopy);

		fprintf(stderr, "ERROR: %s\n", complaintbuf);
		free(scopy);
		exit(1);
	}

	free(scopy);
	return valresult;
}

lset_t parser_lset(const struct keyword_def *kd, const char *value)
{
	assert(kd->type == kt_lset);

	lmod_t result;
	zero(&result);

	/*
	 * Use lmod_args() since it both knows how to parse a comma
	 * separated list and can handle no-XXX (ex: all,no-xauth).
	 * The final set of enabled bits is returned in .set.
	 */
	if (!lmod_arg(&result, kd->info, value)) {
		/*
		 * If the lookup failed, complain (and exit!).
		 *
		 * XXX: the error diagnostic is a little vague -
		 * should lmod_arg() instead return the error?
		 */
		fprintf(stderr, "ERROR: %s: %d: keyword %s, invalid value: %s\n",
			parser_cur_filename(), parser_cur_lineno(),
			kd->keyname, value);
		exit(1);
	}

	return result.set;
}

unsigned int parser_loose_enum(struct keyword *k, const char *s)
{
	const struct keyword_def *kd = k->keydef;
	int kevcount;
	const struct keyword_enum_value *kev;
	unsigned int valresult;

	assert(kd->type == kt_loose_enum || kd->type == kt_rsakey);
	assert(kd->validenum != NULL && kd->validenum->values != NULL);

	for (kevcount = kd->validenum->valuesize, kev = kd->validenum->values;
	     kevcount > 0 && !strcaseeq(s, kev->name);
	     kev++, kevcount--)
		;

	/* if we found something */
	if (kevcount != 0) {
		assert(kev->value != 0);
		valresult = kev->value;
		k->string = NULL;
		return valresult;
	}

	k->string = strdup(s);	/* ??? why not xstrdup? */
	return 255;
}
