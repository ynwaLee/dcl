/* Generated by the protocol buffer compiler.  DO NOT EDIT! */

#ifndef PROTOBUF_C_lis_2dpr_2eproto__INCLUDED
#define PROTOBUF_C_lis_2dpr_2eproto__INCLUDED

#include <google/protobuf-c/protobuf-c.h>

PROTOBUF_C_BEGIN_DECLS


typedef struct _LTERELATE__RELATEDATA LTERELATE__RELATEDATA;


/* --- enums --- */

typedef enum _LTERELATE__RELATETYPE {
  LTE__RELATE__RELATE__TYPE__UPDATE_USER_INFO = 1
} LTERELATE__RELATETYPE;
typedef enum _LTERELATE__INDEXTYPE {
  LTE__RELATE__INDEX__TYPE__BY_UNKNOWN = -1,
  LTE__RELATE__INDEX__TYPE__BY_IMSI = 3
} LTERELATE__INDEXTYPE;
typedef enum _LTERELATE__DATASOURCE {
  LTE__RELATE__DATA__SOURCE__FROM_CS = 6
} LTERELATE__DATASOURCE;
typedef enum _LTERELATE__RETURNTYPE {
  LTE__RELATE__RETURN__TYPE__DATA_ERROR = 0,
  LTE__RELATE__RETURN__TYPE__UPDATE_FAIL = 1,
  LTE__RELATE__RETURN__TYPE__UPDATE_SUCCESS = 2
} LTERELATE__RETURNTYPE;

/* --- messages --- */

struct  _LTERELATE__RELATEDATA
{
  ProtobufCMessage base;
  uint32_t cmd;
  LTERELATE__INDEXTYPE indextype;
  char *imsi;
  char *imei;
  char *msisdn;
  protobuf_c_boolean has_ecgi;
  ProtobufCBinaryData ecgi;
  protobuf_c_boolean has_datasource;
  LTERELATE__DATASOURCE datasource;
  protobuf_c_boolean has_time;
  uint32_t time;
  protobuf_c_boolean has_ret_value;
  LTERELATE__RETURNTYPE ret_value;
};
#define LTE__RELATE__RELATE__DATA__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&lte__relate__relate__data__descriptor) \
    , 0, LTE__RELATE__INDEX__TYPE__BY_UNKNOWN, NULL, NULL, NULL, 0,{0,NULL}, 0,0, 0,0, 0,0 }


/* LTERELATE__RELATEDATA methods */
void   lte__relate__relate__data__init
                     (LTERELATE__RELATEDATA         *message);
size_t lte__relate__relate__data__get_packed_size
                     (const LTERELATE__RELATEDATA   *message);
size_t lte__relate__relate__data__pack
                     (const LTERELATE__RELATEDATA   *message,
                      uint8_t             *out);
size_t lte__relate__relate__data__pack_to_buffer
                     (const LTERELATE__RELATEDATA   *message,
                      ProtobufCBuffer     *buffer);
LTERELATE__RELATEDATA *
       lte__relate__relate__data__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   lte__relate__relate__data__free_unpacked
                     (LTERELATE__RELATEDATA *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*LTERELATE__RELATEDATA_Closure)
                 (const LTERELATE__RELATEDATA *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCEnumDescriptor    lte__relate__relate__type__descriptor;
extern const ProtobufCEnumDescriptor    lte__relate__index__type__descriptor;
extern const ProtobufCEnumDescriptor    lte__relate__data__source__descriptor;
extern const ProtobufCEnumDescriptor    lte__relate__return__type__descriptor;
extern const ProtobufCMessageDescriptor lte__relate__relate__data__descriptor;

PROTOBUF_C_END_DECLS


#endif  /* PROTOBUF_lis_2dpr_2eproto__INCLUDED */
