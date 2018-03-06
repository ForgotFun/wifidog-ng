/*
 * Copyright (C) 2017 Jianhui Zhao <jianhuizhao329@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include "config.h"
#include "utils.h"
#include <uci_blob.h>
#include <libubox/ulog.h>
#include <uhttpd/uhttpd.h>

static struct blob_buf b;

static struct config conf = {
    .gw_interface = "br-lan",
    .gw_port = 2060,
    .gw_ssl_port = 8443,
    .checkinterval = 30,
    .temppass_time = 30,
    .authserver = {
        .port = 80,
        .path = "/wifidog/",
        .login_path = "login",
        .portal_path = "portal",
        .msg_path = "gw_message.php",
        .ping_path = "ping",
        .auth_path = "auth",
    }
};

enum {
	GATEWAY_ATTR_IFNAME,
    GATEWAY_ATTR_ADDRESS,
    GATEWAY_ATTR_ID,
    GATEWAY_ATTR_SSID,
	GATEWAY_ATTR_PORT,
	GATEWAY_ATTR_SSL_PORT,
	GATEWAY_ATTR_CHECKINTERVAL,
    GATEWAY_ATTR_TEMPPASS_TIME,
	GATEWAY_ATTR_MAX
};

static const struct blobmsg_policy gateway_attrs[GATEWAY_ATTR_MAX] = {
	[GATEWAY_ATTR_IFNAME] = { .name = "ifname", .type = BLOBMSG_TYPE_STRING },
    [GATEWAY_ATTR_ADDRESS] = { .name = "address", .type = BLOBMSG_TYPE_STRING },
    [GATEWAY_ATTR_ID] = { .name = "id", .type = BLOBMSG_TYPE_STRING },
    [GATEWAY_ATTR_SSID] = { .name = "ssid", .type = BLOBMSG_TYPE_STRING },
	[GATEWAY_ATTR_PORT] = { .name = "port", .type = BLOBMSG_TYPE_INT32 },
	[GATEWAY_ATTR_SSL_PORT] = { .name = "ssl_port", .type = BLOBMSG_TYPE_INT32 },
	[GATEWAY_ATTR_CHECKINTERVAL] = { .name = "checkinterval", .type = BLOBMSG_TYPE_INT32 },
    [GATEWAY_ATTR_TEMPPASS_TIME] = { .name = "temppass_time", .type = BLOBMSG_TYPE_INT32 }
};

static const struct uci_blob_param_list gateway_attr_list = {
	.n_params = GATEWAY_ATTR_MAX,
	.params = gateway_attrs,
};

static void parse_gateway(struct uci_section *s)
{    
    struct blob_attr *tb[GATEWAY_ATTR_MAX];
    
    blob_buf_init(&b, 0);

    uci_to_blob(&b, s, &gateway_attr_list);
    blobmsg_parse(gateway_attrs, GATEWAY_ATTR_MAX, tb, blob_data(b.head), blob_len(b.head));

    if (tb[GATEWAY_ATTR_IFNAME])
        conf.gw_interface = strdup(blobmsg_get_string(tb[GATEWAY_ATTR_IFNAME]));

    if (tb[GATEWAY_ATTR_ADDRESS])
        conf.gw_address = strdup(blobmsg_get_string(tb[GATEWAY_ATTR_ADDRESS]));
    
    if (tb[GATEWAY_ATTR_ID])
        conf.gw_id = strdup(blobmsg_get_string(tb[GATEWAY_ATTR_ID]));

    if (tb[GATEWAY_ATTR_SSID]) {
        const char *ssid = blobmsg_get_string(tb[GATEWAY_ATTR_SSID]);
        conf.ssid = calloc(1, strlen(ssid) * 4);
        urlencode((char *)conf.ssid, strlen(ssid) * 4, ssid, strlen(ssid));
    }

    if (tb[GATEWAY_ATTR_PORT])
        conf.gw_port = blobmsg_get_u32(tb[GATEWAY_ATTR_PORT]);

    if (tb[GATEWAY_ATTR_SSL_PORT])
        conf.gw_ssl_port = blobmsg_get_u32(tb[GATEWAY_ATTR_SSL_PORT]);

    if (tb[GATEWAY_ATTR_CHECKINTERVAL])
        conf.checkinterval = blobmsg_get_u32(tb[GATEWAY_ATTR_CHECKINTERVAL]);

    if (tb[GATEWAY_ATTR_TEMPPASS_TIME])
        conf.temppass_time = blobmsg_get_u32(tb[GATEWAY_ATTR_TEMPPASS_TIME]);
}

enum {
    AUTHSERVER_ATTR_HOST,
	AUTHSERVER_ATTR_PORT,
	AUTHSERVER_ATTR_PATH,
	AUTHSERVER_ATTR_LOGIN_PATH,
	AUTHSERVER_ATTR_PORTAL_PATH,
	AUTHSERVER_ATTR_MSG_PATH,
	AUTHSERVER_ATTR_PING_PATH,
	AUTHSERVER_ATTR_AUTH_PATH,
	AUTHSERVER_ATTR_MAX
};

static const struct blobmsg_policy authserver_attrs[AUTHSERVER_ATTR_MAX] = {
	[AUTHSERVER_ATTR_HOST] = { .name = "host", .type = BLOBMSG_TYPE_STRING },
	[AUTHSERVER_ATTR_PORT] = { .name = "port", .type = BLOBMSG_TYPE_INT32 },
	[AUTHSERVER_ATTR_PATH] = { .name = "path", .type = BLOBMSG_TYPE_STRING },
	[AUTHSERVER_ATTR_LOGIN_PATH] = { .name = "login_path", .type = BLOBMSG_TYPE_STRING },
	[AUTHSERVER_ATTR_PORTAL_PATH] = { .name = "portal_path", .type = BLOBMSG_TYPE_STRING },
	[AUTHSERVER_ATTR_MSG_PATH] = { .name = "msg_path", .type = BLOBMSG_TYPE_STRING },
	[AUTHSERVER_ATTR_PING_PATH] = { .name = "ping_path", .type = BLOBMSG_TYPE_STRING },
	[AUTHSERVER_ATTR_AUTH_PATH] = { .name = "auth_path", .type = BLOBMSG_TYPE_STRING },
};

const struct uci_blob_param_list authserver_attr_list = {
	.n_params = AUTHSERVER_ATTR_MAX,
	.params = authserver_attrs,
};

static void parse_authserver(struct uci_section *s)
{
    struct blob_attr *tb[AUTHSERVER_ATTR_MAX];
    
    blob_buf_init(&b, 0);

    uci_to_blob(&b, s, &authserver_attr_list);
    blobmsg_parse(authserver_attrs, AUTHSERVER_ATTR_MAX, tb, blob_data(b.head), blob_len(b.head));

    if (tb[AUTHSERVER_ATTR_HOST])
        conf.authserver.host = strdup(blobmsg_get_string(tb[AUTHSERVER_ATTR_HOST]));
    
    if (tb[AUTHSERVER_ATTR_PORT])
        conf.authserver.port = blobmsg_get_u32(tb[AUTHSERVER_ATTR_PORT]);

    if (tb[AUTHSERVER_ATTR_PATH])
        conf.authserver.path = strdup(blobmsg_get_string(tb[AUTHSERVER_ATTR_PATH]));

    if (tb[AUTHSERVER_ATTR_LOGIN_PATH])
        conf.authserver.login_path = strdup(blobmsg_get_string(tb[AUTHSERVER_ATTR_LOGIN_PATH]));

    if (tb[AUTHSERVER_ATTR_PORTAL_PATH])
        conf.authserver.portal_path = strdup(blobmsg_get_string(tb[AUTHSERVER_ATTR_PORTAL_PATH]));

    if (tb[AUTHSERVER_ATTR_MSG_PATH])
        conf.authserver.msg_path = strdup(blobmsg_get_string(tb[AUTHSERVER_ATTR_MSG_PATH]));

    if (tb[AUTHSERVER_ATTR_PING_PATH])
        conf.authserver.ping_path = strdup(blobmsg_get_string(tb[AUTHSERVER_ATTR_PING_PATH]));

    if (tb[AUTHSERVER_ATTR_AUTH_PATH])
        conf.authserver.auth_path = strdup(blobmsg_get_string(tb[AUTHSERVER_ATTR_AUTH_PATH]));
}

int parse_config()
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *p = NULL;
    struct uci_element *e;
    char buf[128];
    
    if (uci_load(ctx, "wifidog", &p) || !p) {
        ULOG_ERR("Load config wifidog failed\n");
        uci_free_context(ctx);
        return -1;
    }

    uci_foreach_element(&p->sections, e) {
        struct uci_section *s = uci_to_section(e);
        if (!strcmp(s->type, "gateway"))
            parse_gateway(s);
        else if (!strcmp(s->type, "authserver"))
            parse_authserver(s);
    }

    blob_buf_free(&b);
    uci_free_context(ctx);
    
    if (!conf.gw_id) {
        if (get_iface_mac(conf.gw_interface, buf, sizeof(buf)) < 0) {
            uci_free_context(ctx);
            return -1;
        }
        conf.gw_id = strdup(buf);
    }

    if (!conf.gw_address) {
        if (get_iface_ip(conf.gw_interface, buf, sizeof(buf)) < 0) {
            uci_free_context(ctx);
            return -1;
        }
        conf.gw_address = strdup(buf);
    }
    
    if (conf.authserver.port == 80) {
        asprintf((char **)&conf.login_url, "http://%s%s%s?gw_address=%s&gw_port=%d&gw_id=%s&ssid=%s",
            conf.authserver.host, conf.authserver.path, conf.authserver.login_path,
            conf.gw_address, conf.gw_port, conf.gw_id, conf.ssid ? conf.ssid : "");

        asprintf((char **)&conf.auth_url, "http://%s%s%s?gw_id=%s",
            conf.authserver.host, conf.authserver.path, conf.authserver.auth_path, conf.gw_id);

        asprintf((char **)&conf.ping_url, "http://%s%s%s?gw_id=%s",
            conf.authserver.host, conf.authserver.path, conf.authserver.ping_path, conf.gw_id);

        asprintf((char **)&conf.portal_url, "http://%s%s%s?gw_id=%s",
            conf.authserver.host, conf.authserver.path, conf.authserver.portal_path, conf.gw_id);

        asprintf((char **)&conf.msg_url, "http://%s%s%s?gw_id=%s",
            conf.authserver.host, conf.authserver.path, conf.authserver.msg_path, conf.gw_id);
    } else {
        asprintf((char **)&conf.login_url, "http://%s:%d%s%s?gw_address=%s&gw_port=%d&gw_id=%s&ssid=%s",
            conf.authserver.host, conf.authserver.port, conf.authserver.path, conf.authserver.login_path,
            conf.gw_address, conf.gw_port, conf.gw_id, conf.ssid ? conf.ssid : "");

        asprintf((char **)&conf.auth_url, "http://%s:%d%s%s?gw_id=%s",
            conf.authserver.host, conf.authserver.port, conf.authserver.path, conf.authserver.auth_path, conf.gw_id);

        asprintf((char **)&conf.ping_url, "http://%s:%d%s%s?gw_id=%s",
            conf.authserver.host, conf.authserver.port, conf.authserver.path, conf.authserver.ping_path, conf.gw_id);

        asprintf((char **)&conf.portal_url, "http://%s:%d%s%s?gw_id=%s",
            conf.authserver.host, conf.authserver.port, conf.authserver.path, conf.authserver.portal_path, conf.gw_id);

        asprintf((char **)&conf.msg_url, "http://%s:%d%s%s?gw_id=%s",
            conf.authserver.host, conf.authserver.port, conf.authserver.path, conf.authserver.msg_path, conf.gw_id);
    }

    return 0;
}

struct config *get_config()
{
    return &conf;
}

