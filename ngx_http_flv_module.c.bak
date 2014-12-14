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
	ngx_str_t index_file_content;
} ngx_http_flv_loc_conf_t;

static char *ngx_http_flv(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void *ngx_http_flv_create_loc_conf(ngx_conf_t *cf);

static ngx_command_t ngx_http_flv_commands[] = {
	{
		ngx_string("flv_file"),
		NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
		ngx_http_flv,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_flv_loc_conf_t, index_file_content),
		NULL
	},
	ngx_null_command
};

static ngx_http_module_t ngx_http_flv_module_ctx = {
	NULL,
	NULL,

	NULL,
	NULL,

	NULL,
	NULL,

	ngx_http_flv_create_loc_conf,

	NULL
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

static ngx_int_t ngx_http_flv_handler(ngx_http_request_t *r)
{
	ngx_int_t rc;
	ngx_chain_t out;

	ngx_http_flv_loc_conf_t *flcf;
	flcf = ngx_http_get_module_loc_conf(r, ngx_http_flv_module);

	ngx_buf_t *b;
    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
	if(b == NULL)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	b->pos = flcf->index_file_content.data;
	b->last = b->pos + flcf->index_file_content.len;
	b->memory = 1;
	b->last_buf = 1;

	ngx_str_t response = ngx_string("This is a test");
	ngx_str_t type = ngx_string("text/plain");
	r->headers_out.content_type = type;
	r->headers_out.content_length_n = response.len;
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

	b->pos = response.data;
	b->last = b->pos + response.len;
	b->memory = 1;
	b->last_buf = 1;

	out.buf = b;
	out.next = NULL;

	return ngx_http_output_filter(r, &out);
}

static void *ngx_http_flv_create_loc_conf(ngx_conf_t *cf)
{
	ngx_http_flv_loc_conf_t *conf;

	conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_flv_loc_conf_t));
	if(conf == NULL)
	{
		return NGX_CONF_ERROR;
	}
	conf->index_file_content.len = 0;
	conf->index_file_content.data = NULL;

	return conf;
}

static char *ngx_http_flv(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_core_loc_conf_t *clcf;

	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

	clcf->handler = ngx_http_flv_handler;

	ngx_conf_set_str_slot(cf, cmd, conf);

	return NGX_CONF_OK;
}
