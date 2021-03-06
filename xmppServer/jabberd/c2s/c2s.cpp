/* vim: set et ts=4 sw=4: */
/*
 * jabberd - Jabber Open Source Server
 * Copyright (c) 2002 Jeremie Miller, Thomas Muldowney,
 *                    Ryan Eatmon, Robert Norris
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA02111-1307USA
 */

#include "c2s.h"
#include <stringprep.h>

#include "ClientProtocol.h"
#include "SyncCritical.h"
#include "GlobalBusi.h"

#include "protocol.h"
#include "xtpacket.h"

#include <time.h>

#include "Function.h"
#include "improtocol.pb.h"
#include "Function.h"



extern IDNAPI const Stringprep_profile stringprep_nameprep[];
#undef stringprep_xmpp_resourceprep


#define stringprep_xmpp_resourceprep(in, maxlen)               \
      stringprep(in, maxlen, (Stringprep_profile_flags)0, stringprep_xmpp_resourceprep)

#undef stringprep_xmpp_nodeprep
#define stringprep_xmpp_nodeprep(in, maxlen)           \
      stringprep(in, maxlen, (Stringprep_profile_flags)0, stringprep_xmpp_nodeprep)



typedef void (*pfnjabberdCallback)(void * pUserKey,int nEvent,const RecvDataStruct * pRecvData);
typedef void (*generatepkt)(uint32 fd,char *username,char *pwd,uint32 src_ip, uint16 src_port,uint16 cmd,
                            uint32 fromid,uint32 toid,RecvDataStruct *RecvData,void *input);

map<uint32,sess_t> sessManager;
map<uint32, nad_t> loginNodManager;



// 解析imserver线程发过来的线程流
unsigned int ParseStream(char *pData, sess_t sess, int len);
// 登录请求xml格式输出
unsigned int SendXmlLogin(XT_LOGIN_ACK* login_ack, sess_t sess);
// 聊天消息转发
unsigned int SendXmlMessage(char* data, sess_t sess);
// 离线消息转发
unsigned int SendXmlOfflineMessage(char* data, sess_t sess);
// 获取完整信息
unsigned int SendXmlFullUserInfo(char* data);
// 获取分流结果
unsigned int SendXmlFlow(char* data);
// 获取最近联系商家
unsigned int SendXmlWebList(char* data);
// 心跳
unsigned int SendXmlHeartBeat(char* data);
//超时
unsigned int SendXmlTimeout(char* data);
// 发送历史消息给移动端
int SendMsgHistory(uint32 did, MsgList &ml);


void _authreg_sasl_auth_set(c2s_t c2s, sess_t sess, nad_t nad)
{
    int ns, elem, attr, authd = 0;
    char username[1024], resource[1024], str[1024], hash[280];
    int ar_mechs;
    log_debug(ZONE, "Into there");


    /* can't auth if they're active */
    if(sess->active)
    {
        sx_nad_write(sess->s, stanza_tofrom(stanza_error(nad, 0, stanza_err_NOT_ALLOWED), 0));
        return;
    }

    log_debug(ZONE, "Into there222");

#if 0



    char from[1024];
//   sess_t sess;
    union xhashv xhv;

    if((attr = nad_find_attr(nad, 0, -1, "from", NULL)) < 0)
    {
        nad_free(nad);
        return;
    }
    strncpy(from, NAD_AVAL(nad, attr), NAD_AVAL_L(nad, attr));
    from[NAD_AVAL_L(nad, attr)] = '\0';

    log_debug(ZONE, "@#####plaintext auth (check) succeded username %s pwd %s  ip %s port %d ",
              username,str,sess->ip,sess->port);
    RecvDataStruct packets;
//(uint32 fd,char *username,char *pwd,uint32 unsigned src_ip, uint32 src_port,uint32 cmd,RecvDataStruct *RecvData)
    log_debug(ZONE, "generatepkt   fd:%d", c2s->server_fd->fd);
    uint32 ipaddr = inet_addr(sess->ip);
    ((generatepkt)c2s->m_generatepkt)((uint32)sess->fd->fd,username,str,ipaddr,sess->port,(uint32)0x211,0,0,&packets,NULL);

    ((pfnjabberdCallback) c2s->m_pfnUserCallback)(c2s->m_pUserKey,NE_RECVDATA,&packets);
    log_write(c2s->log, LOG_ERR, "we are here ! hahahaha   yayaya\r\n");
#endif
//<iq xmlns='jabber:client' to='imserver.vm04.fn.com' id='sd12' type='result'><userid>5003133</userid></iq>


    /*
        <iq xmlns='jabber:client' to='imserver.vm04.fn.com' id='sd12' type='result'>
        <query xmlns='jabber:iq:auth'>
        <resource>telnet</resource>
        <username>sunding3</username>
        <userid>5004086</userid>
        </query>
        </iq>
    */
    if(sess->m_selfID == 0)
    {
        sx_nad_write(sess->s, stanza_tofrom(stanza_error(nad, 0, stanza_err_NOT_ALLOWED), 0));
        log_debug(ZONE, "error in authreg sasl m_selfID = 0");
        return;
    }


    if((attr = nad_find_attr(nad, 0, -1, "id", NULL)) < 0)
    {
        nad_free(nad);
        return;
    }
    char identy[32] = {0};
    strncpy(identy, NAD_AVAL(nad, attr), NAD_AVAL_L(nad, attr));
    identy[NAD_AVAL_L(nad, attr)] = '\0';

    sess->result = nad_new();

    ns = nad_add_namespace(sess->result, uri_CLIENT, NULL);

    nad_append_elem(sess->result, ns, "iq", 0);
    nad_set_attr(sess->result, 0, -1, "type", "result", 6);
    char szID[64] = {0};
    sprintf_s(szID, "%d", sess->m_selfID);
    nad_set_attr(sess->result, 0, -1, "to", szID, strlen(szID));
    nad_set_attr(sess->result, 0, -1, "id",identy,strlen(identy));



    map<uint32,sess_t>::iterator sessold = sessManager.find(sess->m_selfID);
    if(sessold != sessManager.end())
    {
        log_debug(ZONE, "map erase old fd");
        mio_close(sessold->second->c2s->mio, sessold->second->fd);
        sessold->second->fd = NULL;
        sessManager.erase(sess->m_selfID);
    }
    sessManager.insert(pair<uint32,sess_t>(sess->m_selfID,sess));
    log_debug(ZONE, "add map userid %u --- > fd %u",sess->m_selfID,sess->fd->fd);

    // 添加query字段
    nad_append_elem(sess->result, ns, "query", 1);
    nad_append_attr(sess->result, -1, "xmlns", uri_AUTH);


    nad_append_elem(sess->result, ns, "userid", 2);
    nad_append_cdata_gbk(sess->result,szID, strlen(szID), 3);

	nad_append_elem(sess->result, ns, "username", 2);
    nad_append_cdata_gbk(sess->result,sess->m_username, strlen(sess->m_username), 3);
	
	time_t tm;
	time(&tm);
	char timestamp[32] = {0};
	sprintf(timestamp,"%u",(uint32)tm);
	nad_append_elem(sess->result, ns, "timestamp", 2);
    nad_append_cdata_gbk(sess->result,timestamp, strlen(timestamp), 3);
    /* start a session with the sm */
    //sm_start(sess, sess->resources);
    
    sx_nad_write(sess->s, sess->result);
    sess->result = NULL;

    return;
}


void _authreg_auth_set(c2s_t c2s, sess_t sess, nad_t nad)
{
    int ns, elem, attr, authd = 0;
    char username[1024], resource[1024], str[1024], hash[280];
    int ar_mechs;
    log_debug(ZONE, "Into hrerererae");

    /* can't auth if they're active */
    if(sess->active)
    {
        sx_nad_write(sess->s, stanza_tofrom(stanza_error(nad, 0, stanza_err_NOT_ALLOWED), 0));
        return;
    }

    log_debug(ZONE, "Into hrerererae22");
    ns = nad_find_scoped_namespace(nad, uri_AUTH, NULL);

    /* sort out the username */
    elem = nad_find_elem(nad, 1, ns, "username", 1);
    if(elem < 0)
    {
        log_debug(ZONE, "auth set with no username, bouncing it");

        sx_nad_write(sess->s, stanza_tofrom(stanza_error(nad, 0, stanza_err_BAD_REQUEST), 0));

        return;
    }

    snprintf(username, 1024, "%.*s", NAD_CDATA_L(nad, elem), NAD_CDATA(nad, elem));
    /*if(stringprep_xmpp_nodeprep(username, 1024) != 0)
    {
        log_debug(ZONE, "auth set username failed nodeprep, bouncing it");
        sx_nad_write(sess->s, stanza_tofrom(stanza_error(nad, 0, stanza_err_JID_MALFORMED), 0));
        return;
    }*/

    /* make sure we have the resource */
    elem = nad_find_elem(nad, 1, ns, "resource", 1);
    if(elem < 0)
    {
        log_debug(ZONE, "auth set with no resource, bouncing it");

        sx_nad_write(sess->s, stanza_tofrom(stanza_error(nad, 0, stanza_err_BAD_REQUEST), 0));

        return;
    }

    snprintf(resource, 1024, "%.*s", NAD_CDATA_L(nad, elem), NAD_CDATA(nad, elem));
    if(stringprep_xmpp_resourceprep(resource, 1024) != 0)
    {
        log_debug(ZONE, "auth set resource failed resourceprep, bouncing it");
        sx_nad_write(sess->s, stanza_tofrom(stanza_error(nad, 0, stanza_err_JID_MALFORMED), 0));
        return;
    }

    ar_mechs = c2s->ar_mechanisms;
    if (sess->s->ssf > 0)
        ar_mechs = ar_mechs | c2s->ar_ssl_mechanisms;

    /* no point going on if we have no mechanisms */
    if(!(ar_mechs & (AR_MECH_TRAD_PLAIN | AR_MECH_TRAD_DIGEST | AR_MECH_TRAD_CRAMMD5)))
    {
        sx_nad_write(sess->s, stanza_tofrom(stanza_error(nad, 0, stanza_err_FORBIDDEN), 0));
        return;
    }

    /* do we have the user? */
#if 0
    if((c2s->ar->user_exists)(c2s->ar, sess, username, sess->host->realm) == 0)
    {
        sx_nad_write(sess->s, stanza_tofrom(stanza_error(nad, 0, stanza_err_OLD_UNAUTH), 0));
        return;
    }

    /* handle CRAM-MD5 response */
    if(!authd && ar_mechs & AR_MECH_TRAD_CRAMMD5 )
    {
        elem = nad_find_elem(nad, 1, ns, "crammd5", 1);
        if(elem >= 0)
        {
            snprintf(str, 1024, "%.*s", NAD_CDATA_L(nad, elem), NAD_CDATA(nad, elem));
            if((c2s->ar->check_response)(c2s->ar, sess, username, sess->host->realm, sess->auth_challenge, str) == 0)
            {
                log_debug(ZONE, "crammd5 auth (check) succeded");
                authd = 1;
                //_authreg_auth_log(c2s, sess, "traditional.cram-md5", username, resource, TRUE);
            }
            else
            {
                //_authreg_auth_log(c2s, sess, "traditional.cram-md5", username, resource, FALSE);
            }
        }
    }

    /* digest auth */
    if(!authd && ar_mechs & AR_MECH_TRAD_DIGEST && c2s->ar->get_password != NULL)
    {
        elem = nad_find_elem(nad, 1, ns, "digest", 1);
        if(elem >= 0)
        {
            if((c2s->ar->get_password)(c2s->ar, sess, username, sess->host->realm, str) == 0)
            {
                snprintf(hash, 280, "%s%s", sess->s->id, str);
                shahash_r(hash, hash);

                if(strlen(hash) == NAD_CDATA_L(nad, elem) && strncmp(hash, NAD_CDATA(nad, elem), NAD_CDATA_L(nad, elem)) == 0)
                {
                    log_debug(ZONE, "digest auth succeeded");
                    authd = 1;
                    //_authreg_auth_log(c2s, sess, "traditional.digest", username, resource, TRUE);
                }
                else
                {
                    //_authreg_auth_log(c2s, sess, "traditional.digest", username, resource, FALSE);
                }
            }
        }
    }


    /* plaintext auth (compare) */
    if(!authd && ar_mechs & AR_MECH_TRAD_PLAIN && c2s->ar->get_password != NULL)
    {
        elem = nad_find_elem(nad, 1, ns, "password", 1);

        if(elem >= 0)
        {
            if((c2s->ar->get_password)(c2s->ar, sess, username, sess->host->realm, str) == 0 &&
               strlen(str) == NAD_CDATA_L(nad, elem) && strncmp(str, NAD_CDATA(nad, elem), NAD_CDATA_L(nad, elem)) == 0)
            {
                log_debug(ZONE, "plaintext auth (compare) succeeded");
                authd = 1;
                //_authreg_auth_log(c2s, sess, "traditional.plain(compare)", username, resource, TRUE);
            }
            else
            {
                //_authreg_auth_log(c2s, sess, "traditional.plain(compare)", username, resource, FALSE);
            }
        }
    }
#endif
    /* plaintext auth (check) */
    if(!authd && ar_mechs & AR_MECH_TRAD_PLAIN )
    {
        elem = nad_find_elem(nad, 1, ns, "password", 1);
        if(elem >= 0)
        {
            snprintf(str, 1024, "%.*s", NAD_CDATA_L(nad, elem), NAD_CDATA(nad, elem));
#if 0
            if((c2s->ar->check_password)(c2s->ar, sess, username, sess->host->realm, str) == 0)
            {
                log_debug(ZONE, "plaintext auth (check) succeded");
                authd = 1;
                //_authreg_auth_log(c2s, sess, "traditional.plain", username, resource, TRUE);
            }
            else
            {
                //_authreg_auth_log(c2s, sess, "traditional.plain", username, resource, FALSE);
            }
#endif

        }
    }

    elem = nad_find_elem(nad, 1, ns, "password", 1);
    if(elem < 0)
    {
        log_debug(ZONE, "auth set with no password, bouncing it");

        sx_nad_write(sess->s, stanza_tofrom(stanza_error(nad, 0, stanza_err_BAD_REQUEST), 0));

        return;
    }
    snprintf(str, 1024, "%.*s", NAD_CDATA_L(nad, elem), NAD_CDATA(nad, elem));

    log_debug(ZONE, "@#####plaintext auth (check) succeded username %s pwd %s  ip %s port %d ",
              username,str,sess->ip,sess->port);
    RecvDataStruct packets;
//(uint32 fd,char *username,char *pwd,uint32 unsigned src_ip, uint32 src_port,uint32 cmd,RecvDataStruct *RecvData)
    log_debug(ZONE, "generatepkt   fd:%d", c2s->server_fd->fd);
    uint32 ipaddr = inet_addr(sess->ip);
    ((generatepkt)c2s->m_generatepkt)((uint32)sess->fd->fd,username,str,ipaddr,sess->port,(uint32)0x211,0,0,&packets,NULL);



#if 0
    packets.src_ip = 111111111;
    packets.port = 7788;

    packets.src_type = 1;
    packets.data_len = 1;
    packets.vlink = 2;
#endif

    ((pfnjabberdCallback) c2s->m_pfnUserCallback)(c2s->m_pUserKey,NE_RECVDATA,&packets);


    log_write(c2s->log, LOG_ERR, "we are here ! hahahaha   yayaya\r\n");

    return;


    /* now, are they authenticated? */
    if(authd)
    {
        /* create new_cplus bound jid holder */
        if(sess->resources == NULL)
        {
            sess->resources = (bres_t) calloc(1, sizeof(struct bres_st));
        }

        /* our local id */
        sprintf(sess->resources->c2s_id, "%d", sess->s->tag);

        /* the full user jid for this session */
        sess->resources->jid = jid_new(sess->s->req_to, -1);
        jid_reset_components(sess->resources->jid, username, sess->resources->jid->domain, resource);

        log_write(sess->c2s->log, LOG_NOTICE, "[%d] requesting session: jid=%s", sess->s->tag, jid_full(sess->resources->jid));

        /* build a result packet, we'll send this back to the client after we have a session for them */
        sess->result = nad_new();

        ns = nad_add_namespace(sess->result, uri_CLIENT, NULL);

        nad_append_elem(sess->result, ns, "iq", 0);
        nad_set_attr(sess->result, 0, -1, "type", "result", 6);

        attr = nad_find_attr(nad, 0, -1, "id", NULL);
        if(attr >= 0)
            nad_set_attr(sess->result, 0, -1, "id", NAD_AVAL(nad, attr), NAD_AVAL_L(nad, attr));

        /* start a session with the sm */
        //sm_start(sess, sess->resources);

        /* finished with the nad */
        //nad_free(nad);

        return;
    }

    //_authreg_auth_log(c2s, sess, "traditional", username, resource, FALSE);

    /* auth failed, so error */
    sx_nad_write(sess->s, stanza_tofrom(stanza_error(nad, 0, stanza_err_OLD_UNAUTH), 0));

    return;
}


