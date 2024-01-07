/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: lar-defs.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "lar-defs.pb-c.h"
void   request_message__init
                     (RequestMessage         *message)
{
  static const RequestMessage init_value = REQUEST_MESSAGE__INIT;
  *message = init_value;
}
size_t request_message__get_packed_size
                     (const RequestMessage *message)
{
  assert(message->base.descriptor == &request_message__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t request_message__pack
                     (const RequestMessage *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &request_message__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t request_message__pack_to_buffer
                     (const RequestMessage *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &request_message__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
RequestMessage *
       request_message__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (RequestMessage *)
     protobuf_c_message_unpack (&request_message__descriptor,
                                allocator, len, data);
}
void   request_message__free_unpacked
                     (RequestMessage *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &request_message__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   reply_message__init
                     (ReplyMessage         *message)
{
  static const ReplyMessage init_value = REPLY_MESSAGE__INIT;
  *message = init_value;
}
size_t reply_message__get_packed_size
                     (const ReplyMessage *message)
{
  assert(message->base.descriptor == &reply_message__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t reply_message__pack
                     (const ReplyMessage *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &reply_message__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t reply_message__pack_to_buffer
                     (const ReplyMessage *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &reply_message__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
ReplyMessage *
       reply_message__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (ReplyMessage *)
     protobuf_c_message_unpack (&reply_message__descriptor,
                                allocator, len, data);
}
void   reply_message__free_unpacked
                     (ReplyMessage *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &reply_message__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   display_update_message__init
                     (DisplayUpdateMessage         *message)
{
  static const DisplayUpdateMessage init_value = DISPLAY_UPDATE_MESSAGE__INIT;
  *message = init_value;
}
size_t display_update_message__get_packed_size
                     (const DisplayUpdateMessage *message)
{
  assert(message->base.descriptor == &display_update_message__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t display_update_message__pack
                     (const DisplayUpdateMessage *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &display_update_message__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t display_update_message__pack_to_buffer
                     (const DisplayUpdateMessage *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &display_update_message__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
DisplayUpdateMessage *
       display_update_message__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (DisplayUpdateMessage *)
     protobuf_c_message_unpack (&display_update_message__descriptor,
                                allocator, len, data);
}
void   display_update_message__free_unpacked
                     (DisplayUpdateMessage *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &display_update_message__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor request_message__field_descriptors[3] =
{
  {
    "id",
    1,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(RequestMessage, id),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "password",
    2,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(RequestMessage, has_password),
    offsetof(RequestMessage, password),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "direction",
    3,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(RequestMessage, has_direction),
    offsetof(RequestMessage, direction),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned request_message__field_indices_by_name[] = {
  2,   /* field[2] = direction */
  0,   /* field[0] = id */
  1,   /* field[1] = password */
};
static const ProtobufCIntRange request_message__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 3 }
};
const ProtobufCMessageDescriptor request_message__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "request_message",
  "RequestMessage",
  "RequestMessage",
  "",
  sizeof(RequestMessage),
  3,
  request_message__field_descriptors,
  request_message__field_indices_by_name,
  1,  request_message__number_ranges,
  (ProtobufCMessageInit) request_message__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor reply_message__field_descriptors[4] =
{
  {
    "success",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT32,
    0,   /* quantifier_offset */
    offsetof(ReplyMessage, success),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "id",
    2,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(ReplyMessage, id),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "password",
    3,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(ReplyMessage, has_password),
    offsetof(ReplyMessage, password),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "score",
    4,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(ReplyMessage, has_score),
    offsetof(ReplyMessage, score),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned reply_message__field_indices_by_name[] = {
  1,   /* field[1] = id */
  2,   /* field[2] = password */
  3,   /* field[3] = score */
  0,   /* field[0] = success */
};
static const ProtobufCIntRange reply_message__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 4 }
};
const ProtobufCMessageDescriptor reply_message__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "reply_message",
  "ReplyMessage",
  "ReplyMessage",
  "",
  sizeof(ReplyMessage),
  4,
  reply_message__field_descriptors,
  reply_message__field_indices_by_name,
  1,  reply_message__number_ranges,
  (ProtobufCMessageInit) reply_message__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor display_update_message__field_descriptors[4] =
{
  {
    "ch",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(DisplayUpdateMessage, ch),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "pos_x",
    2,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT32,
    0,   /* quantifier_offset */
    offsetof(DisplayUpdateMessage, pos_x),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "pos_y",
    3,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT32,
    0,   /* quantifier_offset */
    offsetof(DisplayUpdateMessage, pos_y),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "score",
    4,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(DisplayUpdateMessage, has_score),
    offsetof(DisplayUpdateMessage, score),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned display_update_message__field_indices_by_name[] = {
  0,   /* field[0] = ch */
  1,   /* field[1] = pos_x */
  2,   /* field[2] = pos_y */
  3,   /* field[3] = score */
};
static const ProtobufCIntRange display_update_message__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 4 }
};
const ProtobufCMessageDescriptor display_update_message__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "display_update_message",
  "DisplayUpdateMessage",
  "DisplayUpdateMessage",
  "",
  sizeof(DisplayUpdateMessage),
  4,
  display_update_message__field_descriptors,
  display_update_message__field_indices_by_name,
  1,  display_update_message__number_ranges,
  (ProtobufCMessageInit) display_update_message__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCEnumValue direction__enum_values_by_number[4] =
{
  { "UP", "DIRECTION__UP", 0 },
  { "DOWN", "DIRECTION__DOWN", 1 },
  { "LEFT", "DIRECTION__LEFT", 2 },
  { "RIGHT", "DIRECTION__RIGHT", 3 },
};
static const ProtobufCIntRange direction__value_ranges[] = {
{0, 0},{0, 4}
};
static const ProtobufCEnumValueIndex direction__enum_values_by_name[4] =
{
  { "DOWN", 1 },
  { "LEFT", 2 },
  { "RIGHT", 3 },
  { "UP", 0 },
};
const ProtobufCEnumDescriptor direction__descriptor =
{
  PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC,
  "Direction",
  "Direction",
  "Direction",
  "",
  4,
  direction__enum_values_by_number,
  4,
  direction__enum_values_by_name,
  1,
  direction__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};
static const ProtobufCEnumValue type__enum_values_by_number[7] =
{
  { "LIZARD_CONNECT", "TYPE__LIZARD_CONNECT", 0 },
  { "BOT_CONNECT", "TYPE__BOT_CONNECT", 1 },
  { "LIZARD_MOVE", "TYPE__LIZARD_MOVE", 2 },
  { "BOT_MOVE", "TYPE__BOT_MOVE", 3 },
  { "LIZARD_DISCONNECT", "TYPE__LIZARD_DISCONNECT", 4 },
  { "BOT_DISCONNECT", "TYPE__BOT_DISCONNECT", 5 },
  { "DISPLAY_UPDATE", "TYPE__DISPLAY_UPDATE", 6 },
};
static const ProtobufCIntRange type__value_ranges[] = {
{0, 0},{0, 7}
};
static const ProtobufCEnumValueIndex type__enum_values_by_name[7] =
{
  { "BOT_CONNECT", 1 },
  { "BOT_DISCONNECT", 5 },
  { "BOT_MOVE", 3 },
  { "DISPLAY_UPDATE", 6 },
  { "LIZARD_CONNECT", 0 },
  { "LIZARD_DISCONNECT", 4 },
  { "LIZARD_MOVE", 2 },
};
const ProtobufCEnumDescriptor type__descriptor =
{
  PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC,
  "Type",
  "Type",
  "Type",
  "",
  7,
  type__enum_values_by_number,
  7,
  type__enum_values_by_name,
  1,
  type__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};
