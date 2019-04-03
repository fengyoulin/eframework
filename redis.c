#include "framework.h"
#include "redis.h"

ef_redis_connection * ef_redis_connect(char *host,int port){
	ef_redis_connection *con = (ef_redis_connection*)malloc(sizeof(ef_redis_connection));
	con->host = host;
	con->port = port;
    con->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    con->buf = (char*)malloc(RES_BUFSIZE);
    con->seek = 0;
    if(con->buf == NULL){
    	printf("res buffer error\n");
    	return NULL;
    }
    if(con->host == NULL){
    	con->host = DEFAULT_HOST;
    }
    if(con->port == 0){
    	con->port = DEFAULT_PORT;
    }
    struct sockaddr_in addr_in = {0};
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(con->port);
    addr_in.sin_addr.s_addr = inet_addr(con->host);
    int res = ef_wrap_connect(con->sockfd,(const struct sockaddr*)&addr_in,sizeof(addr_in));
    if (res < 0){
    	printf("connect fail:%d\n",res);
    	return NULL;
    }
    return con;
}

int ef_redis_close(ef_redis_connection *con){
    close(con->sockfd);
    free(con->buf);
    return 0;
}
int ef_redis_free_reply(ef_redis_reply *rep){
    if(rep == NULL){
        return 0;
    }
    if(rep->type & REPLY_TYPE_NULL){
    	free(rep);
    	return 0;
    }
    if(rep->type & REPLY_TYPE_LONG){
    	return free_reply_long(rep);
    }
    if(rep->type & REPLY_TYPE_STR){
    	return free_reply_str(rep);
    }
    if(rep->type & REPLY_TYPE_ERR){
    	return free_reply_err(rep);
    }
    if(rep->type & REPLY_TYPE_ARR){
    	return free_reply_arr(rep);
    }
    return -1;
}
int free_reply_long(ef_redis_reply *rep){
	free(rep);
	return 0;
}
int free_reply_str(ef_redis_reply *rep){
    if(rep->reply.str != NULL){
    	if(rep->reply.str->buf != NULL){
    		free(rep->reply.str->buf);
    	}
    	free(rep->reply.str);
    }
    free(rep);
    return 0;
}
int free_reply_err(ef_redis_reply *rep){
    if(rep->reply.err != NULL){
    	if(rep->reply.err->type != NULL){
    		free(rep->reply.err->type);
    	}
    	if(rep->reply.err->err != NULL){
    		free(rep->reply.err->err);
    	}
    	free(rep->reply.err);
    }
    free(rep);
    return 0;
}
int free_reply_arr(ef_redis_reply *rep){
    if(rep->reply.arr != NULL){
    	int i;
    	for (i=0;i<rep->reply.arr->num;++i){
            ef_redis_free_reply(*(rep->reply.arr->elem+i));
    	}
    	free(rep->reply.arr->elem);
    	free(rep->reply.arr);
    }
    free(rep);
    return 0;
}

int ef_redis_cmd(ef_redis_connection * con,const char *fmt,...){
	char cmd[1024];
	va_list argp;
	va_start(argp,fmt);
    int cmdlen = vsprintf(cmd,fmt,argp);
    va_end(argp);
    if(cmdlen < 0){
    	printf("vsprintf error:%d\n",cmdlen);
    	return cmdlen;
    }
    strcat(cmd,"\r\n");
    cmdlen += 2;
    int res = ef_wrap_write(con->sockfd,cmd,cmdlen);
    if(res < 0){
    	printf("send cmd fail:%d\n",res);
        return res;
    }
    return res;
}

ef_redis_reply * ef_redis_get(ef_redis_connection *con,const char *key){
    int res = ef_redis_cmd(con,"GET %s",key);
    if(res < 0){
    	return NULL;
    }
    return ef_redis_read_reply(con);
}

ef_redis_reply* ef_redis_read_reply(ef_redis_connection *con){
    int r = ef_wrap_read(con->sockfd,con->buf,RES_BUFSIZE);
    con->seek = 0;
    if (r < 0){
    	printf("read response fail:%d\n",r);
    	return NULL;
    }
    char *start = con->buf + con->seek;
    switch(*(con->buf)){
    	case '-':{
    		return parse_error(con->buf);
    	}
    	case '+':{
    		return parse_single_string(start,&con->seek);
    	}
    	case ':':{
    		return parse_long(start,&con->seek);
    	}
    	case '*':{
    		return parse_array(start,&con->seek);
    	}
    	case '$':{
            return parse_bulk_string(start,&con->seek);
    	}
    	default:
    	    printf("ef_redis_read_reply unsupported type:%c\n",*(con->buf));
    	    return NULL;
    }   
}

// parse reply with prefix ':'
ef_redis_reply* parse_long(char *buf,size_t *seek){
	ef_redis_reply *rep = (ef_redis_reply *)malloc(sizeof(ef_redis_reply));
    if(rep == NULL){
    	return rep;
    }
    rep->type = REPLY_TYPE_LONG;
	long n = 0;
	char * start = buf+1;
	while(*start != '\r'){
		n = n*10 + (*start)-'0';
		start++;
	}
	rep->reply.d = n;
	*seek = *seek + (start-buf)+2;
    return rep;
}