static int _c2s_client_sx_callback(sx_t s, sx_event_t e, void *data, void *arg)
{
    sess_t sess = (sess_t) arg;
    sx_buf_t buf = (sx_buf_t) data;
    int rlen, len, ns, elem, attr;
    sx_error_t *sxe;
    nad_t nad;
    char root[9];
    bres_t bres, ires;
    stream_redirect_t redirect;
    int ImmessageFlag = 0;

    switch(e)
    {
        case event_WANT_READ:
            log_debug(ZONE, "want read");
            mio_read(sess->c2s->mio, sess->fd);
            break;

        case event_WANT_WRITE:
            log_debug(ZONE, "want write");
            mio_write(sess->c2s->mio, sess->fd);
            break;

        case event_READ:
            log_debug(ZONE, "reading from %d", sess->fd->fd);

            /* check rate limits */
            if(sess->rate != NULL)
            {
                if(rate_check(sess->rate) == 0)
                {

                    /* inform the app if we haven't already */
                    if(!sess->rate_log)
                    {
                        if(s->state >= state_STREAM && sess->resources != NULL)
                            log_write(sess->c2s->log, LOG_NOTICE, "[%d] [%s] is being byte rate limited", sess->fd->fd, jid_user(sess->resources->jid));
                        else
                            log_write(sess->c2s->log, LOG_NOTICE, "[%d] [%s, port=%d] is being byte rate limited", sess->fd->fd, sess->ip, sess->port);

                        sess->rate_log = 1;
                    }

                    return -1;
                }

                /* find out how much we can have */
                rlen = rate_left(sess->rate);
                if(rlen > buf->len)
                    rlen = buf->len;

                log_debug(ZONE, "sess->rate != NULL");    
            }

            /* no limit, just read as much as we can */
            else
                rlen = buf->len;

            /* do the read */
            log_debug(ZONE, "will read %d bytes", rlen);
            len = recv(sess->fd->fd, buf->data, rlen, 0);

            /* update rate limits */
            if(sess->rate != NULL)
                rate_add(sess->rate, len);

            if(len < 0)
            {
                if(MIO_WOULDBLOCK)
                {
                    buf->len = 0;
                    return 0;
                }

                if(s->state >= state_STREAM && sess->resources != NULL)
                    log_write(sess->c2s->log, LOG_NOTICE, "[%d] [%s] read error: %s (%d)", sess->fd->fd, jid_user(sess->resources->jid), MIO_STRERROR(MIO_ERROR), MIO_ERROR);
                else
                    log_write(sess->c2s->log, LOG_NOTICE, "[%d] [%s, port=%d] read error: %s (%d)", sess->fd->fd, sess->ip, sess->port, MIO_STRERROR(MIO_ERROR), MIO_ERROR);

                sx_kill(s);

                return -1;
            }

            else if(len == 0)
            {
                /* they went away */
                sx_kill(s);

                return -1;
            }

            log_debug(ZONE, "read %d bytes", len);

            if(strncmp("XMPP", buf->data, 4) == 0)
            {
                // 收到imserver线程的数据
                log_write(sess->c2s->log, LOG_NOTICE, "[%d] imserver thread data", sess->fd->fd);

                char skey[128] = {0};
                uint32 fdCl = *((uint32*)(buf->data+4));
                sprintf(skey, "%d", fdCl);

                sess_t sessCl =(sess_t) xhash_get(sess->c2s->sessions, skey);
//              if(sessCl != NULL)
//              {
                ParseStream(buf->data+8, sessCl, len-8);
//              }

                //sx_kill(s);
                //return -1;
            }

            /* If the first chars are "GET " then it's for HTTP (GET ....)
               and if we configured http client forwarding to a real http server */
            if (sess->c2s->http_forward && !sess->active && !sess->sasl_authd
                && sess->result == NULL && len >= 4 && strncmp("GET ", buf->data, 4) == 0)
            {
                char* http =
                    "HTTP/1.0 301 Found\r\n"
                    "Location: %s\r\n"
                    "Server: " PACKAGE_STRING "\r\n"
                    "Expires: Fri, 10 Oct 1997 10:10:10 GMT\r\n"
                    "Pragma: no-cache\r\n"
                    "Cache-control: priavte_cplus\r\n"
                    "Connection: close\r\n\r\n";
                char *answer;

                len = strlen(sess->c2s->http_forward) + strlen(http);
                answer = (char *)malloc(len * sizeof(char));
                sprintf (answer, http, sess->c2s->http_forward);

                log_write(sess->c2s->log, LOG_NOTICE, "[%d] bouncing HTTP request to %s", sess->fd->fd, sess->c2s->http_forward);

                /* send HTTP answer */
                len = send(sess->fd->fd, answer, len-1, 0);

                free(answer);

                /* close connection */
                sx_kill(s);

                return -1;
            }

            buf->len = len;

            return len;

        case event_WRITE:
            log_debug(ZONE, "writing to %d", sess->fd->fd);

            len = send(sess->fd->fd, buf->data, buf->len, 0);
            if(len >= 0)
            {
                log_debug(ZONE, "%d bytes written", len);
                return len;
            }

            if(MIO_WOULDBLOCK)
                return 0;

            if(s->state >= state_OPEN && sess->resources != NULL)
                log_write(sess->c2s->log, LOG_NOTICE, "[%d] [%s] write error: %s (%d)", sess->fd->fd, jid_user(sess->resources->jid), MIO_STRERROR(MIO_ERROR), MIO_ERROR);
            else
                log_write(sess->c2s->log, LOG_NOTICE, "[%d] [%s. port=%d] write error: %s (%d)", sess->fd->fd, sess->ip, sess->port, MIO_STRERROR(MIO_ERROR), MIO_ERROR);

            sx_kill(s);

            return -1;

        case event_ERROR:
            sxe = (sx_error_t *) data;
            if(sess->resources != NULL)
                log_write(sess->c2s->log, LOG_NOTICE, "[%d] [%s] error: %s (%s)", sess->fd->fd, jid_user(sess->resources->jid), sxe->generic, sxe->specific);
            else
                log_write(sess->c2s->log, LOG_NOTICE, "[%d] [%s, port=%d] error: %s (%s)", sess->fd->fd, sess->ip, sess->port, sxe->generic, sxe->specific);

            break;

        case event_STREAM:

            if(s->req_to == NULL)
            {
                log_debug(ZONE, "no stream to provided, closing");
                sx_error(s, stream_err_HOST_UNKNOWN, "no 'to' attribute on stream header");
                sx_close(s);

                return 0;
            }

            /* send a see-other-host error if we're configured to do so */
            redirect = (stream_redirect_t) xhash_get(sess->c2s->stream_redirects, s->req_to);
            if (redirect != NULL)
            {
                log_debug(ZONE, "redirecting client's stream using see-other-host for domain: '%s'", s->req_to);
                len = strlen(redirect->to_address) + strlen(redirect->to_port) + 1;
                char *other_host = (char *) malloc(len+1);
                snprintf(other_host, len+1, "%s:%s", redirect->to_address, redirect->to_port);
                sx_error_extended(s, stream_err_SEE_OTHER_HOST, other_host);
                free(other_host);
                sx_close(s);

                return 0;
            }

            /* setup the host */
            sess->host = (host_t) xhash_get(sess->c2s->hosts, s->req_to);

            if(sess->host == NULL && sess->c2s->vhost == NULL)
            {
                log_debug(ZONE, "no host available for requested domain '%s'", s->req_to);
                sx_error(s, stream_err_HOST_UNKNOWN, "service requested for unknown domain");
                sx_close(s);

                return 0;
            }
#if 0
            if(xhash_get(sess->c2s->sm_avail, s->req_to) == NULL)
            {
                log_debug(ZONE, "sm for domain '%s' is not online", s->req_to);
                sx_error(s, stream_err_HOST_GONE, "session manager for requested domain is not available");
                sx_close(s);

                return 0;
            }
#endif
            if(sess->host == NULL)
            {
                /* create host on-fly */
                sess->host = (host_t) pmalloc(xhash_pool(sess->c2s->hosts), sizeof(struct host_st));
                memcpy(sess->host, sess->c2s->vhost, sizeof(struct host_st));
                sess->host->realm = pstrdup(xhash_pool(sess->c2s->hosts), s->req_to);
                xhash_put(sess->c2s->hosts, pstrdup(xhash_pool(sess->c2s->hosts), s->req_to), sess->host);
            }

#ifdef HAVE_SSL
            if(sess->host->host_pemfile != NULL)
                sess->s->flags |= SX_SSL_STARTTLS_OFFER;
            if(sess->host->host_require_starttls)
                sess->s->flags |= SX_SSL_STARTTLS_REQUIRE;
#endif
            break;

        case event_PACKET:
            /* we're counting packets */
            sess->packet_count++;
            sess->c2s->packet_count++;

            /* check rate limits */
            if(sess->stanza_rate != NULL)
            {
                if(rate_check(sess->stanza_rate) == 0)
                {

                    /* inform the app if we haven't already */
                    if(!sess->stanza_rate_log)
                    {
                        if(s->state >= state_STREAM && sess->resources != NULL)
                            log_write(sess->c2s->log, LOG_NOTICE, "[%d] [%s] is being stanza rate limited", sess->fd->fd, jid_user(sess->resources->jid));
                        else
                            log_write(sess->c2s->log, LOG_NOTICE, "[%d] [%s, port=%d] is being stanza rate limited", sess->fd->fd, sess->ip, sess->port);

                        sess->stanza_rate_log = 1;
                    }
                }

                /* update rate limits */
                rate_add(sess->stanza_rate, 1);
            }

            nad = (nad_t) data;

            /* we only want (message|presence|iq) in jabber:client, everything else gets dropped */


            snprintf(root, 9, "%.*s", NAD_ENAME_L(nad, 0), NAD_ENAME(nad, 0));
            if(strcmp(root, "message") == 0)
            {
                ImmessageFlag = 1;
            }
            if(NAD_ENS(nad, 0) != nad_find_namespace(nad, 0, uri_CLIENT, NULL) ||
               ((!ImmessageFlag) && strcmp(root, "presence") != 0 && strcmp(root, "iq") != 0))
            {
                nad_free(nad);
                return 0;
            }

            /* resource bind */
            if((ns = nad_find_scoped_namespace(nad, uri_BIND, NULL)) >= 0 && (elem = nad_find_elem(nad, 0, ns, "bind", 1)) >= 0 && nad_find_attr(nad, 0, -1, "type", "set") >= 0)
            {
                bres_t bres;
                jid_t jid = jid_new(sess->s->auth_id, -1);

                /* get the resource */
                elem = nad_find_elem(nad, elem, ns, "resource", 1);

                /* user-specified resource */
                if(elem >= 0)
                {
                    char resource_buf[1024];

                    if(NAD_CDATA_L(nad, elem) == 0)
                    {
                        log_debug(ZONE, "empty resource specified on bind");
                        sx_nad_write(sess->s, stanza_error(nad, 0, stanza_err_BAD_REQUEST));

                        return 0;
                    }

                    snprintf(resource_buf, 1024, "%.*s", NAD_CDATA_L(nad, elem), NAD_CDATA(nad, elem));
                    /* Put resource into JID */
                    if (jid == NULL || jid_reset_components(jid, jid->node, jid->domain, resource_buf) == NULL)
                    {
                        log_debug(ZONE, "invalid jid data");
                        sx_nad_write(sess->s, stanza_error(nad, 0, stanza_err_BAD_REQUEST));

                        return 0;
                    }

                    /* check if resource already bound */
                    for(bres = sess->resources; bres != NULL; bres = bres->next)
                        if(strcmp(bres->jid->resource, jid->resource) == 0)
                        {
                            log_debug(ZONE, "resource /%s already bound - generating", jid->resource);
                            jid_random_part(jid, jid_RESOURCE);
                        }
                }
                else
                {
                    /* generate random resource */
                    log_debug(ZONE, "no resource given - generating");
                    jid_random_part(jid, jid_RESOURCE);
                }

                /* attach new_cplus bound jid holder */
                bres = (bres_t) calloc(1, sizeof(struct bres_st));
                bres->jid = jid;
                if(sess->resources != NULL)
                {
                    for(ires = sess->resources; ires->next != NULL; ires = ires->next);
                    ires->next = bres;
                }
                else
                    sess->resources = bres;

                sess->bound += 1;

                log_write(sess->c2s->log, LOG_NOTICE, "[%d] bound: jid=%s", sess->s->tag, jid_full(bres->jid));

                /* build a result packet, we'll send this back to the client after we have a session for them */
                sess->result = nad_new();

                ns = nad_add_namespace(sess->result, uri_CLIENT, NULL);

                nad_append_elem(sess->result, ns, "iq", 0);
                nad_set_attr(sess->result, 0, -1, "type", "result", 6);

                attr = nad_find_attr(nad, 0, -1, "id", NULL);
                if(attr >= 0)
                    nad_set_attr(sess->result, 0, -1, "id", NAD_AVAL(nad, attr), NAD_AVAL_L(nad, attr));

                ns = nad_add_namespace(sess->result, uri_BIND, NULL);

                nad_append_elem(sess->result, ns, "bind", 1);
                nad_append_elem(sess->result, ns, "jid", 2);
                nad_append_cdata(sess->result, jid_full(bres->jid), strlen(jid_full(bres->jid)), 3);

                /* our local id */
                sprintf(bres->c2s_id, "%d", sess->s->tag);

                /*DY ADD*/
                sx_nad_write(sess->s, sess->result);
                /* start a session with the sm */
                //sm_start(sess, bres);

                /* finished with the nad */
                nad_free(nad);

                /* handled */
                return 0;
            }

            /* resource unbind */
            if((ns = nad_find_scoped_namespace(nad, uri_BIND, NULL)) >= 0 && (elem = nad_find_elem(nad, 0, ns, "unbind", 1)) >= 0 && nad_find_attr(nad, 0, -1, "type", "set") >= 0)
            {
                char resource_buf[1024];
                bres_t bres;

                /* get the resource */
                elem = nad_find_elem(nad, elem, ns, "resource", 1);

                if(elem < 0 || NAD_CDATA_L(nad, elem) == 0)
                {
                    log_debug(ZONE, "no/empty resource given to unbind");
                    sx_nad_write(sess->s, stanza_error(nad, 0, stanza_err_BAD_REQUEST));

                    return 0;
                }

                snprintf(resource_buf, 1024, "%.*s", NAD_CDATA_L(nad, elem), NAD_CDATA(nad, elem));
                if(stringprep_xmpp_resourceprep(resource_buf, 1024) != 0)
                {
                    log_debug(ZONE, "cannot resourceprep");
                    sx_nad_write(sess->s, stanza_error(nad, 0, stanza_err_BAD_REQUEST));

                    return 0;
                }

                /* check if resource bound */
                for(bres = sess->resources; bres != NULL; bres = bres->next)
                    if(strcmp(bres->jid->resource, resource_buf) == 0)
                        break;

                if(bres == NULL)
                {
                    log_debug(ZONE, "resource /%s not bound", resource_buf);
                    sx_nad_write(sess->s, stanza_error(nad, 0, stanza_err_ITEM_NOT_FOUND));

                    return 0;
                }

                /* build a result packet, we'll send this back to the client after we close a session for them */
                sess->result = nad_new();

                ns = nad_add_namespace(sess->result, uri_CLIENT, NULL);

                nad_append_elem(sess->result, ns, "iq", 0);
                nad_set_attr(sess->result, 0, -1, "type", "result", 6);

                attr = nad_find_attr(nad, 0, -1, "id", NULL);
                if(attr >= 0)
                    nad_set_attr(sess->result, 0, -1, "id", NAD_AVAL(nad, attr), NAD_AVAL_L(nad, attr));

                /* end a session with the sm */
                //sm_end(sess, bres);

                /* finished with the nad */
                nad_free(nad);

                /* handled */
                return 0;
            }

            /* pre-session requests */
            if(!sess->active && sess->sasl_authd && sess->result == NULL && strcmp(root, "iq") == 0 && nad_find_attr(nad, 0, -1, "type", "set") >= 0)
            {
                log_debug(ZONE, "unrecognised pre-session packet, bye");
                log_write(sess->c2s->log, LOG_NOTICE, "[%d] unrecognized pre-session packet, closing stream", sess->s->tag);

                sx_error(s, stream_err_NOT_AUTHORIZED, "unrecognized pre-session stanza");
                sx_close(s);

                nad_free(nad);
                return 0;
            }

#ifdef HAVE_SSL
            /* drop packets if they have to starttls and they haven't */
            if((sess->s->flags & SX_SSL_STARTTLS_REQUIRE) && sess->s->ssf == 0)
            {
                log_debug(ZONE, "pre STARTTLS packet, dropping");
                log_write(sess->c2s->log, LOG_NOTICE, "[%d] got pre STARTTLS packet, dropping", sess->s->tag);

                sx_error(s, stream_err_POLICY_VIOLATION, "STARTTLS is required for this stream");

                nad_free(nad);
                return 0;
            }
#endif

            /*处理聊天消息*/
            log_debug(ZONE, "process msgs");




            if(1 == ImmessageFlag)
            {
                char dst_user[64] = {0};
                char messages[1024] = {0};
                //elem = nad_find_elem(nad, 0, -1, "to", 1);
                char fromnickname[80] = {0};

                attr = nad_find_attr(nad, 0, -1, "to", NULL);
                if(attr >= 0)
                {
                    snprintf(dst_user, 64, "%.*s", NAD_AVAL_L(nad, attr), NAD_AVAL(nad, attr));
                }
                else
                {
                    log_debug(ZONE, "to user invalid, dropping");
                    return 0;
                }

                attr = nad_find_attr(nad, 0, -1, "fromnickname", NULL);
                if(attr >= 0)
                {
                	char temp[80] = {0};
                    snprintf(temp, 80, "%.*s", NAD_AVAL_L(nad, attr), NAD_AVAL(nad, attr));
					int outlen = 80;
                    utf2GB18030(temp,NAD_AVAL_L(nad, attr),fromnickname,outlen);
                }

                // 查找uuid，将uuid返给发送端
                char uuid[1024] = {0};
                elem = nad_find_elem(nad, 0, -1, "uuid", 1);
                if(elem >=0)
                {
                    snprintf(uuid, 1024, "%.*s", NAD_CDATA_L(nad, elem),NAD_CDATA(nad, elem));
                    log_write(sess->c2s->log, LOG_NOTICE,"recv uuid: %s", uuid);

                    sess->result = nad_new();

                    int ns = -1;
                    nad_append_elem(sess->result, ns, "iq", 0);
                    nad_append_attr(sess->result, ns, "type","result");
                    nad_append_attr(sess->result, ns, "id","msg");

                    // 添加query字段
        			nad_append_elem(sess->result, ns, "query", 1);
        			nad_append_attr(sess->result, -1, "xmlns", uri_MSG);

                    nad_append_elem(sess->result, ns, "uuid", 2);
                    nad_append_cdata_gbk(sess->result,uuid, strlen(uuid), 3);

                    nad_append_elem(sess->result, ns, "userid", 2);
                    nad_append_cdata_gbk(sess->result,dst_user, strlen(dst_user), 3);

                    sx_nad_write(sess->s, sess->result);
                    sess->result = NULL;

                    log_write(sess->c2s->log, LOG_NOTICE,"send uuid: %s", uuid);
                }

                elem = nad_find_elem(nad, 0, -1, "body", 1);
                log_write(sess->c2s->log, LOG_NOTICE,"3sdato %s msg %s  elem %d", dst_user,messages,elem);
                if(elem >= 0)
                {
                    snprintf(messages, 1024, "%.*s", NAD_CDATA_L(nad, elem),NAD_CDATA(nad, elem));

                    string strmsg = messages;
                    char msg_gbk[1024] = {0};
                    int length = 1024;

                    int rlt = utf2GB18030(messages,NAD_CDATA_L(nad, elem),msg_gbk,length);
                    if( rlt != -1 )
                    {
                        memset(messages, 0, 1024);
                        strcpy(messages, msg_gbk);
                    }
                }

                log_write(sess->c2s->log, LOG_NOTICE,"selfid %d to %s msg %s ",sess->m_selfID,dst_user,messages);

                // 发送时间
                elem = nad_find_elem(nad, 0, -1, "sendtime", 1);
                char send_time[64]= {0};
                if(elem >= 0)
                {
                    snprintf(send_time, 63, "%.*s", NAD_CDATA_L(nad, elem),NAD_CDATA(nad, elem));
                }

                // 消息类型
                elem = nad_find_elem(nad, 0, -1, "datatype", 1);
                char datatype[64]= {0};
                if(elem >= 0)
                {
                    snprintf(datatype, 63, "%.*s", NAD_CDATA_L(nad, elem),NAD_CDATA(nad, elem));
                }

                RecvDataStruct packets;

                uint32 ipaddr = inet_addr(sess->ip);
                uint32 dst_userid = atoi(dst_user);

                XT_MSG input;
                input.dest_ip = 0;
                input.dest_port = 0;
                input.data_type = atoi(datatype);
                input.data_len= strlen(messages);
                input.from_id = sess->m_selfID;
                input.to_id= dst_userid;
                strcpy(input.fontName, "微软雅黑");
                input.fontColor = 0x002e2e2e;
                input.fontSize = 12;
                input.ver = 285;
                input.recv_flag = 2;   
                strcpy(input.from_nickname, fromnickname);
                if(strcmp(send_time, "") == 0)
                {
                    // 使用系统时间
                    input.send_time = time(NULL);
                }
                else
                {
                    input.send_time = atoi(send_time);
                }
                strcpy(input.data,messages);

                strncpy(input.uuid, uuid, MAX_UUID_LEN);

                ((generatepkt)sess->c2s->m_generatepkt)((uint32)sess->fd->fd,NULL,NULL,ipaddr,sess->port,(uint32)0x60b,sess->m_selfID,dst_userid,&packets,&input);


                log_write(sess->c2s->log, LOG_NOTICE,"########### from %u to %u DATALEN %d  ",input.from_id, input.to_id,packets.data_len);
                ((pfnjabberdCallback) sess->c2s->m_pfnUserCallback)(sess->c2s->m_pUserKey,NE_RECVDATA,&packets);

                return 0;


            }


            /*parse pkt and send out*/

            log_debug(ZONE, "opps,not process msgs,process auth");

            {
                int ns, query, type, authreg = -1, getset = -1;
                int saslflag= -1;
                config_elem_t elem_to;
                const char *to = NULL;
                elem_to = config_get(sess->c2s->config, "local.id");
                if(elem_to != NULL)
                {
                    to = *(elem_to->values);
                }

                /* need iq */
                if(NAD_ENAME_L(nad, 0) != 2 || strncmp("iq", NAD_ENAME(nad, 0), 2) != 0)
                    return 1;

                /* 处理分流获取最近联系商户以及验证账号登录请求*/
                if((ns = nad_find_scoped_namespace(nad, uri_AUTH, NULL)) >= 0 && (query = nad_find_elem(nad, 0, ns, "query", 1)) >= 0)
                {
                    authreg = 0;
                }
                else if((ns = nad_find_scoped_namespace(nad, uri_REGISTER, NULL)) >= 0 && (query = nad_find_elem(nad, 0, ns, "query", 1)) >= 0)
                {
                    authreg = 1;
                }
                else if((ns = nad_find_scoped_namespace(nad, uri_XSESSION, NULL)) >= 0 && (query = nad_find_elem(nad, 0, ns, "session", 1)) >= 0)
                {
                    saslflag = 1;
                }
                else if((ns = nad_find_scoped_namespace(nad, uri_FLOW, NULL)) >= 0 && (query = nad_find_elem(nad, 0, ns, "query", 1)) >= 0)
                {
                    char req_mID[64] = {0};
                    char req_gID[64] = {0};
                    char req_type[32] = {0};
                    //elem = nad_find_elem(nad, 0, -1, "to", 1);

                    elem = nad_find_elem(nad, 1, -1, "merchantid", 1);
                    if(elem >= 0)
                    {
                        snprintf(req_mID, 64, "%.*s", NAD_CDATA_L(nad, elem),NAD_CDATA(nad, elem));
                    }
                    elem = nad_find_elem(nad, 1, -1, "groupid", 1);
                    if(elem >= 0)
                    {
                        snprintf(req_gID, 64, "%.*s", NAD_CDATA_L(nad, elem),NAD_CDATA(nad, elem));
                    }
                    elem = nad_find_elem(nad, 1, -1, "type", 1);
                    if(elem >= 0)
                    {
                        snprintf(req_type, 32, "%.*s", NAD_CDATA_L(nad, elem),NAD_CDATA(nad, elem));
                    }

                    log_write(sess->c2s->log, LOG_NOTICE,"merchant id %s groupid %s type %s query %s",
                              req_mID,req_gID,req_type,uri_FLOW);

                    RecvDataStruct packets;

                    uint32 ipaddr = inet_addr(sess->ip);
                    XT_GET_SUB_ACCOUNT_REQ subaccount_req ;

                    if(req_mID[0] == 0)
                    {
                        subaccount_req.merchant_id = 0;
                    }
                    else
                    {
                        subaccount_req.merchant_id = atoi(req_mID);
                    }


                    if(req_gID[0] == 0)
                    {
                        subaccount_req.group_id = 0;
                    }
                    else
                    {
                        subaccount_req.group_id = atoi(req_gID);
                    }

                    if(req_type[0] == 0)
                    {
                        subaccount_req.type = 0;
                    }
                    else
                    {
                        subaccount_req.type = atoi(req_type);
                    }
                    log_write(sess->c2s->log, LOG_NOTICE,"merchant id %d groupid %d type %d query %s",
                              subaccount_req.merchant_id,subaccount_req.group_id,subaccount_req.type,
                              uri_FLOW);


                    ((generatepkt)sess->c2s->m_generatepkt)((uint32)sess->fd->fd,NULL,NULL,ipaddr,sess->port,(uint32)0x943,sess->m_selfID,0,&packets,&subaccount_req);
                    log_write(sess->c2s->log, LOG_NOTICE,"########### DATALEN %d  ",packets.data_len);

                    ((pfnjabberdCallback) sess->c2s->m_pfnUserCallback)(sess->c2s->m_pUserKey,NE_RECVDATA,&packets);

                    return 0;


                }
                else if((ns = nad_find_scoped_namespace(nad, uri_USERINFO, NULL)) >= 0 && (query = nad_find_elem(nad, 0, ns, "query", 1)) >= 0)
                {
                    char req_user[64] = {0};
                    //elem = nad_find_elem(nad, 0, -1, "to", 1);

                    elem = nad_find_elem(nad, 1, -1, "userid", 1);
                    if(elem >= 0)
                    {
                        snprintf(req_user, 64, "%.*s", NAD_CDATA_L(nad, elem),NAD_CDATA(nad, elem));
                    }


                    log_write(sess->c2s->log, LOG_NOTICE,"selfid %d to %s query %s",sess->m_selfID,req_user,uri_USERINFO);

                    RecvDataStruct packets;

                    uint32 ipaddr = inet_addr(sess->ip);
                    uint32 req_userid = 0;
                    if(req_user[0] != 0)
                    {
                        req_userid = atoi(req_user);
                    }


                    XT_USERINFO_GET_REQ req;
                    req.fid = req_userid;

                    ((generatepkt)sess->c2s->m_generatepkt)((uint32)sess->fd->fd,NULL,NULL,ipaddr,sess->port,(uint32)0x415,sess->m_selfID,req.fid,&packets,&req);

                    log_write(sess->c2s->log, LOG_NOTICE,"########### DATALEN %d  ",packets.data_len);
                    ((pfnjabberdCallback) sess->c2s->m_pfnUserCallback)(sess->c2s->m_pUserKey,NE_RECVDATA,&packets);

                    return 0;


                }
                else if((ns = nad_find_scoped_namespace(nad, uri_HEARTBEAT, NULL)) >= 0 && (query = nad_find_elem(nad, 0, ns, "ping", 1)) >= 0)
                {
                    // 心跳包
                    log_write(sess->c2s->log, LOG_NOTICE,"########### recv heartbeat ###########");

                    int c2s_from = nad_find_attr(nad, 0, -1, "from", NULL);
                    if(c2s_from < 0)
                    {
                        return 0;
                    }
                    char c_from_id[128] = {0};
                    snprintf(c_from_id, sizeof(c_from_id), "%.*s", NAD_AVAL_L(nad, c2s_from), NAD_AVAL(nad, c2s_from));
                    /*
                                        int c2s_to = nad_find_attr(nad, 0, -1, "to", NULL);
                                        if(c2s_to < 0)
                                        {
                                            return 0;
                                        }
                                        char c_to_id[128] = {0};
                                        snprintf(c_to_id, sizeof(c_to_id), "%.*s", NAD_AVAL_L(nad, c2s_to), NAD_AVAL(nad, c2s_to));
                                        if(to != NULL && strcmp(to, c_to_id) != 0)
                                        {
                                            return 0;
                                        }
                    */
                    log_write(sess->c2s->log, LOG_NOTICE,"########### heartbeat fromid: %s ###########",c_from_id);

                    int n_from_id = atoi(c_from_id);
                    if(sess->m_selfID != n_from_id)
                    {
                        // 错误的包
                        log_write(sess->c2s->log, LOG_NOTICE,"error packet");
                        return 0;
                    }

                    RecvDataStruct packets;

                    uint32 ipaddr = inet_addr(sess->ip);

                    XT_DIRECTORY_REQ req;
                    req.id = n_from_id;
                    req.local_port = sess->port;
                    req.local_ip   = ipaddr;

                    ((generatepkt)sess->c2s->m_generatepkt)((uint32)sess->fd->fd,NULL,NULL,ipaddr,sess->port,(uint32)0x301,sess->m_selfID,req.id,&packets,&req);

                    log_write(sess->c2s->log, LOG_NOTICE,"########### DATALEN %d  ",packets.data_len);
                    ((pfnjabberdCallback) sess->c2s->m_pfnUserCallback)(sess->c2s->m_pUserKey,NE_RECVDATA,&packets);

                    return 0;


                }
                else if((ns = nad_find_scoped_namespace(nad, uri_LATESTMERCHANT, NULL)) >= 0 && (query = nad_find_elem(nad, 0, ns, "query", 1)) >= 0)
                {
                    char req_user[64] = {0};
                    char req_msgnum[64] = {0};
                    //elem = nad_find_elem(nad, 0, -1, "to", 1);

                    elem = nad_find_elem(nad, 1, -1, "userid", 1);
                    if(elem >= 0)
                    {
                        snprintf(req_user, 64, "%.*s", NAD_CDATA_L(nad, elem),NAD_CDATA(nad, elem));
                    }
                    elem = nad_find_elem(nad, 1, -1, "msgnum", 1);
                    if(elem >= 0)
                    {
                        snprintf(req_msgnum, 64, "%.*s", NAD_CDATA_L(nad, elem),NAD_CDATA(nad, elem));
                    }


                    log_write(sess->c2s->log, LOG_NOTICE,"selfid %d to %s msgnum %s query %s",sess->m_selfID,req_user,req_msgnum,uri_LATESTMERCHANT);

                    RecvDataStruct packets;

                    uint32 ipaddr = inet_addr(sess->ip);
                    uint32 req_userid = 0;
                    uint32 req_msgnums = 0;
                    if(req_user[0] != 0)
                    {
                        req_userid = atoi(req_user);
                        req_msgnums = atoi(req_msgnum);
                    }

                    XT_WEB_BUSI_LIST_REQ req;
                    req.id = req_userid;
                    req.msgnum = req_msgnums;

                    ((generatepkt)sess->c2s->m_generatepkt)((uint32)sess->fd->fd,NULL,NULL,ipaddr,sess->port,(uint32)CMD_XMPP_BUSI_LIST_REQ,sess->m_selfID,0,&packets,&req);

                    log_write(sess->c2s->log, LOG_NOTICE,"########### DATALEN %d  reqid %d req msgnum %d ",packets.data_len,req_userid,req_msgnums);
                    ((pfnjabberdCallback) sess->c2s->m_pfnUserCallback)(sess->c2s->m_pUserKey,NE_RECVDATA,&packets);

                    return 0;


                }
                else if((ns = nad_find_scoped_namespace(nad, uri_HISTORYMSG, NULL)) >= 0 && (query = nad_find_elem(nad, 0, ns, "query", 1)) >= 0)
                {
                    // 获取历史消息
                    char szFriendID[64] = {0};
                    char szBgnTime[64] = {0};
                    char szEndTime[64] = {0};
                    char szMaxMsg[64] = {0};
                    char szMsgID[64] = {0};

                    elem = nad_find_elem(nad, 1, -1, "friendid", 1);
                    if(elem >= 0)
                    {
                        snprintf(szFriendID, 64, "%.*s", NAD_CDATA_L(nad, elem),NAD_CDATA(nad, elem));
                    }
                    elem = nad_find_elem(nad, 1, -1, "bgntime", 1);
                    if(elem >= 0)
                    {
                        snprintf(szBgnTime, 64, "%.*s", NAD_CDATA_L(nad, elem),NAD_CDATA(nad, elem));
                    }
                    elem = nad_find_elem(nad, 1, -1, "endtime", 1);
                    if(elem >= 0)
                    {
                        snprintf(szEndTime, 64, "%.*s", NAD_CDATA_L(nad, elem),NAD_CDATA(nad, elem));
                    }
                    elem = nad_find_elem(nad, 1, -1, "maxmsg", 1);
                    if(elem >= 0)
                    {
                        snprintf(szMaxMsg, 64, "%.*s", NAD_CDATA_L(nad, elem),NAD_CDATA(nad, elem));
                    }
                    elem = nad_find_elem(nad, 1, -1, "msgid", 1);
                    if(elem >= 0)
                    {
                        snprintf(szMsgID, 64, "%.*s", NAD_CDATA_L(nad, elem),NAD_CDATA(nad, elem));
                    }

                    uint32 frienid = 0;
                    uint32 selfid = 0;
                    uint32 bgntime = 0;
                    uint32 endtime = 0;
                    uint32 maxmsg = 50;
                    uint32 msgid = 0;

                    if(szFriendID[0] != 0)
                    {
                        frienid = atoi(szFriendID);
                    }
                    selfid = sess->m_selfID;
                    if(szBgnTime[0] != 0)
                    {
                        bgntime = atoi(szBgnTime);
                    }
                    if(szEndTime[0] != 0)
                    {
                        endtime = atoi(szEndTime);
                    }
                    else
                    {
                        endtime = time(NULL);
                    }
                    if(szMaxMsg[0] != 0)
                    {
                        maxmsg = atoi(szMaxMsg);
                        if(maxmsg > 50)
                        {
                        	maxmsg = 50;
                        }
                    }
                    if(szMsgID[0] != 0)
                    {
                        msgid = atoi(szMsgID);
                    }

                    log_write(sess->c2s->log, LOG_NOTICE,"history msg ack.selfid:%d, friendid:%d, bgntime:%d, endtime:%d, maxmsg:%d",
                              selfid, frienid, bgntime, endtime, maxmsg);

                    if(szFriendID == 0 || bgntime > endtime)
                    {
                        // 错误的包
                        log_write(sess->c2s->log, LOG_NOTICE,"error packet");
                        return 0;
                    }

                    MsgListReq req;
                    req.set_fromid(selfid);
                    req.set_toid(frienid);
                    req.set_bgntime(bgntime);
                    req.set_endtime(endtime);
                    req.set_maxmsg(maxmsg);
                    req.set_bgnmsgid(msgid);

                    RecvDataStruct packets;
                    uint32 ipaddr = inet_addr(sess->ip);
                    ((generatepkt)sess->c2s->m_generatepkt)((uint32)sess->fd->fd,NULL,NULL,ipaddr,sess->port,(uint32)CMD_HISTORY_MSG_REQ,sess->m_selfID,0,&packets,&req);
                    log_write(sess->c2s->log, LOG_NOTICE,"########### DATALEN %d  reqid %d req msgnum %d ",packets.data_len,selfid,maxmsg);
                    ((pfnjabberdCallback) sess->c2s->m_pfnUserCallback)(sess->c2s->m_pUserKey,NE_RECVDATA,&packets);
                }
#if 0
                else if((ns = nad_find_scoped_namespace(nad, uri_REGISTER, NULL)) >= 0 && (query = nad_find_elem(nad, 0, ns, "query", 1)) >= 0)
                {


                }
                else if((ns = nad_find_scoped_namespace(nad, uri_REGISTER, NULL)) >= 0 && (query = nad_find_elem(nad, 0, ns, "query", 1)) >= 0)
                {


                }
#endif
                else
                {
                    nad_free(nad);
                    return 1;
                }
                /* if its to someone else, pass it */
                if(nad_find_attr(nad, 0, -1, "to", NULL) >= 0 && nad_find_attr(nad, 0, -1, "to", sess->s->req_to) < 0)
                {
                    nad_free(nad);
                    return 1;
                }

                /* need a type */
                if((type = nad_find_attr(nad, 0, -1, "type", NULL)) < 0 || NAD_AVAL_L(nad, type) != 3)
                {
                    sx_nad_write(sess->s, stanza_tofrom(stanza_error(nad, 0, stanza_err_BAD_REQUEST), 0));
                    nad_free(nad);
                    return 0;
                }

                /* get or set? */
                if(strncmp("get", NAD_AVAL(nad, type), NAD_AVAL_L(nad, type)) == 0)
                    getset = 0;
                else if(strncmp("set", NAD_AVAL(nad, type), NAD_AVAL_L(nad, type)) == 0)
                    getset = 1;
                else
                {
                    sx_nad_write(sess->s, stanza_tofrom(stanza_error(nad, 0, stanza_err_BAD_REQUEST), 0));
                    nad_free(nad);
                    return 0;
                }

                /* hand to the correct handler */
                if(authreg == 0)
                {
                    /* can't do iq:auth after sasl auth */
                    if(sess->sasl_authd)
                    {
                        sx_nad_write(sess->s, stanza_tofrom(stanza_error(nad, 0, stanza_err_NOT_ALLOWED), 0));
                        nad_free(nad);
                        return 0;
                    }

                    if(getset == 0)
                    {
                        log_debug(ZONE, "auth get");
                        //      _authreg_auth_get(c2s, sess, nad);
                    }
                    else if(getset == 1)
                    {
                        log_debug(ZONE, "auth set 123321123321");
                        nad_t nadcy = nad_copy(nad);
                        loginNodManager[sess->fd->fd] = nadcy;
                        _authreg_auth_set(sess->c2s, sess, nad);
                    }
                }

                if(saslflag == 1)
                {
                    if(getset == 1)
                    {
                        log_debug(ZONE, "auth set 1233211233214");
                        //   nad_t nadcy = nad_copy(nad);
                        //   loginNodManager[sess->fd->fd] = nadcy;
                        _authreg_sasl_auth_set(sess->c2s, sess, nad);
                    }
                }
#if 0
                if(authreg == 1)
                {
                    if(getset == 0)
                    {
                        log_debug(ZONE, "register get");
                        //      _authreg_register_get(c2s, sess, nad);
                    }
                    else if(getset == 1)
                    {
                        log_debug(ZONE, "register set");
                        //      _authreg_register_set(c2s, sess, nad);
                    }
                }
#endif

                //nad_free(nad);

                /* handled */
                return 0;
            }



            /* handle iq:auth packets */
            //      if(authreg_process(sess->c2s, sess, nad) == 0)
            //          return 0;

            /* drop it if no session */
            if(!sess->active)
            {
                log_debug(ZONE, "pre-session packet, bye");
                log_write(sess->c2s->log, LOG_NOTICE, "[%d] packet sent before session start, closing stream", sess->s->tag);

                sx_error(s, stream_err_NOT_AUTHORIZED, "stanza sent before session start");
                sx_close(s);

                nad_free(nad);
                return 0;
            }

            /* validate 'from' */
            assert(sess->resources != NULL);
            if(sess->bound > 1)
            {
                bres = NULL;
                if((attr = nad_find_attr(nad, 0, -1, "from", NULL)) >= 0)
                    for(bres = sess->resources; bres != NULL; bres = bres->next)
                        if(strncmp(jid_full(bres->jid), NAD_AVAL(nad, attr), NAD_AVAL_L(nad, attr)) == 0)
                            break;

                if(bres == NULL)
                {
                    if(attr >= 0)
                    {
                        log_debug(ZONE, "packet from: %.*s that has not bound the resource", NAD_AVAL_L(nad, attr), NAD_AVAL(nad, attr));
                    }
                    else
                    {
                        log_debug(ZONE, "packet without 'from' on multiple resource stream");
                    }

                    sx_nad_write(sess->s, stanza_error(nad, 0, stanza_err_UNKNOWN_SENDER));

                    return 0;
                }
            }
            else
                bres = sess->resources;

            /* pass it on to the session manager */
//            sm_packet(sess, bres, nad);

            break;

        case event_OPEN:

            /* only send a result and bring us online if this wasn't a sasl auth */
            if(strlen(s->auth_method) < 4 || strncmp("SASL", s->auth_method, 4) != 0)
            {
                /* return the auth result to the client */
                sx_nad_write(s, sess->result);
                sess->result = NULL;

                /* we're good to go */
                sess->active = 1;
            }

            /* they sasl auth'd, so we only want the new_cplus-style session start */
            else
            {
                log_write(sess->c2s->log, LOG_NOTICE, "[%d] %s authentication succeeded: %s %s:%d%s%s",
                          sess->s->tag, &sess->s->auth_method[5],
                          sess->s->auth_id, sess->s->ip, sess->s->port,
                          sess->s->ssf ? " TLS" : "", sess->s->compressed ? " ZLIB" : ""
                         );
                sess->sasl_authd = 1;
            }

            break;

        case event_CLOSED:
            mio_close(sess->c2s->mio, sess->fd);
            sess->fd = NULL;
            return -1;
    }

    return 0;
}

