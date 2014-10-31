/*
 * This file is part of SmartAGENTX
 * Copyright (C) 2014, Credo Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef _AGENTX_H_
#define _AGENTX_H_

#include "asn1.h"
#include "list.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#ifdef SYSLOG
  #ifdef LOGOFF
    #define CREDO_AGENTX_LOG(level, fmt...)
  #else
    #define CREDO_AGENTX_LOG(level, fmt...) \
      do { \
        switch (level) { \
          case AGENTX_LOG_DEBUG: \
          case AGENTX_LOG_INFO: \
          case AGENTX_LOG_WARNING: error(fmt); break;\
          case AGENTX_LOG_ERROR: die(fmt);     break; \
          default:                             break; \
        } \
      } while (0)
  #endif
#else
  #ifdef LOGOFF
    #define CREDO_AGENTX_LOG(level, fmt...)
  #else
    #define CREDO_AGENTX_LOG(level, fmt...) \
      do { \
        switch (level) { \
          case AGENTX_LOG_DEBUG: \
          case AGENTX_LOG_INFO: \
          case AGENTX_LOG_WARNING: fprintf(stdout, fmt); break; \
          case AGENTX_LOG_ERROR: fprintf(stderr, fmt);   break; \
          default:                                       break; \
        } \
      } while (0)
  #endif
#endif

/* AgentX PDU flags */
#define INSTANCE_REGISTRATION  0x1
#define NEW_INDEX              0x2
#define ANY_INDEX              0x4
#define NON_DEFAULT_CONTEXT    0x8
#define NETWORD_BYTE_ORDER     0x10

/* vocal information */
typedef enum agentx_log_level {
  AGENTX_LOG_ALL,
  AGENTX_LOG_DEBUG = 1,
  AGENTX_LOG_INFO,
  AGENTX_LOG_WARNING,
  AGENTX_LOG_ERROR,
  AGENTX_LOG_OFF,

  AGENTX_LOG_LEVEL_NUM
} AGENTX_LOG_LEVEL_E;

/* AgentX PDU tags */
typedef enum agentx_pdu_type {
  AGENTX_PDU_OPEN = 1,
  AGENTX_PDU_CLOSE,
  AGENTX_PDU_REG,
  AGENTX_PDU_UNREG,
  AGENTX_PDU_GET,
  AGENTX_PDU_GETNEXT,
  AGENTX_PDU_GETBULK,
  AGENTX_PDU_TESTSET,
  AGENTX_PDU_COMMITSET,
  AGENTX_PDU_UNDOSET,
  AGENTX_PDU_CLEANUPSET,
  AGENTX_PDU_NOTIFY,
  AGENTX_PDU_PING,
  AGENTX_PDU_INDEXALLOC,
  AGENTX_PDU_INDEXDEALLOC,
  AGENTX_PDU_ADDAGENTCAP,
  AGENTX_PDU_REMOVEAGENTCAP,
  AGENTX_PDU_RESPONSE,
} AGENTX_PDU_TYPE_E;

/* return code */
typedef enum agentx_err_code {
  AGENTX_ERR_OK                 = 0,

  AGENTX_ERR_PDU_CTX_LEN        = -101,
  AGENTX_ERR_PDU_ERRSTAT        = -102,
  AGENTX_ERR_PDU_ERRIDX         = -103,

  AGENTX_ERR_STAT_AUTHORIZATION = -200,

  AGENTX_ERR_VB_VAR             = -201,
  AGENTX_ERR_VB_VALUE_LEN       = -202,
  AGENTX_ERR_VB_OID_LEN         = -203,

  AGENTX_ERR_SR_VAR             = -301,
  AGENTX_ERR_SR_VALUE_LEN       = -302,
  AGENTX_ERR_SR_OID_LEN         = -303,
} AGENTX_ERR_CODE_E;

/* Close PDU reason */
typedef enum agentx_close_reason {
  R_OTHER = 1,
  R_PARSE_ERROR,
  R_PROTOCOL_ERROR,
  R_TIMEOUTS,
  R_SHUTDOWN,
  R_MANAGE,
} AGENTX_CLOSE_REASON_E;

