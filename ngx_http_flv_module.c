/*
 * Author: Liguanpei, Huanglong
 * Date:   2014-12-12
 * Version:v0.1
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <stdio.h>

typedef struct {
	ngx_str_t flv_path;
} ngx_http_flv_loc_conf_t;

static char *ngx_http_flv(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char*ngx_http_flv_merge_loc_conf(ngx_conf_t *, void *, void *);
static ngx_int_t ngx_http_flv_init(ngx_conf_t *cf);
static void *ngx_http_flv_create_loc_conf(ngx_conf_t *cf);

static ngx_command_t ngx_http_flv_commands[] = {
	{
		ngx_string("flv_path"),
		NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
		ngx_http_flv,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_flv_loc_conf_t, flv_path),
		NULL
	},
	ngx_null_command
};

static ngx_http_module_t ngx_http_flv_module_ctx = {
	NULL,
	//NULL,
	ngx_http_flv_init,

	NULL,
	NULL,

	NULL,
	NULL,

	ngx_http_flv_create_loc_conf,

	ngx_http_flv_merge_loc_conf
};

ngx_module_t ngx_http_flv_module = {
	NGX_MODULE_V1,
	&ngx_http_flv_module_ctx,
	ngx_http_flv_commands,
	NGX_HTTP_MODULE,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NGX_MODULE_V1_PADDING
};

u_char *ngx_http_uri_to_path(ngx_http_request_t *r, ngx_str_t *path, 
		size_t *root_length)
{
	u_char                  *last;
	ngx_http_flv_loc_conf_t *flcf;

	flcf = ngx_http_get_module_loc_conf(r, ngx_http_flv_module);

	*root_length = flcf->flv_path.len;
	path->len = flcf->flv_path.len + r->uri.len;
	path->data = ngx_pnalloc(r->pool, path->len);
	if(path->data == NULL)
	{
		return NULL;
	}
	last = ngx_copy(path->data, flcf->flv_path.data, flcf->flv_path.len);
	last = ngx_cpystrn(last, r->uri.data, r->uri.len + 1);
	
	return last;
}

static ngx_int_t ngx_http_flv_handler(ngx_http_request_t *r)
{
	ngx_int_t                rc;
	ngx_buf_t               *b;
	ngx_chain_t              out[4];
	ngx_http_flv_loc_conf_t *flcf;

	flcf = ngx_http_get_module_loc_conf(r, ngx_http_flv_module);

	ngx_str_t message = ngx_string("The flv_path you config is: ");
    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
	if(b == NULL)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	b->pos = message.data;
	b->last = b->pos + message.len;
	b->memory = 1;

	out[0].buf = b;
	out[0].next = &out[1];

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
	if(b == NULL)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	b->pos = flcf->flv_path.data;
	b->last = b->pos + flcf->flv_path.len;
	b->memory = 1;
	b->last_buf = 0;

	ngx_str_t type = ngx_string("text/plain");
	r->headers_out.content_type = type;
	r->headers_out.status = NGX_HTTP_OK;

	rc = ngx_http_send_header(r);
	if(rc == NGX_ERROR || rc > NGX_OK || r->header_only)
	{
		return rc;
	}

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
	if(b == NULL)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	b->pos = flcf->flv_path.data;
	b->last = b->pos + flcf->flv_path.len;
	r->headers_out.content_length_n = flcf->flv_path.len;
	b->memory = 1;
	b->last_buf = 0;

	out[1].buf = b;
	out[1].next = &out[2];

	ngx_str_t message_1 = ngx_string("\nThe file you request is: ");
    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
	if(b == NULL)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	b->pos = message_1.data;
	b->last = b->pos + message_1.len;
	b->memory = 1;
	b->last_buf = 0;

	out[2].buf = b;
	out[2].next = &out[3];

	//u_char *last;
	ngx_str_t path;
    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
	if(b == NULL)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	size_t root;
	ngx_http_uri_to_path(r, &path, &root);

	b->pos = path.data;
	b->last = path.data + path.len;
	b->memory = 1;
	b->last_buf = 1;

	out[3].buf = b;
	out[3].next = NULL;

	return ngx_http_output_filter(r, &out[0]);
}

static void *ngx_http_flv_create_loc_conf(ngx_conf_t *cf)
{
	ngx_http_flv_loc_conf_t *conf;

	conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_flv_loc_conf_t));
	if(conf == NULL)
	{
		return NGX_CONF_ERROR;
	}
	conf->flv_path.len = 0;
	conf->flv_path.data = NULL;

	return conf;
}

static char*ngx_http_flv_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
	ngx_http_flv_loc_conf_t *prev = parent;
	ngx_http_flv_loc_conf_t *conf = child;

	ngx_conf_merge_str_value(conf->flv_path, prev->flv_path, "/var/www/html");

	return NGX_CONF_OK;
}

static char *ngx_http_flv(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_conf_set_str_slot(cf, cmd, conf);

	return NGX_CONF_OK;
}

static ngx_int_t ngx_http_flv_init(ngx_conf_t *cf)
{
	ngx_http_handler_pt       *h;
	ngx_http_core_main_conf_t *cmcf;

	cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

	h = ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
	if(h == NULL)
	{
		return NGX_ERROR;
	}

	*h = ngx_http_flv_handler;

	return NGX_OK; 
}