static int _c2s_client_accept_check(c2s_t c2s, mio_fd_t fd, const char *ip)
{
    rate_t rt;

    if(access_check(c2s->access, ip) == 0)
    {
        log_write(c2s->log, LOG_NOTICE, "[%d] [%s] access denied by configuration", fd->fd, ip);
        return 1;
    }

    if(c2s->conn_rate_total != 0)
    {
        rt = (rate_t) xhash_get(c2s->conn_rates, ip);
        if(rt == NULL)
        {
            rt = rate_new(c2s->conn_rate_total, c2s->conn_rate_seconds, c2s->conn_rate_wait);
            xhash_put(c2s->conn_rates, pstrdup(xhash_pool(c2s->conn_rates), ip), (void *) rt);
            pool_cleanup(xhash_pool(c2s->conn_rates), (void (*)(void *)) rate_free, rt);
        }

        if(rate_check(rt) == 0)
        {
            log_write(c2s->log, LOG_NOTICE, "[%d] [%s] is being connect rate limited", fd->fd, ip);
            return 1;
        }

        rate_add(rt, 1);
    }

    return 0;
}

int _c2s_client_mio_callback(mio_t m, mio_action_t a, mio_fd_t fd, void *data, void *arg)
{
    sess_t sess = (sess_t) arg;
    c2s_t c2s = (c2s_t) arg;
    bres_t bres;
    struct sockaddr_storage sa;
    socklen_t namelen = sizeof(sa);
    int port, nbytes, flags = 0;
	uint32 ipaddr = -1;
	XT_LOGOUT req;
	RecvDataStruct packets;

    switch(a)
    {
        case action_READ:
            log_debug(ZONE, "read action on fd %d", fd->fd);

            /* they did something */
            sess->last_activity = time(NULL);

            ioctl(fd->fd, FIONREAD, &nbytes);
            if(nbytes == 0)
            {
                sx_kill(sess->s);
                return 0;
            }

            return sx_can_read(sess->s);

        case action_WRITE:
            log_debug(ZONE, "write action on fd %d", fd->fd);

            return sx_can_write(sess->s);

        case action_CLOSE:
            log_debug(ZONE, "close action on fd %d", fd->fd);

	        req.id = sess->m_selfID;
			req.authData = 0x00;	  
			ipaddr = inet_addr(sess->ip);
	        ((generatepkt)sess->c2s->m_generatepkt)((uint32)sess->fd->fd,NULL,NULL,ipaddr,sess->port,(uint32)CMD_LOGOUT,sess->m_selfID,req.id,&packets,&req);

	        log_write(sess->c2s->log, LOG_NOTICE,"########### DATALEN %d  ",packets.data_len);
	        ((pfnjabberdCallback) sess->c2s->m_pfnUserCallback)(sess->c2s->m_pUserKey,NE_RECVDATA,&packets);
			
            log_write(sess->c2s->log, LOG_NOTICE, "[%d] [%s, port=%d] disconnect jid=%s, packets: %i", sess->fd->fd, sess->ip, sess->port, ((sess->resources)?((char*) jid_full(sess->resources->jid)):"unbound"), sess->packet_count);

            /* tell the sm to close their session */
#if 0

            if(sess->active)
                for(bres = sess->resources; bres != NULL; bres = bres->next)
                    sm_end(sess, bres);


            /* call the session end callback to allow for authreg
             * module to cleanup priavte_cplus data */
            if(sess->c2s->ar->sess_end != NULL)
                (sess->c2s->ar->sess_end)(sess->c2s->ar, sess);
#endif

            /* force free authreg_private if pointer is still set */
            if (sess->authreg_private != NULL)
            {
                free(sess->authreg_private);
                sess->authreg_private = NULL;
            }

            jqueue_push(sess->c2s->dead, (void *) sess->s, 0);

            // erase sessManager中的数据
            sessManager.erase(sess->m_selfID);

            xhash_zap(sess->c2s->sessions, sess->skey);

            jqueue_push(sess->c2s->dead_sess, (void *) sess, 0);

            break;

        case action_ACCEPT:
            log_debug(ZONE, "accept action on fd %d", fd->fd);

            getpeername(fd->fd, (struct sockaddr *) &sa, &namelen);
            port = j_inet_getport(&sa);

            log_write(c2s->log, LOG_NOTICE, "[%d] [%s, port=%d] connect", fd->fd, (char *) data, port);

            if(_c2s_client_accept_check(c2s, fd, (char *) data) != 0)
                return 1;

            sess = (sess_t) calloc(1, sizeof(struct sess_st));

            sess->c2s = c2s;

            sess->fd = fd;

            sess->ip = strdup((char *) data);
            sess->port = port;

            /* they did something */
            sess->last_activity = time(NULL);

            sess->s = sx_new(c2s->sx_env, fd->fd, _c2s_client_sx_callback, (void *) sess);
            mio_app(m, fd, _c2s_client_mio_callback, (void *) sess);

            if(c2s->stanza_size_limit != 0)
                sess->s->rbytesmax = c2s->stanza_size_limit;

            if(c2s->byte_rate_total != 0)
                sess->rate = rate_new(c2s->byte_rate_total, c2s->byte_rate_seconds, c2s->byte_rate_wait);

            if(c2s->stanza_rate_total != 0)
                sess->stanza_rate = rate_new(c2s->stanza_rate_total, c2s->stanza_rate_seconds, c2s->stanza_rate_wait);

            /* give IP to SX */
            sess->s->ip = sess->ip;
            sess->s->port = sess->port;

            /* find out which port this is */
            getsockname(fd->fd, (struct sockaddr *) &sa, &namelen);
            port = j_inet_getport(&sa);

            /* remember it */
            sprintf(sess->skey, "%d", fd->fd);
            xhash_put(c2s->sessions, sess->skey, (void *) sess);

            flags = SX_SASL_OFFER;
#ifdef HAVE_SSL
            /* go ssl wrappermode if they're on the ssl port */
            if(port == c2s->local_ssl_port)
                flags |= SX_SSL_WRAPPER;
#endif
#ifdef HAVE_LIBZ
            if(c2s->compression)
                flags |= SX_COMPRESS_OFFER;
#endif
            sx_server_init(sess->s, flags);

            break;

        default:
        {
            log_debug(ZONE, "other");
        }
        break;
    }

    return 0;
}