/* Error status in response */
typedef enum agentx_err_response {
  E_OPEN_FAILED = 0x100,
  E_NOT_OPEN,
  E_INDEX_WRONG_TYPE,
  E_INDEX_ALREADY_ALLOCATED,
  E_INDEX_NONE_AVAILABLE,
  E_INDEX_NOT_ALLOCATED,
  E_UNSUPPORTED_CONTEXT,
  E_DUPLICATE_REGISTRATION,
  E_UNKNOWN_REGISTRATION,
  E_UNKNOWN_AGENT_CAPS,
} AGENTX_ERR_RESPONSE_E;

struct x_pdu_hdr {
  uint8_t version;
  uint8_t type;
  uint8_t flags;
  uint8_t reserved;
  uint32_t session_id;
  uint32_t transaction_id;
  uint32_t packet_id;
  uint32_t payload_length;
};

struct x_objid_t {
  uint8_t n_subid;
  uint8_t prefix;
  uint8_t include;
  uint8_t reserved;
  uint32_t sub_id[0];
};

struct x_octstr_t {
  uint32_t len;
  uint8_t str[0];
};

struct x_pdu_buf {
  uint8_t *buf;
  uint32_t len;
};

struct x_search_range {
  struct list_head link;
  oid_t *start;
  oid_t *end;
  uint32_t start_len;
  uint32_t end_len;
  uint8_t start_include;
  uint8_t end_include;
};

struct x_var_bind {
  struct list_head link;
  oid_t *oid;
  uint32_t oid_len;
  uint16_t val_type;
  uint32_t val_len;
  uint8_t value[0];
};

struct agentx_datagram {
  lua_State *lua_state;
  
  int sock;
  
  void *recv_buf;
  void *send_buf;

  /* context (community) */
  octstr_t context[41];
  uint32_t ctx_len;
  
  struct x_pdu_hdr pdu_hdr;

  union {
    struct {
      uint8_t reason;
    } close;
    struct {
      uint16_t non_rep;
      uint16_t max_rep;
    } getbulk;
    struct {
      uint32_t sys_up_time;
      uint16_t error;
      uint16_t index;
    } response;
  } u;

  uint32_t vb_in_cnt;
  uint32_t vb_out_cnt;
  struct list_head vb_in_list;
  struct list_head vb_out_list;

  uint32_t sr_in_cnt;
  uint32_t sr_out_cnt;
  struct list_head sr_in_list;
  struct list_head sr_out_list;
};

extern struct agentx_datagram agentx_datagram;

uint32_t agentx_value_dec(uint8_t **buffer, uint8_t flag, uint8_t type, void *value);
uint32_t agentx_value_dec_test(uint8_t *buf, uint8_t flag, uint8_t type);
uint32_t agentx_value_enc(const void *value, uint32_t len, uint8_t type, uint8_t *buf);
uint32_t agentx_value_enc_test(const void *value, uint32_t len, uint8_t type);

void agentx_send_response(struct agentx_datagram *xdg);
int agentx_recv(uint8_t *buffer, int len, void *arg);
void agentx_send(uint8_t *buf, int len);

struct x_pdu_buf agentx_open_pdu(struct agentx_datagram *xdg, oid_t *oid, uint32_t oid_len, const char *descr, uint32_t descr_len);
struct x_pdu_buf agentx_close_pdu(struct agentx_datagram *xdg, uint32_t reason);
struct x_pdu_buf agentx_register_pdu(struct agentx_datagram *xdg, oid_t *oid, uint32_t oid_len, const char *community, uint32_t comm_len,
                                     uint8_t timeout, uint8_t priority, uint8_t range_subid, uint32_t upper_bound);
struct x_pdu_buf agentx_unregister_pdu(struct agentx_datagram *xdg, oid_t *oid, uint32_t oid_len, const char *community, uint32_t comm_len,
                                       uint8_t timeout, uint8_t priority, uint8_t range_subid, uint32_t upper_bound);
struct x_pdu_buf agentx_ping_pdu(struct agentx_datagram *xdg, const char *context, uint32_t context_len);
struct x_pdu_buf agentx_response_pdu(struct agentx_datagram *xdg);
#endif /* _AGENTX_H_ */
