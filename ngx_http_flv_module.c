/*
 * Author: Liguanpei, Huanglong
 * Date:   2014-12-12
 * Version:v0.1
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_md5.h>
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

void count_md5_value(ngx_str_t *path, u_char result[])
{
	ngx_md5_t md5;

	ngx_md5_init(&md5);
	ngx_md5_update(&md5, path->data, path->len);
	ngx_md5_final(result, &md5);
}

void count_crc32_value(u_char src[], uint32_t *crc32_value)
{
	ngx_crc32_init(*crc32_value);
	ngx_crc32_update(crc32_value, src, 16);
}

ngx_int_t get_file_num(ngx_str_t *path)
{
	ngx_int_t file_num = 0;
	ngx_dir_t dir;

	ngx_open_dir(path, &dir);
	while(ngx_read_dir(&dir) != NGX_ERROR)
	{
		if (dir.type != 8 ||
				ngx_strcmp(ngx_de_name(&dir) + ngx_de_namelen(&dir) - 5, ".file") != 0)  // 类型8为文件，且规定文件以".file"为后缀
		{
			continue;
		}
		file_num++;
	}

	return file_num;
}

u_char *ngx_http_uri_to_path(ngx_http_request_t *r, ngx_str_t *path, 
		size_t *root_length)
{
	u_char                  *last;
	u_char                   md5_value[16];
	u_char                   tmp[20]; 
	//ngx_str_t                filename;
	uint32_t                 crc32_value = 0;
	ngx_int_t                file_num; 
	ngx_http_flv_loc_conf_t *flcf;

	flcf = ngx_http_get_module_loc_conf(r, ngx_http_flv_module);

	*root_length = flcf->flv_path.len;
	path->len = flcf->flv_path.len + r->uri.len + 1;
	path->data = ngx_pnalloc(r->pool, path->len);
	if(path->data == NULL)
	{
		return NULL;
	}
	last = ngx_copy(path->data, flcf->flv_path.data, flcf->flv_path.len);
	last = ngx_cpystrn(last, r->uri.data, r->uri.len + 1);

	ngx_memzero(md5_value, sizeof(md5_value));

	count_md5_value(path, md5_value);
	count_crc32_value(md5_value, &crc32_value);
	file_num = get_file_num(&flcf->flv_path);
	
	
	ngx_memzero(tmp, sizeof(tmp));
	ngx_sprintf(tmp, "/%d.file", crc32_value % file_num);
	path->len = flcf->flv_path.len + ngx_strlen(tmp) + 1;
	path->data = ngx_pnalloc(r->pool, path->len);
	if(path->data == NULL)
	{
		return NULL;
	}
	last = ngx_copy(path->data, flcf->flv_path.data, flcf->flv_path.len);
	last = ngx_cpystrn(last, tmp, ngx_strlen(tmp) + 1);
	
	return last;
}

static ngx_int_t ngx_http_flv_handler(ngx_http_request_t *r)
{
	u_char                  *last, *location;
	size_t                   root, len;
	ngx_int_t                rc;
	ngx_buf_t               *b;
	ngx_str_t                type = ngx_string("text/plain");
	ngx_chain_t              out[6];
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

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
	if(b == NULL)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	b->pos = flcf->flv_path.data;
	b->last = b->pos + flcf->flv_path.len;
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

	ngx_str_t path;
    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
	if(b == NULL)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	last = ngx_http_uri_to_path(r, &path, &root);
	path.len = last - path.data;

	if(last == NULL)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	b->pos = path.data;
	b->last = path.data + path.len;
	b->memory = 1;
	b->last_buf = 0;

	out[3].buf = b;
	out[3].next = &out[4];

	ngx_str_t message_2 = ngx_string("\nFile content: ");
    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
	if(b == NULL)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	b->pos = message_2.data;
	b->last = b->pos + message_2.len;
	b->memory = 1;
	b->last_buf = 1;

	out[4].buf = b;
	out[4].next = &out[5];

	ngx_open_file_info_t of;
	ngx_http_core_loc_conf_t *clcf;
	clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
	ngx_memzero(&of, sizeof(ngx_open_file_info_t));

	of.read_ahead = clcf->read_ahead;
	of.directio = clcf->directio;
	of.valid = clcf->open_file_cache_valid;
	of.min_uses = clcf->open_file_cache_min_uses;
	of.errors = clcf->open_file_cache_errors;
	of.events = clcf->open_file_cache_events;

	if(ngx_http_set_disable_symlinks(r, clcf, &path, &of) != NGX_OK)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	if(ngx_open_cached_file(clcf->open_file_cache, &path, &of, r->pool) != NGX_OK)
	{
		switch(of.err){
			
		case 0:
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		
		case NGX_ENOENT:
		case NGX_ENOTDIR:
		case NGX_ENAMETOOLONG:

			rc = NGX_HTTP_NOT_FOUND;
			break;

		case NGX_EACCES:
#if (NGX_HAVE_OPENAT)
		case NGX_EMLINK:
		case NGX_ELOOP:
#endif 
			rc = NGX_HTTP_FORBIDDEN;
			break;

		default:
			rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
			break;
		}
		return rc;
	}
	r->root_tested = !r->error_page;

	if(of.is_dir)
	{
		ngx_http_clear_location(r);

		r->headers_out.location = ngx_palloc(r->pool, sizeof(ngx_table_elt_t));
		if(r->headers_out.location == NULL)
		{
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
		len = r->uri.len + 1;
		if(!clcf->alias && clcf->root_lengths == NULL && r->args.len == 0)
		{
			location = path.data + root;

			*last = '/';
		}
		else
		{
			if(r->args.len)
			{
				len += r->args.len + 1;
			}
			location = ngx_pnalloc(r->pool, len);
			if(location == NULL)
			{
				return NGX_HTTP_INTERNAL_SERVER_ERROR;
			}
			last = ngx_copy(location, r->uri.data, r->uri.len);
			*last = '/';
			if(r->args.len)
			{
				*++last = '?';
				ngx_memcpy(++last, r->args.data, r->args.len);
			}
		}
		r->headers_out.location->value.len = len;
		r->headers_out.location->value.data = location;

		return NGX_HTTP_MOVED_PERMANENTLY;
	}

	if(!of.is_file)
	{
		return NGX_HTTP_NOT_FOUND;
	}

	if(r->method & NGX_HTTP_POST)
	{
		return NGX_HTTP_NOT_ALLOWED;
	}

	rc = ngx_http_discard_request_body(r);
	if(rc != NGX_OK)
	{
		return rc;
	}

	r->headers_out.status = NGX_HTTP_OK;
	r->headers_out.last_modified_time = of.mtime;

	if(ngx_http_set_etag(r) != NGX_OK)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	if(ngx_http_set_content_type(r) != NGX_OK)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	if(r != r->main && of.size == 0)
	{
		return ngx_http_send_header(r);
	}

	r->allow_ranges = 1;

	b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
	if(b == NULL)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	b->file = ngx_pcalloc(r->pool, sizeof(ngx_file_t));
	if(b->file == NULL)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}


	r->headers_out.content_type = type;
	//r->headers_out.content_length_n = flcf->flv_path.len;
	r->headers_out.status = NGX_HTTP_OK;

	rc = ngx_http_send_header(r);
	if(rc == NGX_ERROR || rc > NGX_OK || r->header_only)
	{
		return rc;
	}

	b->file_pos = 0;
	b->file_last = of.size;

	b->in_file = b->file_last ? 1: 0;
	b->last_buf = (r == r->main) ? 1: 0;
	b->last_in_chain = 1;

	b->file->fd = of.fd;
	b->file->name = path;
	//b->file->log = log;
	b->file->directio = of.is_directio;

	out[5].buf = b;
	out[5].next = NULL;

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