static void _c2s_component_presence(c2s_t c2s, nad_t nad)
{
    int attr;
    char from[1024];
    sess_t sess;
    union xhashv xhv;

    if((attr = nad_find_attr(nad, 0, -1, "from", NULL)) < 0)
    {
        nad_free(nad);
        return;
    }

    strncpy(from, NAD_AVAL(nad, attr), NAD_AVAL_L(nad, attr));
    from[NAD_AVAL_L(nad, attr)] = '\0';

    if(nad_find_attr(nad, 0, -1, "type", NULL) < 0)
    {
        log_debug(ZONE, "component available from '%s'", from);

        log_debug(ZONE, "sm for serviced domain '%s' online", from);

        xhash_put(c2s->sm_avail, pstrdup(xhash_pool(c2s->sm_avail), from), (void *) 1);

        nad_free(nad);
        return;
    }

    if(nad_find_attr(nad, 0, -1, "type", "unavailable") < 0)
    {
        nad_free(nad);
        return;
    }

    log_debug(ZONE, "component unavailable from '%s'", from);

    if(xhash_get(c2s->sm_avail, from) != NULL)
    {
        log_debug(ZONE, "sm for serviced domain '%s' offline", from);

        if(xhash_iter_first(c2s->sessions))
            do
            {
                xhv.sess_val = &sess;
                xhash_iter_get(c2s->sessions, NULL, NULL, xhv.val);

                if(sess->resources != NULL && strcmp(sess->resources->jid->domain, from) == 0)
                {
                    log_debug(ZONE, "killing session %s", jid_user(sess->resources->jid));

                    sess->active = 0;
                    if(sess->s) sx_close(sess->s);
                }
            }
            while(xhash_iter_next(c2s->sessions));

        xhash_zap(c2s->sm_avail, from);
    }
}

