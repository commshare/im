/* LibMemcached
 * Copyright (C) 2006-2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary:
 *
 */

#include <libmemcached/common.h>

memcached_return_t memcached_vdo(memcached_server_write_instance_st instance,
                                 libmemcached_io_vector_st vector[],
                                 const size_t count,
                                 const bool with_flush)
{
  memcached_return_t rc;

  assert_msg(vector, "Invalid vector passed");

  if (memcached_failed(rc= memcached_connect(instance)))
  {
    WATCHPOINT_ERROR(rc);
    assert_msg(instance->error_messages, "memcached_connect() returned an error but the memcached_server_write_instance_st showed none.");
    return rc;
  }

  /*
  ** Since non buffering ops in UDP mode dont check to make sure they will fit
  ** before they start writing, if there is any data in buffer, clear it out,
  ** otherwise we might get a partial write.
  **/
  if (memcached_is_udp(instance->root))
  {
    if (vector[0].buffer or vector[0].length)
    {
      return memcached_set_error(*instance->root, MEMCACHED_NOT_SUPPORTED, MEMCACHED_AT, 
                                 memcached_literal_param("UDP messages was attempted, but vector was not setup for it"));
    }

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));

    increment_udp_message_id(instance);
    vector[0].buffer= instance->write_buffer;
    vector[0].length= UDP_DATAGRAM_HEADER_LENGTH;

    msg.msg_iov= (struct iovec*)vector;
    msg.msg_iovlen= count;

    uint32_t retry= 5;
    while (--retry)
    {
      ssize_t sendmsg_length= ::sendmsg(instance->fd, &msg, 0);
      if (sendmsg_length > 0)
      {
        break;
      }
      else if (sendmsg_length < 0)
      {
        if (errno == EMSGSIZE)
        {
          return memcached_set_error(*instance, MEMCACHED_WRITE_FAILURE, MEMCACHED_AT);
        }

        return memcached_set_errno(*instance, errno, MEMCACHED_AT);
      }
    }

    return MEMCACHED_SUCCESS;
  }

  bool sent_success= memcached_io_writev(instance, vector, count, with_flush);
  if (sent_success == false)
  {
    if (memcached_last_error(instance->root) == MEMCACHED_SUCCESS)
    {
      return memcached_set_error(*instance, MEMCACHED_WRITE_FAILURE, MEMCACHED_AT);
    }
    else
    {
      rc= MEMCACHED_WRITE_FAILURE;
    }
  }
  else if (memcached_is_replying(instance->root))
  {
    memcached_server_response_increment(instance);
  }

  return rc;
}