// parse reply with prefix '+'
ef_redis_reply* parse_single_string(char *buf,size_t *seek){
    buf += 1;
    char *pos = strstr(buf,"\r\n");
    *seek += 1;
    return parse_string(buf,pos-buf,seek);
}

// parse reply with prefix '$'
ef_redis_reply* parse_bulk_string(char *buf,size_t *seek){
	char *start = buf;
    int strlen = parse_len(buf);
    char *pos = strstr(buf,"\r\n");
    buf = pos + 2;
    *seek = *seek + (buf-start);
    return parse_string(buf,strlen,seek);
}

// parse string content with sufix "\r\n"
ef_redis_reply* parse_string(char *buf,size_t strlen,size_t *seek){
    ef_redis_reply *rep = (ef_redis_reply *)malloc(sizeof(ef_redis_reply));
    if(rep == NULL){
    	return rep;
    }

    if(strlen == -1){
    	rep->type = REPLY_TYPE_NULL;
    	return rep;
    }

    rep->type = REPLY_TYPE_STR;
    if(strlen >= 0){
    	reply_string *str = (reply_string*)malloc(sizeof(reply_string));
        if(str == NULL){
            return NULL;
        }
    	char *content = (char*)malloc(strlen);
        if(content == NULL){
    	    free(rep);
    	    return NULL;
        }   
        content = strncpy(content,buf,strlen);
        str->buf = content;
        str->len = strlen;
        rep->reply.str = str;
        *seek = *seek + strlen + 2;
    }
    
    return rep;
}

// parse reply with prefix '*'
ef_redis_reply * parse_array(char *buf,size_t *seek){
	char *start = buf;
    ef_redis_reply *rep = (ef_redis_reply *)malloc(sizeof(ef_redis_reply));
    if(rep == NULL){
    	return rep;
    }
    rep->type = REPLY_TYPE_ARR;
    int repnum = parse_len(buf);
    rep->reply.arr = (reply_array*)malloc(sizeof(reply_array));
    if(rep->reply.arr == NULL){
    	printf("ef_redis_reply malloc error\n");
    	return NULL;
    }
    rep->reply.arr->num = repnum;
    if(repnum == -1){
    	rep->type = REPLY_TYPE_NULL;
    	*seek = *seek + 5;
    	return rep;
    }
    rep->reply.arr->elem = (ef_redis_reply**)malloc(repnum*sizeof(ef_redis_reply*));
    if(rep->reply.arr->elem == NULL){
    	printf("arr elem malloc error\n");
    	return NULL;
    }

    int i,replen;
    char *pos;
    pos = strstr(buf,"\r\n");
    if(pos-buf < 0){
        printf("connection buffer is too small\n");
    	return NULL;
    }
    buf = pos + 2;
    *seek = *seek + (buf-start);
    for(i=0;i<repnum;i++){
        switch(*buf){
        	case '$':{
        		*(rep->reply.arr->elem + i) = parse_bulk_string(buf,seek);
        		continue;
        	}
        	case ':':{
                *(rep->reply.arr->elem + i) = parse_long(buf,seek);
        		continue;
        	}
        	case '*':{
        		*(rep->reply.arr->elem + i) = parse_array(buf,seek);
        		continue;
        	}
            default:{
                printf("unsupport type:%c\n",*buf);
                break;
            }
        }
    }

    return rep;
}

// parse reply with prefix '-'
ef_redis_reply* parse_error(char *buf){   
    ef_redis_reply *rep = (ef_redis_reply *)malloc(sizeof(ef_redis_reply));
    if(rep == NULL){
    	return rep;
    }
    rep->type = REPLY_TYPE_ERR;
    reply_error *err = (reply_error*)malloc(sizeof(reply_error));
    if(err == NULL){
        return NULL;
    }
    char *type,*errinfo,*start,*pos;
    start = buf+1;
    pos = strstr(buf," ");
    if (pos - start > 0){
        type = (char *)malloc(pos - start);
        if(type != NULL){
        	type = strncpy(type,start,pos-start);
        	if(type == NULL){
        		printf("strcpy error\n");
        	}
        	*(type+(pos-start)) = '\0';
        }
    }
    start = pos+1;
    while(*(start) != ' '){
        start++;
    }
    pos = strstr(start,"\r\n");
    if(pos-start <  0){
    	printf("redis buffer is too small.\n");
    }
    errinfo = (char *)malloc(pos-start);
    errinfo = strncpy(errinfo,start,pos-start);
    *(errinfo + (pos-start)) = '\0';
    err->type = type;
    err->err = errinfo;
    rep->reply.err = err;
    return rep;
}

// parse length in response with format:
// *3\r\n number of multiple responses
// $12\r\n length of a single response
int parse_len(char *res){
	if((*res != '$') && (*res != '*') || *(res+1) == '-'){
		return -1;
	}
	res++;
	int len = 0;
	while(*res != '\r'){
        len = len * 10 + (*res - '0');
        res++;
	}
	return len;
}