int c2s_router_sx_callback(sx_t s, sx_event_t e, void *data, void *arg)
{
    c2s_t c2s = (c2s_t) arg;
    sx_buf_t buf = (sx_buf_t) data;
    sx_error_t *sxe;
    nad_t nad;
    int len, elem, from, c2sid, smid, action, id, ns, attr, scan, replaced;
    char skey[44];
    sess_t sess;
    bres_t bres, ires;
    char *smcomp;

    switch(e)
    {
        case event_WANT_READ:
            log_debug(ZONE, "want read");
            mio_read(c2s->mio, c2s->fd);
            break;

        case event_WANT_WRITE:
            log_debug(ZONE, "want write");
            mio_write(c2s->mio, c2s->fd);
            break;

        case event_READ:
            log_debug(ZONE, "reading from %d", c2s->fd->fd);

            /* do the read */
            len = recv(c2s->fd->fd, buf->data, buf->len, 0);

            if(len < 0)
            {
                if(MIO_WOULDBLOCK)
                {
                    buf->len = 0;
                    return 0;
                }

                log_write(c2s->log, LOG_NOTICE, "[%d] [router] read error: %s (%d)", c2s->fd->fd, MIO_STRERROR(MIO_ERROR), MIO_ERROR);

                sx_kill(s);

                return -1;
            }

            else if(len == 0)
            {
                /* they went away */
                sx_kill(s);

                return -1;
            }

            log_debug(ZONE, "read %d bytes", len);

            buf->len = len;

            return len;

        case event_WRITE:
            log_debug(ZONE, "writing to %d", c2s->fd->fd);

            len = send(c2s->fd->fd, buf->data, buf->len, 0);
            if(len >= 0)
            {
                log_debug(ZONE, "%d bytes written", len);
                return len;
            }

            if(MIO_WOULDBLOCK)
                return 0;

            log_write(c2s->log, LOG_NOTICE, "[%d] [router] write error: %s (%d)", c2s->fd->fd, MIO_STRERROR(MIO_ERROR), MIO_ERROR);

            sx_kill(s);

            return -1;

        case event_ERROR:
            sxe = (sx_error_t *) data;
            log_write(c2s->log, LOG_NOTICE, "error from router: %s (%s)", sxe->generic, sxe->specific);

            if(sxe->code == SX_ERR_AUTH)
                sx_close(s);

            break;

        case event_STREAM:
            break;

        case event_OPEN:
            log_write(c2s->log, LOG_NOTICE, "connection to router established");

            /* set connection attempts counter */
            c2s->retry_left = c2s->retry_lost;

            nad = nad_new();
            ns = nad_add_namespace(nad, uri_COMPONENT, NULL);
            nad_append_elem(nad, ns, "bind", 0);
            nad_append_attr(nad, -1, "name", c2s->id);

            log_debug(ZONE, "requesting component bind for '%s'", c2s->id);

            sx_nad_write(c2s->router, nad);

            return 0;

        case event_PACKET:
            nad = (nad_t) data;

            /* drop unqualified packets */
            if(NAD_ENS(nad, 0) < 0)
            {
                nad_free(nad);
                return 0;
            }

            /* watch for the features packet */
            if(s->state == state_STREAM)
            {
                if(NAD_NURI_L(nad, NAD_ENS(nad, 0)) != strlen(uri_STREAMS) || strncmp(uri_STREAMS, NAD_NURI(nad, NAD_ENS(nad, 0)), strlen(uri_STREAMS)) != 0 || NAD_ENAME_L(nad, 0) != 8 || strncmp("features", NAD_ENAME(nad, 0), 8) != 0)
                {
                    log_debug(ZONE, "got a non-features packet on an unauth'd stream, dropping");
                    nad_free(nad);
                    return 0;
                }

#ifdef HAVE_SSL
                /* starttls if we can */
                if(c2s->sx_ssl != NULL && c2s->router_pemfile != NULL && s->ssf == 0)
                {
                    ns = nad_find_scoped_namespace(nad, uri_TLS, NULL);
                    if(ns >= 0)
                    {
                        elem = nad_find_elem(nad, 0, ns, "starttls", 1);
                        if(elem >= 0)
                        {
                            if(sx_ssl_client_starttls(c2s->sx_ssl, s, c2s->router_pemfile, c2s->router_private_key_password) == 0)
                            {
                                nad_free(nad);
                                return 0;
                            }
                            log_write(c2s->log, LOG_ERR, "unable to establish encrypted session with router");
                        }
                    }
                }
#endif

                /* !!! pull the list of mechanisms, and choose the best one.
                 *     if there isn't an appropriate one, error and bail */

                /* authenticate */
                sx_sasl_auth(c2s->sx_sasl, s, "jabberd-router", "DIGEST-MD5", c2s->router_user, c2s->router_pass);

                nad_free(nad);
                return 0;
            }

            /* watch for the bind response */
            if(s->state == state_OPEN && !c2s->online)
            {
                if(NAD_NURI_L(nad, NAD_ENS(nad, 0)) != strlen(uri_COMPONENT) || strncmp(uri_COMPONENT, NAD_NURI(nad, NAD_ENS(nad, 0)), strlen(uri_COMPONENT)) != 0 || NAD_ENAME_L(nad, 0) != 4 || strncmp("bind", NAD_ENAME(nad, 0), 4) != 0)
                {
                    log_debug(ZONE, "got a packet from router, but we're not online, dropping");
                    nad_free(nad);
                    return 0;
                }

                /* catch errors */
                attr = nad_find_attr(nad, 0, -1, "error", NULL);
                if(attr >= 0)
                {
                    log_write(c2s->log, LOG_ERR, "router refused bind request (%.*s)", NAD_AVAL_L(nad, attr), NAD_AVAL(nad, attr));
                    exit(1);
                }

                log_debug(ZONE, "coming online");

                /* if we're coming online for the first time, setup listening sockets */
#ifdef HAVE_SSL
                if(c2s->server_fd == 0 && c2s->server_ssl_fd == 0)
                {
#else
                if(c2s->server_fd == 0)
                {
#endif
                    if(c2s->local_port != 0)
                    {
                        c2s->server_fd = mio_listen(c2s->mio, c2s->local_port, c2s->local_ip, _c2s_client_mio_callback, (void *) c2s);
                        if(c2s->server_fd == NULL)
                            log_write(c2s->log, LOG_ERR, "[%s, port=%d] failed to listen", c2s->local_ip, c2s->local_port);
                        else
                            log_write(c2s->log, LOG_NOTICE, "[%s, port=%d] listening for connections", c2s->local_ip, c2s->local_port);
                    }
                    else
                        c2s->server_fd = NULL;

#ifdef HAVE_SSL
                    if(c2s->local_ssl_port != 0 && c2s->local_pemfile != NULL)
                    {
                        c2s->server_ssl_fd = mio_listen(c2s->mio, c2s->local_ssl_port, c2s->local_ip, _c2s_client_mio_callback, (void *) c2s);
                        if(c2s->server_ssl_fd == NULL)
                            log_write(c2s->log, LOG_ERR, "[%s, port=%d] failed to listen", c2s->local_ip, c2s->local_ssl_port);
                        else
                            log_write(c2s->log, LOG_NOTICE, "[%s, port=%d] listening for SSL connections", c2s->local_ip, c2s->local_ssl_port);
                    }
                    else
                        c2s->server_ssl_fd = NULL;
#endif
                }

#ifdef HAVE_SSL
                if(c2s->server_fd == NULL && c2s->server_ssl_fd == NULL && c2s->pbx_pipe == NULL)
                {
                    log_write(c2s->log, LOG_ERR, "both normal and SSL ports are disabled, nothing to do!");
#else
                if(c2s->server_fd == NULL && c2s->pbx_pipe == NULL)
                {
                    log_write(c2s->log, LOG_ERR, "server port is disabled, nothing to do!");
#endif
                    exit(1);
                }

                /* open PBX integration FIFO */
                if(c2s->pbx_pipe != NULL)
                    c2s_pbx_init(c2s);

                /* we're online */
                c2s->online = c2s->started = 1;
                log_write(c2s->log, LOG_NOTICE, "ready for connections", c2s->id);

                nad_free(nad);
                return 0;
            }

            /* need component packets */
            if(NAD_NURI_L(nad, NAD_ENS(nad, 0)) != strlen(uri_COMPONENT) || strncmp(uri_COMPONENT, NAD_NURI(nad, NAD_ENS(nad, 0)), strlen(uri_COMPONENT)) != 0)
            {
                log_debug(ZONE, "wanted component packet, dropping");
                nad_free(nad);
                return 0;
            }

            /* component presence */
            if(NAD_ENAME_L(nad, 0) == 8 && strncmp("presence", NAD_ENAME(nad, 0), 8) == 0)
            {
                _c2s_component_presence(c2s, nad);
                return 0;
            }

            /* we want route */
            if(NAD_ENAME_L(nad, 0) != 5 || strncmp("route", NAD_ENAME(nad, 0), 5) != 0)
            {
                log_debug(ZONE, "wanted {component}route, dropping");
                nad_free(nad);
                return 0;
            }

            /* only handle unicasts */
            if(nad_find_attr(nad, 0, -1, "type", NULL) >= 0)
            {
                log_debug(ZONE, "non-unicast packet, dropping");
                nad_free(nad);
                return 0;
            }

            /* need some payload */
            if(nad->ecur == 1)
            {
                log_debug(ZONE, "no route payload, dropping");
                nad_free(nad);
                return 0;
            }

            ns = nad_find_namespace(nad, 1, uri_SESSION, NULL);
            if(ns < 0)
            {
                log_debug(ZONE, "not a c2s packet, dropping");
                nad_free(nad);
                return 0;
            }

            /* figure out the session */
            c2sid = nad_find_attr(nad, 1, ns, "c2s", NULL);
            if(c2sid < 0)
            {
                log_debug(ZONE, "no c2s id on payload, dropping");
                nad_free(nad);
                return 0;
            }
            snprintf(skey, sizeof(skey), "%.*s", NAD_AVAL_L(nad, c2sid), NAD_AVAL(nad, c2sid));

            /* find the session, quietly drop if we don't have it */
            sess = (sess_t)xhash_get(c2s->sessions, skey);
            if(sess == NULL)
            {
                /* if we get this, the SM probably thinks the session is still active
                 * so we need to tell SM to free it up */
                log_debug(ZONE, "no session for %s", skey);

                /* check if it's a started action; otherwise we could end up in an infinite loop
                 * trying to tell SM to close in response to errors */
                action = nad_find_attr(nad, 1, -1, "action", NULL);
                if(action >= 0 && NAD_AVAL_L(nad, action) == 7 && strncmp("started", NAD_AVAL(nad, action), 7) == 0)
                {
                    int target;
                    bres_t tres;
                    sess_t tsess;

                    log_write(c2s->log, LOG_NOTICE, "session %s does not exist; telling sm to close", skey);

                    /* we don't have a session and we don't have a resource; we need to forge them both
                     * to get SM to close stuff */
                    target = nad_find_attr(nad, 1, -1, "target", NULL);
                    smid = nad_find_attr(nad, 1, ns, "sm", NULL);
                    if(target < 0 || smid < 0)
                    {
                        const char *buf;
                        int len;
                        nad_print(nad, 0, &buf, &len);
                        log_write(c2s->log, LOG_NOTICE, "sm sent an invalid start packet: %.*s", len, buf );
                        nad_free(nad);
                        return 0;
                    }

                    /* build temporary resource to close session for */
                    tres = NULL;
                    tres = (bres_t) calloc(1, sizeof(struct bres_st));
                    tres->jid = jid_new(NAD_AVAL(nad, target), NAD_AVAL_L(nad, target));

                    strncpy(tres->c2s_id, skey, sizeof(tres->c2s_id));
                    snprintf(tres->sm_id, sizeof(tres->sm_id), "%.*s", NAD_AVAL_L(nad, smid), NAD_AVAL(nad, smid));

                    /* make a temporary session */
                    tsess = (sess_t) calloc(1, sizeof(struct sess_st));
                    tsess->c2s = c2s;
                    tsess->result = nad_new();
                    strncpy(tsess->skey, skey, sizeof(tsess->skey));

                    /* end a session with the sm */
//                    sm_end(tsess, tres);

                    /* free our temporary messes */
                    nad_free(tsess->result);
                    jid_free(tres->jid); //TODO will this crash?
                    free(tsess);
                    free(tres);
                }

                nad_free(nad);
                return 0;
            }

            /* if they're pre-stream, then this is leftovers from a previous session */
            if(sess->s && sess->s->state < state_STREAM)
            {
                log_debug(ZONE, "session %s is pre-stream", skey);

                nad_free(nad);
                return 0;
            }

            /* check the sm session id if they gave us one */
            smid = nad_find_attr(nad, 1, ns, "sm", NULL);

            /* get the action attribute */
            action = nad_find_attr(nad, 1, -1, "action", NULL);

            /* first user created packets - these are out of session */
            if(action >= 0 && NAD_AVAL_L(nad, action) == 7 && strncmp("created", NAD_AVAL(nad, action), 7) == 0)
            {

                nad_free(nad);

                if(sess->result)
                {
                    /* return the result to the client */
                    sx_nad_write(sess->s, sess->result);
                    sess->result = NULL;
                }
                else
                {
                    log_write(sess->c2s->log, LOG_WARNING, "user created for session %s which is already gone", skey);
                }

                return 0;
            }

            /* route errors */
            if(nad_find_attr(nad, 0, -1, "error", NULL) >= 0)
            {
                log_debug(ZONE, "routing error");

                if(sess->s)
                {
                    sx_error(sess->s, stream_err_INTERNAL_SERVER_ERROR, "internal server error");
                    sx_close(sess->s);
                }

                nad_free(nad);
                return 0;
            }

            /* all other packets need to contain an sm ID */
            if (smid < 0)
            {
                log_debug(ZONE, "received packet from sm without an sm ID, dropping");
                nad_free(nad);
                return 0;
            }

            /* find resource that we got packet for */
            bres = NULL;
            if(smid >= 0)
                for(bres = sess->resources; bres != NULL; bres = bres->next)
                {
                    if(bres->sm_id[0] == '\0' || (strlen(bres->sm_id) == NAD_AVAL_L(nad, smid) && strncmp(bres->sm_id, NAD_AVAL(nad, smid), NAD_AVAL_L(nad, smid)) == 0))
                        break;
                }
            if(bres == NULL)
            {
                jid_t jid = NULL;
                bres_t tres = NULL;

                /* if it's a failure, just drop it */
                if(nad_find_attr(nad, 1, ns, "failed", NULL) >= 0)
                {
                    nad_free(nad);
                    return 0;
                }

                /* build temporary resource to close session for */
                tres = (bres_t) calloc(1, sizeof(struct bres_st));
                if(sess->s)
                {
                    jid = jid_new(sess->s->auth_id, -1);
                    sprintf(tres->c2s_id, "%d", sess->s->tag);
                }
                else
                {
                    /* does not have SX - extract values from route packet */
                    int c2sid, target;
                    c2sid = nad_find_attr(nad, 1, ns, "c2s", NULL);
                    target = nad_find_attr(nad, 1, -1, "target", NULL);
                    if(c2sid < 0 || target < 0)
                    {
                        log_debug(ZONE, "needed ids not found - c2sid:%d target:%d", c2sid, target);
                        nad_free(nad);
                        free(tres);
                        return 0;
                    }
                    jid = jid_new(NAD_AVAL(nad, target), NAD_AVAL_L(nad, target));
                    snprintf(tres->c2s_id, sizeof(tres->c2s_id), "%.*s", NAD_AVAL_L(nad, c2sid), NAD_AVAL(nad, c2sid));
                }
                tres->jid = jid;
                snprintf(tres->sm_id, sizeof(tres->sm_id), "%.*s", NAD_AVAL_L(nad, smid), NAD_AVAL(nad, smid));

                if(sess->resources)
                {
                    log_debug(ZONE, "expected packet from sm session %s, but got one from %.*s, ending sm session", sess->resources->sm_id, NAD_AVAL_L(nad, smid), NAD_AVAL(nad, smid));
                }
                else
                {
                    log_debug(ZONE, "no resource bound yet, but got packet from sm session %.*s, ending sm session", NAD_AVAL_L(nad, smid), NAD_AVAL(nad, smid));
                }

                /* end a session with the sm */
//                sm_end(sess, tres);

                /* finished with the nad */
                nad_free(nad);

                /* free temp objects */
                jid_free(jid);
                free(tres);

                return 0;
            }

            /* session control packets */
            if(NAD_ENS(nad, 1) == ns && action >= 0)
            {
                /* end responses */

                /* !!! this "replaced" stuff is a hack - its really a subaction of "ended".
                 *     hurrah, another control protocol rewrite is needed :(
                 */

                replaced = 0;
                if(NAD_AVAL_L(nad, action) == 8 && strncmp("replaced", NAD_AVAL(nad, action), NAD_AVAL_L(nad, action)) == 0)
                    replaced = 1;
                if(sess->active &&
                   (replaced || (NAD_AVAL_L(nad, action) == 5 && strncmp("ended", NAD_AVAL(nad, action), NAD_AVAL_L(nad, action)) == 0)))
                {

                    sess->bound -= 1;
                    /* no more resources bound? */
                    if(sess->bound < 1)
                    {
                        sess->active = 0;

                        if(sess->s)
                        {
                            /* return the unbind result to the client */
                            if(sess->result != NULL)
                            {
                                sx_nad_write(sess->s, sess->result);
                                sess->result = NULL;
                            }

                            if(replaced)
                                sx_error(sess->s, stream_err_CONFLICT, NULL);

                            sx_close(sess->s);

                        }
                        else
                        {
                            // handle fake PBX sessions
                            if(sess->result != NULL)
                            {
                                nad_free(sess->result);
                                sess->result = NULL;
                            }
                        }

                        nad_free(nad);
                        return 0;
                    }

                    /* else remove the bound resource */
                    if(bres == sess->resources)
                    {
                        sess->resources = bres->next;
                    }
                    else
                    {
                        for(ires = sess->resources; ires != NULL; ires = ires->next)
                            if(ires->next == bres)
                                break;
                        assert(ires != NULL);
                        ires->next = bres->next;
                    }

                    log_write(sess->c2s->log, LOG_NOTICE, "[%d] unbound: jid=%s", sess->s->tag, jid_full(bres->jid));

                    jid_free(bres->jid);
                    free(bres);

                    /* and return the unbind result to the client */
                    if(sess->result != NULL)
                    {
                        sx_nad_write(sess->s, sess->result);
                        sess->result = NULL;
                    }

                    return 0;
                }

                id = nad_find_attr(nad, 1, -1, "id", NULL);

                /* make sure the id matches */
                if(id < 0 || bres->sm_request[0] == '\0' || strlen(bres->sm_request) != NAD_AVAL_L(nad, id) || strncmp(bres->sm_request, NAD_AVAL(nad, id), NAD_AVAL_L(nad, id)) != 0)
                {
                    if(id >= 0)
                    {
                        log_debug(ZONE, "got a response with id %.*s, but we were expecting %s", NAD_AVAL_L(nad, id), NAD_AVAL(nad, id), bres->sm_request);
                    }
                    else
                    {
                        log_debug(ZONE, "got a response with no id, but we were expecting %s", bres->sm_request);
                    }

                    nad_free(nad);
                    return 0;
                }

                /* failed requests */
                if(nad_find_attr(nad, 1, ns, "failed", NULL) >= 0)
                {
                    /* handled request */
                    bres->sm_request[0] = '\0';

                    /* we only care about failed start and create */
                    if((NAD_AVAL_L(nad, action) == 5 && strncmp("start", NAD_AVAL(nad, action), 5) == 0) ||
                       (NAD_AVAL_L(nad, action) == 6 && strncmp("create", NAD_AVAL(nad, action), 6) == 0))
                    {

#if 0
                        /* create failed, so we need to remove them from authreg */
                        if(NAD_AVAL_L(nad, action) == 6 && c2s->ar->delete_user != NULL)
                        {
                            if((c2s->ar->delete_user)(c2s->ar, sess, bres->jid->node, sess->host->realm) != 0)
                                log_write(c2s->log, LOG_NOTICE, "[%d] user creation failed, and unable to delete user credentials: user=%s, realm=%s", sess->s->tag, bres->jid->node, sess->host->realm);
                            else
                                log_write(c2s->log, LOG_NOTICE, "[%d] user creation failed, so deleted user credentials: user=%s, realm=%s", sess->s->tag, bres->jid->node, sess->host->realm);
                        }
#endif
                        /* error the result and return it to the client */
                        sx_nad_write(sess->s, stanza_error(sess->result, 0, stanza_err_INTERNAL_SERVER_ERROR));
                        sess->result = NULL;

                        /* remove the bound resource */
                        if(bres == sess->resources)
                        {
                            sess->resources = bres->next;
                        }
                        else
                        {
                            for(ires = sess->resources; ires != NULL; ires = ires->next)
                                if(ires->next == bres)
                                    break;
                            assert(ires != NULL);
                            ires->next = bres->next;
                        }

                        jid_free(bres->jid);
                        free(bres);

                        nad_free(nad);
                        return 0;
                    }

                    log_debug(ZONE, "weird, got a failed session response, with a matching id, but the action is bogus *shrug*");

                    nad_free(nad);
                    return 0;
                }

                /* session started */
                if(NAD_AVAL_L(nad, action) == 7 && strncmp("started", NAD_AVAL(nad, action), 7) == 0)
                {
                    /* handled request */
                    bres->sm_request[0] = '\0';

                    /* copy the sm id */
                    if(smid >= 0)
                        snprintf(bres->sm_id, sizeof(bres->sm_id), "%.*s", NAD_AVAL_L(nad, smid), NAD_AVAL(nad, smid));

                    /* and remember the SM that services us */
                    from = nad_find_attr(nad, 0, -1, "from", NULL);


                    smcomp = (char*)malloc(NAD_AVAL_L(nad, from) + 1);
                    snprintf(smcomp, NAD_AVAL_L(nad, from) + 1, "%.*s", NAD_AVAL_L(nad, from), NAD_AVAL(nad, from));
                    sess->smcomp = smcomp;

                    nad_free(nad);

                    /* bring them online, old-skool */
                    if(!sess->sasl_authd && sess->s)
                    {
                        sx_auth(sess->s, "traditional", jid_full(bres->jid));
                        return 0;
                    }

                    if(sess->result)
                    {
                        /* return the auth result to the client */
                        if(sess->s) sx_nad_write(sess->s, sess->result);
                        /* or follow-up the session creation with cached presence packet */
                        else
                        {
                            log_debug(ZONE, "follow-up the session creation with cached presence packet,skip");
                            //sm_packet(sess, bres, sess->result);
                        }
                    }
                    sess->result = NULL;

                    /* we're good to go */
                    sess->active = 1;

                    return 0;
                }

                /* handled request */
                bres->sm_request[0] = '\0';

                log_debug(ZONE, "unknown action %.*s", NAD_AVAL_L(nad, id), NAD_AVAL(nad, id));

                nad_free(nad);

                return 0;
            }

            /* client packets */
            if(NAD_NURI_L(nad, NAD_ENS(nad, 1)) == strlen(uri_CLIENT) && strncmp(uri_CLIENT, NAD_NURI(nad, NAD_ENS(nad, 1)), strlen(uri_CLIENT)) == 0)
            {
                if(!sess->active || !sess->s)
                {
                    /* its a strange world .. */
                    log_debug(ZONE, "Got packet for %s - dropping", !sess->s ? "session without stream (PBX pipe session?)" : "inactive session");
                    nad_free(nad);
                    return 0;
                }

                /* sm is bouncing something */
                if(nad_find_attr(nad, 1, ns, "failed", NULL) >= 0)
                {
                    if(s)
                    {
                        /* there's really no graceful way to handle this */
                        sx_error(s, stream_err_INTERNAL_SERVER_ERROR, "session manager failed control action");
                        sx_close(s);
                    }

                    nad_free(nad);
                    return 0;
                }

                /* we're counting packets */
                sess->packet_count++;
                sess->c2s->packet_count++;

                /* remove sm specifics */
                nad_set_attr(nad, 1, ns, "c2s", NULL, 0);
                nad_set_attr(nad, 1, ns, "sm", NULL, 0);

                /* forget about the internal namespace too */
                if(nad->elems[1].ns == ns)
                    nad->elems[1].ns = nad->nss[ns].next;

                else
                {
                    for(scan = nad->elems[1].ns; nad->nss[scan].next != -1 && nad->nss[scan].next != ns; scan = nad->nss[scan].next);

                    /* got it */
                    if(nad->nss[scan].next != -1)
                        nad->nss[scan].next = nad->nss[ns].next;
                }

                sx_nad_write_elem(sess->s, nad, 1);

                return 0;
            }

            /* its something else */
            log_debug(ZONE, "unknown packet, dropping");

            nad_free(nad);
            return 0;

        case event_CLOSED:
            mio_close(c2s->mio, c2s->fd);
            c2s->fd = NULL;
            return -1;
    }

    return 0;
}

int c2s_router_mio_callback(mio_t m, mio_action_t a, mio_fd_t fd, void *data, void *arg)
{
    c2s_t c2s = (c2s_t) arg;
    int nbytes;

    switch(a)
    {
        case action_READ:
            log_debug(ZONE, "read action on fd %d", fd->fd);

            ioctl(fd->fd, FIONREAD, &nbytes);
            if(nbytes == 0)
            {
                sx_kill(c2s->router);
                return 0;
            }

            return sx_can_read(c2s->router);

        case action_WRITE:
            log_debug(ZONE, "write action on fd %d", fd->fd);
            return sx_can_write(c2s->router);

        case action_CLOSE:
            log_debug(ZONE, "close action on fd %d", fd->fd);
            log_write(c2s->log, LOG_NOTICE, "connection to router closed");

            c2s_lost_router = 1;

            /* we're offline */
            c2s->online = 0;

            break;

        case action_ACCEPT:
            break;
    }

    return 0;
}

// 解析imserver线程发过来的线程流
unsigned int ParseStream(char *pData, sess_t sess, int datalen)
{
    log_debug(ZONE, "ParseStream");

    XT_HEAD* head = NULL;
    unsigned int len = 0;

    head = (XT_HEAD*)pData;

    switch (head->cmd)
    {
        case CMD_LOGIN_ACK:
        {
            log_debug(ZONE, "ParseStream CMD_LOGIN_ACK");
            XT_LOGIN_ACK* login_ack = (XT_LOGIN_ACK*)(pData + sizeof(XT_HEAD));
            if(sess != NULL)
                len = SendXmlLogin(login_ack, sess);
        }
        break;
        /*
            case CMD_P2PMSG_ACK:
                {
                    XT_SERVER_P2PMSG_ACK* p2pmsg_ack = (XT_SERVER_P2PMSG_ACK*)(data->data + sizeof(XT_HEAD));
                    len = WriteP2PMsgSend(p2pmsg_ack, write, w_str);
                }
            break;
        */
        case CMD_SERVERMSG_IND:
        {
            log_debug(ZONE, "ParseStream CMD_SERVERMSG_IND");
            LPXT_SERVER_TRANSMIT server_transmit = (LPXT_SERVER_TRANSMIT)(pData + sizeof(XT_HEAD));
            switch(server_transmit->msgtype)
            {
                case CMD_P2PMSG_SEND:
                    len = SendXmlMessage(pData, sess);
                    break;
				case CMD_P2PMSG_SEND_OFFLINE:
					len = SendXmlOfflineMessage(pData, sess);
                /*
                case CMD_GET_FRIEND_BASICINFO_NOTIFY:
                    len = NotifyGetUserBasicInfo(m_RecvPacket, write, w_str);
                break;
                */
                default:
                    break;
            }
        }
        break;
        case CMD_USERINFO_GET_ACK:
        {
            SendXmlFullUserInfo(pData);
        }
        break;
        /*
                case CMD_USERINFO_MOD_ACK:
                    {
                        XT_USERINFO_MOD_ACK* pUserInfoModAck = (XT_USERINFO_MOD_ACK*)(data->data + sizeof(XT_HEAD));
                        len = WriteUserInfoMod(pUserInfoModAck, write, w_str);
                    }
                break;
                case CMD_GROUPINFO_GET_ACK:
                    {
                        XT_GROUPINFO_GET_ACK* pGroupInfoGetAck = (XT_GROUPINFO_GET_ACK*)(data->data + sizeof(XT_HEAD));
                        len = WriteGroupInfoGet(pGroupInfoGetAck, write, w_str);
                    }
                break;
                case CMD_FRIENDLIST_ACK:
                    {
                        len = WriteFriendListAck(m_RecvPacket, write, w_str);
                    }
                break;
                case CMD_DIRECTORY_ACK:
                    {
                        len = WriteHeartTick(m_RecvPacket, write, w_str);
                    }
                break;
                case CMD_GET_CLIST_ACK:
                    {
                        len = WriteClusterListAck(m_RecvPacket, write, w_str);
                    }
                break;
                case CMD_GET_CINFO_ACK:
                    {
                        len = WriteClusterInfoAck(m_RecvPacket, write, w_str);
                    }
                break;
                case CMD_WEB_MORE_MSG_ACK:
                    {
                        len = WriteWebMoreMsgAck(m_RecvPacket, write, w_str);
                    }
                break;
        */
        case CMD_XMPP_BUSI_LIST_ACK:
        {
            SendXmlWebList(pData);
        }
        break;

        case CMD_GET_SUB_ACCOUNT_ACK:
        {
            SendXmlFlow(pData);
        }
        break;
        case CMD_DIRECTORY_ACK:
        {
            SendXmlHeartBeat(pData);
        }
        break;
        /*
                case CMD_GET_SUB_ONLINE_ACK:
                    {
                        XT_GET_SUB_ONLINE_ACK* pAck = (XT_GET_SUB_ONLINE_ACK*)(data->data + sizeof(XT_HEAD));
                        len = WriteCheckSubOnline(pAck, write, w_str);
                    }
                break;

                case CMD_GET_USER_STATUS_ACK:
                    {
                        XT_GET_USER_STATUS_ACK *pAck = (XT_GET_USER_STATUS_ACK*)(data->data + sizeof(XT_HEAD));
                        len = WriteGetOnlineStatus(pAck, write, w_str);
                    }
                break;
            */
        case CMD_LOGOUT:
        {
            SendXmlTimeout(pData);
            break;
        }
        case CMD_HISTORY_MSG_ACK:
        {
            MsgList ml;
            if(ml.ParseFromArray(pData+sizeof(XT_HEAD), datalen-sizeof(XT_HEAD)))
            {
                // 反序列号成功
                log_debug(ZONE, "ParseStream CMD_HISTORY_MSG_ACK ParseFromArray success");
                SendMsgHistory(head->did, ml);
            }
            else
            {
                log_debug(ZONE, "ParseStream CMD_HISTORY_MSG_ACK ParseFromArray failed");
            }
        }
        break;
        default:
            break;
    }



    return len;
}

// 登录请求xml格式输出
unsigned int SendXmlLogin(XT_LOGIN_ACK* login_ack, sess_t sess)
{
#if 0
    if(login_ack->user_id != 0 )
    {
        sess->m_selfID = login_ack->user_id;    //sasl mode save userid    
       
		CRecvPacket plainPacket((char *)login_ack,(int)sizeof(XT_LOGIN_ACK));
		XT_LOGIN_ACK ack;
		plainPacket>>ack.ret;
		if(ack.ret == 0)
		{
			long data_len =	plainPacket.ReadPacket((char *)ack.session_key,16);

			if ( data_len!=16 )
			{				
				log_debug(ZONE, "parse sasl SendXmlLoginACK FAILED");
 				return -1;			
			}
			else
			{
			//	memcpy(m_SessionKey,ack.session_key,16);

				char key1[16], key2[16];

				plainPacket>>ack.user_id>>ack.merchant_id>>ack.publicip>>ack.version_flag;

			//	g_ImUserInfo.SetId(ack.user_id);
			//	g_ImUserInfo.merchant_id = ack.merchant_id;

			//	uint32 id = g_ImUserInfo.GetId();
			//	md5T.MD5Update ((unsigned char *)&id, sizeof(id));
			//	md5T.MD5Final ((unsigned char *)key1);

			//	md5T.MD5Update ((unsigned char*)m_SessionKey, sizeof(m_SessionKey));
			//	md5T.MD5Final ((unsigned char *)key2);

			//	for ( int i=0; i<16; i++ )
			//	{ 
			//		m_P2pKey[i] = key1[i]&key2[i];
			//	}

				//版本更新url
				plainPacket>>ack.version_url;
				plainPacket>>ack.update_config;
				plainPacket>>ack.update_info;
				plainPacket>>ack.needChangeAccount;
				plainPacket>>ack.szGUID;
				plainPacket>>ack.username;
			//	plainPacket>>ack.login_time;
			
				strcpy(sess->m_username,ack.username);
			}			
			log_debug(ZONE, "sasl connect set userid %u username %s",login_ack->user_id,ack.username);
		}
    }
#endif
    // 查找到以前的nad
    if(loginNodManager.find(sess->fd->fd) == loginNodManager.end())
    {
        log_debug(ZONE, "SendXmlLogin not find nad maybe sasl auth,skip ...");
        return -1;
    }
    /* create new_cplus bound jid holder */
    if(sess->resources == NULL)
    {
        sess->resources = (bres_t) calloc(1, sizeof(struct bres_st));
    }
    nad_t pAuthNad = loginNodManager[sess->fd->fd];

    int ns, elem, attr = 0;
    char username[1024], resource[1024], password[1024];
    ns = nad_find_scoped_namespace(pAuthNad, uri_AUTH, NULL);

    /* sort out the username */
    elem = nad_find_elem(pAuthNad, 1, ns, "username", 1);
    if(elem < 0)
    {
        log_debug(ZONE, "auth set with no username, bouncing it");

        sx_nad_write(sess->s, stanza_tofrom(stanza_error(pAuthNad, 0, stanza_err_BAD_REQUEST), 0));
        nad_free(pAuthNad);
        loginNodManager.erase(loginNodManager.find(sess->fd->fd));

        return -1;
    }

    snprintf(username, 1024, "%.*s", NAD_CDATA_L(pAuthNad, elem), NAD_CDATA(pAuthNad, elem));


    /* make sure we have the resource */
    elem = nad_find_elem(pAuthNad, 1, ns, "resource", 1);
    if(elem < 0)
    {
        log_debug(ZONE, "auth set with no resource, bouncing it");

        sx_nad_write(sess->s, stanza_tofrom(stanza_error(pAuthNad, 0, stanza_err_BAD_REQUEST), 0));
        nad_free(pAuthNad);
        loginNodManager.erase(loginNodManager.find(sess->fd->fd));

        return -1;
    }

    snprintf(resource, 1024, "%.*s", NAD_CDATA_L(pAuthNad, elem), NAD_CDATA(pAuthNad, elem));

    elem = nad_find_elem(pAuthNad, 1, ns, "password", 1);
    if(elem < 0)
    {
        log_debug(ZONE, "auth set with no password, bouncing it");

        sx_nad_write(sess->s, stanza_tofrom(stanza_error(pAuthNad, 0, stanza_err_BAD_REQUEST), 0));
        nad_free(pAuthNad);
        loginNodManager.erase(loginNodManager.find(sess->fd->fd));

        return -1;
    }

    snprintf(password, 1024, "%.*s", NAD_CDATA_L(pAuthNad, elem), NAD_CDATA(pAuthNad, elem));

    /* our local id */
    sprintf(sess->resources->c2s_id, "%d", sess->s->tag);

    /* the full user jid for this session */
    sess->resources->jid = jid_new(sess->s->req_to, -1);
    jid_reset_components(sess->resources->jid, username, sess->resources->jid->domain, resource);

    log_write(sess->c2s->log, LOG_NOTICE, "[%d] requesting session: jid=%s", sess->s->tag, jid_full(sess->resources->jid));

    /* build a result packet, we'll send this back to the client after we have a session for them */
    sess->result = nad_new();


    if(login_ack->ret == 0)
    {
        ns = nad_add_namespace(sess->result, uri_CLIENT, NULL);

        nad_append_elem(sess->result, ns, "iq", 0);
        nad_set_attr(sess->result, 0, -1, "type", "result", 6);
        char szID[64] = {0};
        sprintf_s(szID, "%d", login_ack->user_id);
        nad_set_attr(sess->result, 0, -1, "to", szID, strlen(szID));


        // 返回id
        //nad_append_elem(sess->result, ns, "userid", 1);
        //nad_append_cdata_gbk(sess->result,szID, strlen(szID), 2);

        sess->m_selfID = login_ack->user_id;
        map<uint32,sess_t>::iterator sessold = sessManager.find(login_ack->user_id);
        if(sessold != sessManager.end())
        {
            log_debug(ZONE, "map erase old fd");
            mio_close(sessold->second->c2s->mio, sessold->second->fd);
            sessold->second->fd = NULL;
            sessManager.erase(login_ack->user_id);
        }
        sessManager.insert(pair<uint32,sess_t>(login_ack->user_id,sess));
        log_debug(ZONE, "add map userid %u --- > fd %u",login_ack->user_id,sess->fd->fd);

        if(pAuthNad != NULL)
        {
            int attr = nad_find_attr(pAuthNad, 0, -1, "id", NULL);
            if(attr >= 0)
                nad_set_attr(sess->result, 0, -1, "id", NAD_AVAL(pAuthNad, attr), NAD_AVAL_L(pAuthNad, attr));

            //nad_free(pAuthNad);
        }

        // 添加query字段
        nad_append_elem(sess->result, ns, "query", 1);
        nad_append_attr(sess->result, -1, "xmlns", uri_AUTH);

        nad_append_elem(sess->result, ns, "resource", 2);
        nad_append_cdata_gbk(sess->result,resource, strlen(resource), 3);

        nad_append_elem(sess->result, ns, "username", 2);
        nad_append_cdata_gbk(sess->result, username, strlen(username), 3);

        nad_append_elem(sess->result, ns, "userid", 2);
        nad_append_cdata_gbk(sess->result,szID, strlen(szID), 3);

        /* start a session with the sm */
        //sm_start(sess, sess->resources);
        sx_nad_write(sess->s, sess->result);
        sess->result = NULL;
    }
    else
    {
        /* auth failed, so error */
        ns = nad_add_namespace(sess->result, uri_CLIENT, NULL);

        nad_append_elem(sess->result, ns, "iq", 0);
        nad_set_attr(sess->result, 0, -1, "type", "error", 5);
        nad_set_attr(sess->result, 0, -1, "to", username, strlen(username));

        // 返回id
        //nad_append_elem(sess->result, ns, "userid", 1);
        //nad_append_cdata_gbk(sess->result,szID, strlen(szID), 2);

        sess->m_selfID = login_ack->user_id;
        map<uint32,sess_t>::iterator sessold = sessManager.find(login_ack->user_id);
        if(sessold != sessManager.end())
        {
            log_debug(ZONE, "map erase old fd");
            mio_close(sessold->second->c2s->mio, sessold->second->fd);
            sessold->second->fd = NULL;
            sessManager.erase(login_ack->user_id);
        }
        sessManager.insert(pair<uint32,sess_t>(login_ack->user_id,sess));
        log_debug(ZONE, "add map userid %u --- > fd %u",login_ack->user_id,sess->fd->fd);

        if(pAuthNad != NULL)
        {
            int attr = nad_find_attr(pAuthNad, 0, -1, "id", NULL);
            if(attr >= 0)
                nad_set_attr(sess->result, 0, -1, "id", NAD_AVAL(pAuthNad, attr), NAD_AVAL_L(pAuthNad, attr));

            //nad_free(pAuthNad);
        }

        // 添加query字段
        nad_append_elem(sess->result, ns, "query", 1);
        nad_append_attr(sess->result, -1, "xmlns", uri_AUTH);

        nad_append_elem(sess->result, ns, "resource", 2);
        nad_append_cdata_gbk(sess->result,resource, strlen(resource), 3);

        nad_append_elem(sess->result, ns, "username", 2);
        nad_append_cdata_gbk(sess->result, username, strlen(username), 3);

        nad_append_elem(sess->result, ns, "password", 2);
        nad_append_cdata_gbk(sess->result,password, strlen(password), 3);

        nad_append_elem(sess->result, ns, "errorcode", 2);
        nad_append_cdata_gbk(sess->result,"1", 1, 3);

        nad_append_elem(sess->result, ns, "errordesc", 2);
        nad_append_cdata_gbk(sess->result,"账号或密码错误", strlen("账号或密码错误"), 3);

        // error
        nad_append_elem(sess->result, ns, "error", 1);
        nad_append_attr(sess->result, -1, "type", "cancel");

        /* start a session with the sm */
        //sm_start(sess, sess->resources);
        sx_nad_write(sess->s, sess->result);
        sess->result = NULL;

        //nad_free(pAuthNad);
        loginNodManager.erase(loginNodManager.find(sess->fd->fd));

        return -1;
    }

    nad_free(pAuthNad);
    loginNodManager.erase(loginNodManager.find(sess->fd->fd));

    return 0;
}

// 聊天消息转发
unsigned int SendXmlMessage(char* data, sess_t sess)
{
    XT_MSG msg;
    uint8 weight;
    uint8 style;
    uint8 line;
    char nickName[32] = {0};
    CRecvPacket notifyPacket;

    LPXT_SERVER_TRANSMIT transmit = (LPXT_SERVER_TRANSMIT)(data + sizeof(XT_HEAD));
    XT_SERVER_P2PMSG_SEND* p2pMsg_send = (XT_SERVER_P2PMSG_SEND*)(data + sizeof(XT_HEAD) + sizeof(XT_SERVER_TRANSMIT));
    notifyPacket.SetData(data + sizeof(XT_HEAD) + sizeof(XT_SERVER_TRANSMIT), MAX_SEND_BUFFERS - sizeof(XT_HEAD) - sizeof(XT_SERVER_TRANSMIT));

    notifyPacket>>msg.from_id
                >>msg.to_id
                >>msg.data_type
                >>msg.ver
                >>msg.send_time
                >>msg.recv_flag;

    if (msg.data_type==XT_MSG::IM_MSG
        //|| msg.data_type==XT_MSG::EVALUA_ACK
        || msg.data_type==XT_MSG::CUST_SER_TRAN_REQ
        || msg.data_type==XT_MSG::AUTO_REPLY
        /*|| msg.data_type==XT_MSG::IM_MSG_INSERT*/)
    {
        notifyPacket>>msg.fontSize
                    >>msg.fontColor
                    >>msg.fontStyle
                    >>msg.fontName;
    }

    weight = msg.fontStyle & 0x04;
    style = msg.fontStyle & 0x01;
    line = msg.fontStyle & 0x02;

    if(weight == 0x04)
        weight = 1;
    else
        weight = 0;

    if(style == 0x01)
        style = 1;
    else
        style = 0;

    if(line == 0x02)
        line = 1;
    else
        line = 0;


    map<uint32,sess_t>::iterator it = sessManager.find(msg.to_id);
    if(it != sessManager.end())
    {
        sess = it->second;
        log_debug(ZONE, "SendXmlMessage find sess");
    }
    else
    {
        log_debug(ZONE, "SendXmlMessage not find sess");
        return -1;
    }

    // 构造返回消息
    /* build a result packet, we'll send this back to the client after we have a session for them */
    sess->result = nad_new();

    int ns = nad_add_namespace(sess->result, uri_CLIENT, NULL);

    nad_append_elem(sess->result, ns, "message", 0);
    char szTemp[1024]= {0};

    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", p2pMsg_send->Sender);
    nad_append_attr(sess->result, ns, "from", szTemp);

    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", p2pMsg_send->Recver);
    nad_append_attr(sess->result, ns, "to", szTemp);
    /*
           nad_append_elem(sess->result, ns, "origin_id", 1);
           sprintf_s(szTemp, "%d", transmit->origin_id);
           nad_append_cdata_gbk(sess->result, szTemp, sizeof(szTemp), 2);

           nad_append_elem(sess->result, ns, "msgtype", 1);
           memset(szTemp, 0, 1024);
           sprintf_s(szTemp, "%d", transmit->msgtype);
           nad_append_cdata_gbk(sess->result,szTemp, sizeof(szTemp), 2);
    */
    nad_append_elem(sess->result, ns, "fnext", 1);
    nad_append_attr(sess->result, -1, "xmlns", "jabber:message");

    nad_append_elem(sess->result, ns, "from", 2);
    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", p2pMsg_send->Sender);
    nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

    nad_append_elem(sess->result, ns, "to", 2);
    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", p2pMsg_send->Recver);
    nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

    nad_append_elem(sess->result, ns, "datatype", 2);
    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", msg.data_type);
    nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);
    /*
        nad_append_elem(sess->result, ns, "font", 1);

        nad_append_elem(sess->result, ns, "fontname", 2);
            memset(szTemp, 0, 1024);
            sprintf_s(szTemp, "%s", msg.fontName);
            nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

            nad_append_elem(sess->result, ns, "color", 2);
            memset(szTemp, 0, 1024);
            sprintf_s(szTemp, "0x%06X", msg.fontColor);
            nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

            nad_append_elem(sess->result, ns, "size", 2);
            memset(szTemp, 0, 1024);
            sprintf_s(szTemp, "%d", msg.fontSize);
            nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

            nad_append_elem(sess->result, ns, "bold", 2);
            memset(szTemp, 0, 1024);
            sprintf_s(szTemp, "%d", weight);
            nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

            nad_append_elem(sess->result, ns, "italic", 2);
            memset(szTemp, 0, 1024);
            sprintf_s(szTemp, "%d", style);
            nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

            nad_append_elem(sess->result, ns, "underline", 2);
            memset(szTemp, 0, 1024);
            sprintf_s(szTemp, "%d", line);
            nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);
    */

    nad_append_elem(sess->result, ns, "nickname", 2);
    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%s", nickName);
    nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

    nad_append_elem(sess->result, ns, "sendtime", 2);
    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", msg.send_time);
    nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

    notifyPacket.ReadData(msg.data,sizeof(msg.data));

    if ( msg.data_type==XT_MSG::IM_MSG
         /*|| msg.data_type==XT_MSG::EVALUA_ACK
         || msg.data_type==XT_MSG::OPEN_P2PSENDMSG_DLG
         || msg.data_type==XT_MSG::CUST_SER_TRAN_REQ*/
         || msg.data_type==XT_MSG::AUTO_REPLY
         /*|| msg.data_type==XT_MSG::IM_MSG_INSERT*/)

        notifyPacket>>msg.from_nickname;
    nad_append_elem(sess->result, ns, "fromnickname", 2);
    nad_append_cdata_gbk(sess->result,msg.from_nickname, strlen(msg.from_nickname), 3);

    notifyPacket>>msg.uuid;
    nad_append_elem(sess->result, ns, "uuid", 2);
    nad_append_cdata_gbk(sess->result,msg.uuid, strlen(msg.uuid), 3);

    int nNewNs = nad_append_elem(sess->result, ns, "subject", 1);
    nad_append_attr(sess->result, ns, "xml:lang", "cn");
    nad_append_cdata_gbk(sess->result,"chat title", 10, 2);



    if(msg.data_type == 7)
    {
        nad_append_elem(sess->result, ns, "body", 1);
        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%s", "recomment");
        nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 2);
    }
    else
    {
        nad_append_elem(sess->result, ns, "body", 1);
        nad_append_cdata_gbk(sess->result,msg.data, strlen(msg.data), 2);
    }

    sx_nad_write(sess->s, sess->result);
    sess->result = NULL;

    return 0;
}


unsigned int SendXmlOfflineMessage(char* data, sess_t sess)
{
	XT_MSG msg;
	uint8 weight;
	uint8 style;
	uint8 line;
	char nickName[32] = {0};
	CRecvPacket notifyPacket;

	LPXT_SERVER_TRANSMIT transmit = (LPXT_SERVER_TRANSMIT)(data + sizeof(XT_HEAD));
	XT_SERVER_P2PMSG_SEND* p2pMsg_send = (XT_SERVER_P2PMSG_SEND*)(data + sizeof(XT_HEAD) + sizeof(XT_SERVER_TRANSMIT));
	notifyPacket.SetData(data + sizeof(XT_HEAD) + sizeof(XT_SERVER_TRANSMIT), MAX_SEND_BUFFERS - sizeof(XT_HEAD) - sizeof(XT_SERVER_TRANSMIT));

	notifyPacket>>msg.from_id
				>>msg.to_id
				>>msg.data_type
				>>msg.ver
				>>msg.send_time
				>>msg.recv_flag;

	if (msg.data_type==XT_MSG::IM_MSG
		//|| msg.data_type==XT_MSG::EVALUA_ACK
		|| msg.data_type==XT_MSG::CUST_SER_TRAN_REQ
		|| msg.data_type==XT_MSG::AUTO_REPLY
		/*|| msg.data_type==XT_MSG::IM_MSG_INSERT*/)
	{
		notifyPacket>>msg.fontSize
					>>msg.fontColor
					>>msg.fontStyle
					>>msg.fontName;
	}

	weight = msg.fontStyle & 0x04;
	style = msg.fontStyle & 0x01;
	line = msg.fontStyle & 0x02;

	if(weight == 0x04)
		weight = 1;
	else
		weight = 0;

	if(style == 0x01)
		style = 1;
	else
		style = 0;

	if(line == 0x02)
		line = 1;
	else
		line = 0;


	map<uint32,sess_t>::iterator it = sessManager.find(msg.to_id);
	if(it != sessManager.end())
	{
		sess = it->second;
		log_debug(ZONE, "SendXmlOfflineMessage find sess");
	}
	else
	{
		log_debug(ZONE, "SendXmlOfflineMessage not find sess");
		return -1;
	}

	// 构造返回消息
	/* build a result packet, we'll send this back to the client after we have a session for them */
	sess->result = nad_new();

	int ns = nad_add_namespace(sess->result, uri_CLIENT, NULL);

	nad_append_elem(sess->result, ns, "message", 0);
	char szTemp[1024]= {0};

	memset(szTemp, 0, 1024);
	sprintf_s(szTemp, "%d", p2pMsg_send->Sender);
	nad_append_attr(sess->result, ns, "from", szTemp);

	memset(szTemp, 0, 1024);
	sprintf_s(szTemp, "%d", p2pMsg_send->Recver);
	nad_append_attr(sess->result, ns, "to", szTemp);
	/*
		   nad_append_elem(sess->result, ns, "origin_id", 1);
		   sprintf_s(szTemp, "%d", transmit->origin_id);
		   nad_append_cdata_gbk(sess->result, szTemp, sizeof(szTemp), 2);

		   nad_append_elem(sess->result, ns, "msgtype", 1);
		   memset(szTemp, 0, 1024);
		   sprintf_s(szTemp, "%d", transmit->msgtype);
		   nad_append_cdata_gbk(sess->result,szTemp, sizeof(szTemp), 2);
	*/
	nad_append_elem(sess->result, ns, "delay", 1);
	nad_append_attr(sess->result, -1, "xmlns", "urn:xmpp:delay");

	nad_append_elem(sess->result, ns, "from", 2);
	memset(szTemp, 0, 1024);
	sprintf_s(szTemp, "%d", p2pMsg_send->Sender);
	nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

	nad_append_elem(sess->result, ns, "to", 2);
	memset(szTemp, 0, 1024);
	sprintf_s(szTemp, "%d", p2pMsg_send->Recver);
	nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

	nad_append_elem(sess->result, ns, "datatype", 2);
	memset(szTemp, 0, 1024);
	sprintf_s(szTemp, "%d", msg.data_type);
	nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);
	/*
		nad_append_elem(sess->result, ns, "font", 1);

		nad_append_elem(sess->result, ns, "fontname", 2);
			memset(szTemp, 0, 1024);
			sprintf_s(szTemp, "%s", msg.fontName);
			nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

			nad_append_elem(sess->result, ns, "color", 2);
			memset(szTemp, 0, 1024);
			sprintf_s(szTemp, "0x%06X", msg.fontColor);
			nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

			nad_append_elem(sess->result, ns, "size", 2);
			memset(szTemp, 0, 1024);
			sprintf_s(szTemp, "%d", msg.fontSize);
			nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

			nad_append_elem(sess->result, ns, "bold", 2);
			memset(szTemp, 0, 1024);
			sprintf_s(szTemp, "%d", weight);
			nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

			nad_append_elem(sess->result, ns, "italic", 2);
			memset(szTemp, 0, 1024);
			sprintf_s(szTemp, "%d", style);
			nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

			nad_append_elem(sess->result, ns, "underline", 2);
			memset(szTemp, 0, 1024);
			sprintf_s(szTemp, "%d", line);
			nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);
	*/

	nad_append_elem(sess->result, ns, "nickname", 2);
	memset(szTemp, 0, 1024);
	sprintf_s(szTemp, "%s", nickName);
	nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

	nad_append_elem(sess->result, ns, "stamp", 2);
	memset(szTemp, 0, 1024);
	sprintf_s(szTemp, "%d", msg.send_time);
	nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

	notifyPacket.ReadData(msg.data,sizeof(msg.data));

	if ( msg.data_type==XT_MSG::IM_MSG
		 /*|| msg.data_type==XT_MSG::EVALUA_ACK
		 || msg.data_type==XT_MSG::OPEN_P2PSENDMSG_DLG
		 || msg.data_type==XT_MSG::CUST_SER_TRAN_REQ*/
		 || msg.data_type==XT_MSG::AUTO_REPLY
		 /*|| msg.data_type==XT_MSG::IM_MSG_INSERT*/)

		notifyPacket>>msg.from_nickname;
	nad_append_elem(sess->result, ns, "fromnickname", 2);
	nad_append_cdata_gbk(sess->result,msg.from_nickname, strlen(msg.from_nickname), 3);

	notifyPacket>>msg.uuid;
	nad_append_elem(sess->result, ns, "uuid", 2);
	nad_append_cdata_gbk(sess->result,msg.uuid, strlen(msg.uuid), 3);

	int nNewNs = nad_append_elem(sess->result, ns, "subject", 1);
	nad_append_attr(sess->result, ns, "xml:lang", "cn");
	nad_append_cdata_gbk(sess->result,"chat title", 10, 2);



	if(msg.data_type == 7)
	{
		nad_append_elem(sess->result, ns, "body", 1);
		memset(szTemp, 0, 1024);
		sprintf_s(szTemp, "%s", "recomment");
		nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 2);
	}
	else
	{
		nad_append_elem(sess->result, ns, "body", 1);
		nad_append_cdata_gbk(sess->result,msg.data, strlen(msg.data), 2);
	}

	sx_nad_write(sess->s, sess->result);
	sess->result = NULL;

	return 0;
}

// 获取完整信息
unsigned int SendXmlFullUserInfo(char* data)
{
    XT_HEAD* head = NULL;
    head = (XT_HEAD*)data;

    CRecvPacket notifyPacket;
    notifyPacket.SetData(data + sizeof(XT_HEAD), MAX_SEND_BUFFERS - sizeof(XT_HEAD));

    uint8   rlt;
    uint32  nClientID;
    XT_SERVER_USERINFO_GET_ACK UserInfo;

    notifyPacket >> rlt;        //操作结果
    if(rlt == 0)
        notifyPacket>> nClientID
                    >> UserInfo.username
                    >> UserInfo.nickname
                    >> UserInfo.email
                    >> UserInfo.birthday
                    >> UserInfo.sex
                    >> UserInfo.province
                    >> UserInfo.country
                    >> UserInfo.city
                    >> UserInfo.face_url
                    >> UserInfo.career
                    >> UserInfo.rule
                    >> UserInfo.familyflag
                    >> UserInfo.gold_money
                    >> UserInfo.online_time
                    >> UserInfo.linkpop
                    >> UserInfo.address
                    >> UserInfo.mobile
                    >> UserInfo.description
                    >> UserInfo.usersign
                    >> UserInfo.avatoruri
                    >> UserInfo.mobilestatus
                    >> UserInfo.integral;

    // 查找sess
    sess_t sess = NULL;
    map<uint32,sess_t>::iterator it = sessManager.find(head->did);
    if(it != sessManager.end())
    {
        sess = it->second;
        log_debug(ZONE, "SendXmlFullUserInfo find sess");
    }
    else
    {
        log_debug(ZONE, "SendXmlFullUserInfo not find sess");
        return -1;
    }

    // 构造返回消息
    /* build a result packet, we'll send this back to the client after we have a session for them */
    sess->result = nad_new();

    int ns = /*nad_add_namespace(sess->result, uri_USERINFO, NULL)*/-1;
    nad_append_elem(sess->result, ns, "iq", 0);
    nad_append_attr(sess->result, ns, "id","userinfo");

    if(rlt != 0)
    {
        nad_append_attr(sess->result, ns, "type","error");
    }
    else
    {
        nad_append_attr(sess->result, ns, "type","result");
    }

    char szTemp[1024]= {0};

    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", head->did);
    nad_append_attr(sess->result, ns, "to",szTemp);

    nad_append_elem(sess->result, ns, "query", 1);
    nad_append_attr(sess->result, -1, "xmlns", uri_USERINFO);
    if(rlt != 0)
    {
        nad_append_elem(sess->result, ns, "errorcode", 2);
        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", rlt);
        nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

        notifyPacket>>szTemp;
        nad_append_elem(sess->result, ns, "errordesc", 2);
        nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

        nad_append_elem(sess->result, ns, "error", 1);
        nad_append_cdata_gbk(sess->result,"cancel", strlen("cancel"), 2);
    }
    else
    {
        nad_append_elem(sess->result, ns, "clientid", 2);
        nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

        nad_append_elem(sess->result, ns, "username", 2);
        nad_append_cdata_gbk(sess->result,UserInfo.username, strlen(UserInfo.username), 3);

        nad_append_elem(sess->result, ns, "nickname", 2);
        nad_append_cdata_gbk(sess->result,UserInfo.nickname, strlen(UserInfo.nickname), 3);

        nad_append_elem(sess->result, ns, "email", 2);
        nad_append_cdata_gbk(sess->result,UserInfo.email, strlen(UserInfo.email), 3);

        nad_append_elem(sess->result, ns, "birthday", 2);
        nad_append_cdata_gbk(sess->result,UserInfo.birthday, strlen(UserInfo.birthday), 3);

        nad_append_elem(sess->result, ns, "sex", 2);
        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", UserInfo.sex);
        nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

        nad_append_elem(sess->result, ns, "province", 2);
        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", UserInfo.province);
        nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

        nad_append_elem(sess->result, ns, "country", 2);
        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", UserInfo.country);
        nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

        nad_append_elem(sess->result, ns, "city", 2);
        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", UserInfo.city);
        nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

        nad_append_elem(sess->result, ns, "face_url", 2);
        nad_append_cdata_gbk(sess->result,UserInfo.face_url, strlen(UserInfo.face_url), 3);

        nad_append_elem(sess->result, ns, "career", 2);
        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", UserInfo.career);
        nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

        nad_append_elem(sess->result, ns, "rule", 2);
        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", UserInfo.rule);
        nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

        nad_append_elem(sess->result, ns, "familyflag", 2);
        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", UserInfo.familyflag);
        nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

        nad_append_elem(sess->result, ns, "gold_money", 2);
        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", UserInfo.gold_money);
        nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

        nad_append_elem(sess->result, ns, "online_time", 2);
        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", UserInfo.online_time);
        nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

        nad_append_elem(sess->result, ns, "linkpop", 2);
        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", UserInfo.linkpop);
        nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

        nad_append_elem(sess->result, ns, "address", 2);
        nad_append_cdata_gbk(sess->result,UserInfo.address, strlen(UserInfo.address), 3);

        nad_append_elem(sess->result, ns, "mobile", 2);
        nad_append_cdata_gbk(sess->result,UserInfo.mobile, strlen(UserInfo.mobile), 3);

        nad_append_elem(sess->result, ns, "description", 2);
        nad_append_cdata_gbk(sess->result,UserInfo.description, strlen(UserInfo.description), 3);

        nad_append_elem(sess->result, ns, "usersign", 2);
        nad_append_cdata_gbk(sess->result,UserInfo.usersign, strlen(UserInfo.usersign), 3);

        nad_append_elem(sess->result, ns, "avatoruri", 2);
        nad_append_cdata_gbk(sess->result,UserInfo.avatoruri, strlen(UserInfo.avatoruri), 3);

        nad_append_elem(sess->result, ns, "mobilestatus", 2);
        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", UserInfo.mobilestatus);
        nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

        nad_append_elem(sess->result, ns, "integral", 2);
        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", UserInfo.integral);
        nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);
    }

    sx_nad_write(sess->s, sess->result);
    sess->result = NULL;

    return 0;
}

// 获取完整信息
unsigned int SendXmlFlow(char* data)
{
    XT_HEAD* head = NULL;
    head = (XT_HEAD*)data;

    CRecvPacket notifyPacket;
    notifyPacket.SetData(data + sizeof(XT_HEAD), MAX_SEND_BUFFERS - sizeof(XT_HEAD));

    /*返回子帐号基本信息*/
    uint32 id = 0;
    char    strEmail[MAX_EMAIL_LEN+1] = {0};
    char    strUsername[MAX_USERNAME_LEN+1] = {0};  // 用户名
    char    strNickname[MAX_NICKNAME_LEN+1] = {0};  // 呢称
    char    strMobile[MAX_PHONE_LEN+1] = {0};           // 手机
    char    usersign[MAX_USERSIGN_LEN+1] = {0};     //个性签名
    uint32  merchantid = 0;

    notifyPacket >> id
                 >> strEmail
                 >> strUsername
                 >> strMobile
                 >> strNickname
                 >> usersign
                 >> merchantid;     // 移动端需要返回merchantid add by sd

    // 解析结果
    log_debug(ZONE, "SendXmlFlow recv:id:%d, email:%s, username:%s, mobile:%s, nickname:%s, usersign:%s, merchantid:%d"
              , id, strEmail, strUsername, strMobile, strNickname, usersign, merchantid);

    // 查找sess
    sess_t sess = NULL;
    map<uint32,sess_t>::iterator it = sessManager.find(head->did);
    if(it != sessManager.end())
    {
        sess = it->second;
        log_debug(ZONE, "SendXmlFlow find sess");
    }
    else
    {
        log_debug(ZONE, "SendXmlFlow not find sess");
        return -1;
    }

    // 构造返回消息
    /* build a result packet, we'll send this back to the client after we have a session for them */
    sess->result = nad_new();

    int ns = /*nad_add_namespace(sess->result, uri_FLOW, NULL)*/-1;
    nad_append_elem(sess->result, ns, "iq", 0);
    nad_append_attr(sess->result, ns, "id","flow");
    nad_append_attr(sess->result, ns, "type","result");

    char szTemp[1024]= {0};
    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", head->did);
    nad_append_attr(sess->result, ns, "to",szTemp);

    nad_append_elem(sess->result, ns, "query", 1);
    nad_append_attr(sess->result, -1, "xmlns", uri_FLOW);

    nad_append_elem(sess->result, ns, "userid", 2);
    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", id);
    nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

    nad_append_elem(sess->result, ns, "queueLength", 2);
    nad_append_cdata_gbk(sess->result,"0", 1, 3);

    nad_append_elem(sess->result, ns, "username", 2);
    nad_append_cdata_gbk(sess->result,strUsername, strlen(strUsername), 3);

    nad_append_elem(sess->result, ns, "mobile", 2);
    nad_append_cdata_gbk(sess->result,strMobile, strlen(strMobile), 3);

    nad_append_elem(sess->result, ns, "nickname", 2);
    nad_append_cdata_gbk(sess->result,strNickname, strlen(strNickname), 3);

    nad_append_elem(sess->result, ns, "usersign", 2);
    nad_append_cdata_gbk(sess->result,usersign, strlen(usersign), 3);

    nad_append_elem(sess->result, ns, "merchantid", 2);
    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", merchantid);
    nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

    sx_nad_write(sess->s, sess->result);
    sess->result = NULL;

    return 0;
}

// 获取最近联系商家
unsigned int SendXmlWebList(char* data)
{
    XT_HEAD* head = NULL;
    head = (XT_HEAD*)data;

    CRecvPacket notifyPacket;
    notifyPacket.SetData(data + sizeof(XT_HEAD), MAX_SEND_BUFFERS - sizeof(XT_HEAD));

    uint32 id,msgnum,actualnum,state;
    uint8 ret;
    XT_XMPP_BUSI_LIST_ACK::BusiList ListInfo;


    // 查找sess
    sess_t sess = NULL;
    map<uint32,sess_t>::iterator it = sessManager.find(head->did);
    if(it != sessManager.end())
    {
        sess = it->second;
        log_debug(ZONE, "SendXmlWebList find sess");
    }
    else
    {
        log_debug(ZONE, "SendXmlWebList not find sess");
        return -1;
    }

    // 构造返回xml
    sess->result = nad_new();

    int ns = nad_add_namespace(sess->result, uri_LATESTMERCHANT, NULL);
    nad_append_elem(sess->result, ns, "iq", 0);
    nad_append_attr(sess->result, ns, "id","latestmerchant");
    nad_append_attr(sess->result, ns, "type","result");
	
    nad_append_elem(sess->result, ns, "query", 1);
    nad_append_attr(sess->result, -1, "xmlns", uri_LATESTMERCHANT);

    notifyPacket>>ret
                >>id
                >>msgnum
                >>actualnum;

    char szTemp[1024]= {0};
    nad_append_elem(sess->result, ns, "userid", 2);
    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", id);
    nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

    nad_append_elem(sess->result, ns, "msgnum", 2);
    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", msgnum);
    nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

    nad_append_elem(sess->result, ns, "actualnum", 2);
    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", actualnum);
    nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp),3);

    nad_append_elem(sess->result, ns, "merchantlist", 2);
    if(ret == RESULT_SUCCESS)
    {
        for(size_t i = 0; i < actualnum; i++)
        {
            notifyPacket>>ListInfo.id
						>>ListInfo.merchantId
                        >>ListInfo.nickname
                        >>ListInfo.time
                        >>ListInfo.state
                        >>ListInfo.headImgUrl
                        >>ListInfo.usersign
                        >>ListInfo.storename
						>>ListInfo.subCount;					
		
            if(ListInfo.state == XTREAM_OFFLINE)
                state = 1;
            else
                state = 0;

            nad_append_elem(sess->result, ns, "MerchantItem", 3);
			
			memset(szTemp, 0, 1024);
            sprintf_s(szTemp, "%d", ListInfo.id);
            nad_append_attr(sess->result, -1, "submerchantid", szTemp);

            memset(szTemp, 0, 1024);
            sprintf_s(szTemp, "%d", ListInfo.merchantId);
            nad_append_attr(sess->result, -1, "merchantid", szTemp);
			
			char gbkNickname[MAX_NICKNAME_LEN * 3] = {0};
			char gbkUsersign[MAX_USERSIGN_LEN * 3] = {0};			
	
			gbk2utf8(gbkNickname,ListInfo.nickname);
			gbk2utf8(gbkUsersign,ListInfo.usersign);
			

            nad_append_attr(sess->result, -1, "nickname", gbkNickname);

            memset(szTemp, 0, 1024);
            sprintf_s(szTemp, "%d", ListInfo.time);
            nad_append_attr(sess->result, -1, "time", szTemp);

            memset(szTemp, 0, 1024);
            sprintf_s(szTemp, "%d", state);
            nad_append_attr(sess->result, -1, "state", szTemp);
            nad_append_attr(sess->result, -1, "headImgUrl", ListInfo.headImgUrl);
            nad_append_attr(sess->result, -1, "usersign", gbkUsersign);			
            nad_append_attr(sess->result, -1, "storename", ListInfo.storename);
			nad_append_elem(sess->result, ns, "subMerchantList", 4);

			for(uint32 subCount = 0 ; subCount < ListInfo.subCount ; subCount++)
			{
				uint32 tmpSubAccount = 0;
				notifyPacket >> tmpSubAccount;
				nad_append_elem(sess->result, ns, "subMerchantItem", 5);
				memset(szTemp, 0, 1024);
    			sprintf_s(szTemp, "%d", tmpSubAccount);
            	nad_append_attr(sess->result, -1, "subMerchantid", szTemp);
			}			
        }
    }

    sx_nad_write(sess->s, sess->result);
    sess->result = NULL;

    return 0;
}

// 心跳
unsigned int SendXmlHeartBeat(char* data)
{
    XT_HEAD* head = NULL;
    head = (XT_HEAD*)data;

    CRecvPacket notifyPacket;
    notifyPacket.SetData(data + sizeof(XT_HEAD), MAX_SEND_BUFFERS - sizeof(XT_HEAD));

    XT_DIRECTORY_ACK ack;

    // 查找sess
    sess_t sess = NULL;
    map<uint32,sess_t>::iterator it = sessManager.find(head->did);
    if(it != sessManager.end())
    {
        sess = it->second;
        log_debug(ZONE, "SendXmlHeartBeat find sess");
    }
    else
    {
        log_debug(ZONE, "SendXmlHeartBeat not find sess");
        return -1;
    }

    // 构造返回xml
    sess->result = nad_new();

    notifyPacket>>ack.ret;

    int ns = /*nad_add_namespace(sess->result, uri_FLOW, NULL)*/-1;
    nad_append_elem(sess->result, ns, "iq", 0);
    nad_append_attr(sess->result, ns, "id","c2s1");
    nad_append_attr(sess->result, ns, "type","result");

    config_elem_t elem;
    elem = config_get(it->second->c2s->config, "local.id");
    if(elem != NULL)
    {
        nad_append_attr(sess->result, ns, "from",*(elem->values));
    }

    char szTemp[1024]= {0};
    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", head->did);
    nad_append_attr(sess->result, ns, "to",szTemp);

    sx_nad_write(sess->s, sess->result);
    sess->result = NULL;

    return 0;
}

unsigned int SendXmlTimeout(char* data)
{
    XT_HEAD* head = NULL;
    head = (XT_HEAD*)data;

    CRecvPacket notifyPacket;
    notifyPacket.SetData(data + sizeof(XT_HEAD), MAX_SEND_BUFFERS - sizeof(XT_HEAD));

    XT_DIRECTORY_ACK ack;

    // 查找sess
    sess_t sess = NULL;

    map<uint32,sess_t>::iterator it = sessManager.find(head->did);
    if(it != sessManager.end())
    {
        sess = it->second;
        mio_close(sess->c2s->mio, sess->fd);
        sessManager.erase(head->did);
        log_debug(ZONE, "SendXmlTimeout del user %u",head->did);
    }
    else
    {
        log_debug(ZONE, "SendXmlTimeout not find sess");
        return -1;
    }


    return 0;
}

// 发送历史消息给移动端
int SendMsgHistory(uint32 did, MsgList &ml)
{
    // 查找sess
    sess_t sess = NULL;

    map<uint32,sess_t>::iterator it = sessManager.find(did);
    if(it != sessManager.end())
    {
        sess = it->second;
        log_debug(ZONE, "SendMsgHistory find sess");
    }
    else
    {
        log_debug(ZONE, "SendMsgHistory not find sess");
        return -1;
    }

    // 构造返回xml
    sess->result = nad_new();

    int ns = /*nad_add_namespace(sess->result, uri_HISTORYMSG, NULL)*/-1;
    nad_append_elem(sess->result, ns, "iq", 0);
    nad_append_attr(sess->result, ns, "id","historymsg");
    nad_append_attr(sess->result, ns, "type","result");

	nad_append_elem(sess->result, ns, "query", 1);
    nad_append_attr(sess->result, -1, "xmlns", uri_HISTORYMSG);

    int nsize = ml.msglist_size();

    char szTemp[1024]= {0};
    nad_append_elem(sess->result, ns, "MsgList", 2);

    for(int i = 0; i < nsize; i++)
    {
        MsgInfo *pMsg = ml.mutable_msglist(i);

        nad_append_elem(sess->result, ns, "MsgInfo", 3);

        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", pMsg->msgid());
        nad_append_attr(sess->result, -1, "msgid", szTemp);

        nad_append_attr(sess->result, -1, "uuid", pMsg->uuid().c_str());

        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", pMsg->fromid());
        nad_append_attr(sess->result, -1, "fromid", szTemp);

        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", pMsg->toid());
        nad_append_attr(sess->result, -1, "toid", szTemp);

        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", pMsg->send_time());
        nad_append_attr(sess->result, -1, "send_time", szTemp);

		char szMsg[1300];
		gbk2utf8(szMsg, pMsg->data().c_str());
        nad_append_attr(sess->result, -1, "data", szMsg);

		char szNickName[128];
		gbk2utf8(szNickName, pMsg->fromnickname().c_str());
        nad_append_attr(sess->result, -1, "fromnickname", szNickName);

        memset(szTemp, 0, 1024);
        sprintf_s(szTemp, "%d", pMsg->datatype());
        nad_append_attr(sess->result, -1, "datatype", szTemp);
    }

    nad_append_elem(sess->result, ns, "isend", 2);
    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", ml.isend());
    nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

    nad_append_elem(sess->result, ns, "lastmsgid", 2);
    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", ml.lastmsgid());
    nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

    nad_append_elem(sess->result, ns, "userid", 2);
    memset(szTemp, 0, 1024);
    sprintf_s(szTemp, "%d", ml.userid());
    nad_append_cdata_gbk(sess->result,szTemp, strlen(szTemp), 3);

    sx_nad_write(sess->s, sess->result);
    sess->result = NULL;
    return 0;
}




