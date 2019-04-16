#include "http.h"
#include "framework.h"

ef_curl *ef_curl_init(char *host,int port,char *request_uri){
	ef_curl_request *req;
	ef_curl_response *resp;
	ef_curl *cu;

	req = (ef_curl_request *)malloc(sizeof(ef_curl_request));
	if(req == NULL){
		goto error;
	}
	req->status = R_STATUS_INIT;
	if(request_uri != NULL){
    	req->request_uri = (char *)malloc(strlen(request_uri)+1);
    	if(req->request_uri == NULL){
    		goto error;
    	}
    	req->request_uri = strcpy(req->request_uri,request_uri);
    }
    
	resp = (ef_curl_response *)malloc(sizeof(ef_curl_response));
	if(resp == NULL){
		free(req);
		return NULL;
	}
	resp->resp_status = R_STATUS_INIT;
	resp->content_length = 0;
	resp->content_received = 0;
	resp->bufsend = resp->buf;
	resp->bufend = resp->buf;
	resp->chunk_remain = 0;
	resp->headers = strtab_new(0);

	cu = (ef_curl *)malloc(sizeof(ef_curl));
	if(cu == NULL){
		free(req);
		free(resp);
		return NULL;
	}
	cu->req = req;
	cu->resp = resp;
    ef_curl_set_header(cu,"Host",host);

	if(port <= 0){
		port = DEFAULT_HTTP_PORT;
	}
	cu->fd = socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in addr_in = {0};
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(port);
    addr_in.sin_addr.s_addr = host ? inet_addr(host) : inet_addr(DEFAULT_HTTP_HOST);
    int res = ef_wrap_connect(cu->fd,(const struct sockaddr*)&addr_in,sizeof(addr_in));
    if (res < 0){
    	printf("init fail:%d\n",res);
    	goto error;
    }
    return cu;
error:
    free(req);
    free(resp);
    free(cu);
    return NULL;
}

void ef_curl_close(ef_curl *cu){
	return;
}

int ef_curl_set_header(ef_curl *cu,char *header,char *content){
	if(cu == NULL || cu->req == NULL){
		return -1;
	}
	
	if(cu->req->headers == NULL){
		cu->req->headers = strtab_new(0);
        if(!cu->req->headers){
        	return -1;
        }
	}
	if(!strtab_set(cu->req->headers,header,strlen(header),content,strlen(content))){
        return -1;
    }
    return 0;
}

int ef_curl_set_request_body(ef_curl *cu,char *key,char *val){
	if(cu == NULL || cu->req == NULL){
		return -1;
	}
	
	if(cu->req->request_body == NULL){
		cu->req->request_body = strtab_new(0);
        if(cu->req->request_body == NULL){
        	return -1;
        }
	}
	if(!strtab_set(cu->req->request_body,key,strlen(key),val,strlen(val))){
        return -1;
    }
    return 0;
}

// send request,parse response headers and buffer response body.
int ef_curl_exec(ef_curl *cu){
	if(cu == NULL || cu->req == NULL){
		return -1;
	}
    char buf[MAX_HEADER_BUFSIZE];
    int remain=MAX_HEADER_BUFSIZE,used,res;
    char *pos = buf;
    used = http_build_request_line(cu->req,pos,remain);
    if(used < 0){
    	printf("http_build_request_line error\n");
    	return -1;
    }
    pos += used;
    remain -= used;
    cu->req->status = R_STATUS_HEADER;
    while(cu->req->status == R_STATUS_HEADER){
    	used = http_build_request_header(cu->req,pos,remain);
    	if(used < 0 || cu->req->status == R_STATUS_BODY){
    		if(used > 0){
    			pos += used;
    	        remain -= used;
    		}
    		res = ef_wrap_write(cu->fd,buf,pos-buf);
            if(res < 0){
    	        return res;
            }
            pos = buf;
            remain = MAX_HEADER_BUFSIZE;
            continue;
    	}
    	pos += used;
    	remain -= used;
    }

    while(cu->req->status == R_STATUS_BODY){
    	used = http_build_request_body(cu->req,pos,remain);
    	if(used < 0 || cu->req->status == R_STATUS_DONE){
    		if(used > 0){
    			pos += used;
    		}
    		if(pos - buf > 0){
                res = ef_wrap_write(cu->fd,buf,pos-buf);
                if(res < 0){
    	            return res;
                }
    		}    		
            pos = buf;
            remain = MAX_HEADER_BUFSIZE;
            continue;
    	}
    	pos += used;
    	remain -= used;
    }

    res = http_parse_resp_headers(cu);
    printf("tttttttt%d\n",res);
    return res;
}

int http_build_request_body(ef_curl_request *req,char *buf,size_t cap){
	printf("prepare to build request body\n");
	if(! req->request_body){
		req->status = R_STATUS_DONE;
        return 0;
	}

	// TODO
	req->status = R_STATUS_DONE;
	return 0;
}

int http_build_request_header(ef_curl_request *req,char *buf,size_t cap){
	printf("prepare to build headers\n");
	if(! req->headers->used){
		printf("build headers end\n");
		strcat(buf,"\r\n");
		req->status = R_STATUS_BODY;
        return 2;
	}
	//hash_first(ht,k,v);
	char *pos=buf;
    ef_string_t *k,*v;
	int res;
	k = req->headers->arrData->key;
    v = strtab_find(req->headers,k->str,k->len);
	if(cap < k->len+v->len+3){
		return -1;
	}
	strcpy(pos,k->str);
	pos += k->len;
    *pos++ = ':';
	strcpy(pos,v->str);
	pos += v->len;
	strcat(pos,"\r\n");
	pos += 2;
    res = strtab_remove(req->headers,k->str,k->len);
    if(res < 0){
    	printf("hash_remove fail:%s\n",k);
    	return res;
    }
	
	printf("remain headers:%d\n",req->headers->used);
	return pos - buf;
}

// build request line
int http_build_request_line(ef_curl_request *req,char *buf,size_t cap){
	printf("prepare to build request line\n");
	if(!req){
		return -1;
	}
    if(!req->method){
    	req->method = CURL_METHOD_GET;
    }
    if(!req->protocol){
    	req->protocol = HTTP_PROTOCOL_0;
    }
    if(!req->request_uri){
    	req->request_uri = "/";
    }
    int mlen,rlen,plen;
    mlen = strlen(req->method);
    rlen = strlen(req->request_uri);
    plen = strlen(req->protocol);
    if(mlen+rlen+plen+4 > cap){
    	printf("request line buffer is too small\n");
    	return -1;
    }
    char *pos = buf;
    strcat(pos,req->method);
    pos += mlen;
    *pos++ = ' ';
    strcat(pos,req->request_uri);
    pos += rlen;
    *pos++ = ' ';
    strcat(pos,req->protocol);
    pos += plen;
    strcat(pos,"\r\n");
    pos += 2;
    printf("%s %s %s %d\n",req->method,req->request_uri,req->protocol,pos-buf);
    return pos - buf;
}

// read response body from ef buffer into application buffer
int ef_curl_read_response(ef_curl *cu,char *buf,size_t len){
	printf("prepare to read response\n");
    int r,chunked = 0,buflen,sendlen=0;
    char *headerName = "transfer-encoding";
    ef_string_t *v = strtab_find(cu->resp->headers,headerName,strlen(headerName));
    strtolower(v->str,v->len);
    if(v && strcmp(v->str,"chunked") == 0){
    	printf("chunked response\n");
        chunked = 1;
    }
    if(chunked){
    	if(cu->resp->resp_status == R_STATUS_DONE){
    		printf("alread read done\n");
    	    return 0;
        }
        return http_parse_resp_chunked(cu,buf,len);
    }
    buflen = cu->resp->bufend - cu->resp->bufsend;
    if(buflen > len){
    	strncpy(buf,cu->resp->buf,len);
    	cu->resp->bufsend += len;
    	return len;
    }
    if(buflen > 0){
    	strncpy(buf,cu->resp->buf,buflen);
        sendlen = buflen;
        cu->resp->bufsend = cu->resp->buf;
        cu->resp->bufend = cu->resp->buf;
    }    
    if(cu->resp->resp_status == R_STATUS_DONE){
    	return sendlen;
    }

    r = ef_wrap_read(cu->fd,buf+sendlen,len);
    if(r < 0){
    	return sendlen;
    }
    sendlen += r;
    cu->resp->content_received += r;
    if(cu->resp->content_received == cu->resp->content_length){
    	cu->resp->resp_status = R_STATUS_DONE;
    }
    return sendlen;
}

// read and parse chunked response body
int http_parse_resp_chunked(ef_curl *cu,char *buf,size_t len){
	printf("prepare to parse chunked response\n");
	int buflen,chunklen,sendlen=0,r;
	char *pos,**start=&cu->resp->bufsend,**end=&cu->resp->bufend;
	
again:
    buflen = *end - *start;
    if(cu->resp->chunk_remain > 0){
    	if(cu->resp->chunk_remain > buflen){
    		if(buflen > len){
    			strncpy(buf+sendlen,*start,len);
    			*start += len;
    			cu->resp->chunk_remain -= len;
    		    return len;
    		}
    		sendlen = buflen;
    		strncpy(buf+sendlen,*start,sendlen);
    		cu->resp->chunk_remain -= sendlen;
    		r = ef_wrap_read(cu->fd,*start,MAX_RESP_BUFSIZE);
    		if(r < 0){
    			return sendlen;
    		}
    		*start = cu->resp->buf;
    		*end = cu->resp->buf + r;
    		goto again;
    	}
        if(cu->resp->chunk_remain + sendlen > len){
        	strncpy(buf+sendlen,*start,len-sendlen);
        	cu->resp->chunk_remain -= len-sendlen;
        	sendlen = len;
        	*start += len-sendlen;
        	return sendlen;
        }
        strncpy(buf+sendlen,*start,cu->resp->chunk_remain);
        sendlen += cu->resp->chunk_remain;
        *start += cu->resp->chunk_remain;
        cu->resp->chunk_remain = 0;
        buflen = *end - *start;
        if(buflen < 2){
        	// chunk remain\r\n
        	r = ef_wrap_read(cu->fd,cu->resp->buf,MAX_RESP_BUFSIZE);
        	if(r < 0){
        		return sendlen;
        	}
        	*start = cu->resp->buf + 2 - buflen;
        	*end = cu->resp->buf + r;
        }else{
        	*start +=2;
        } 
    }

	pos = strstr(*start,"\r\n");
	while(!pos){
        r = ef_wrap_read(cu->fd,*end,MAX_RESP_BUFSIZE-buflen);
        printf("read from server:%d\n",r);
        if(r < 0){
        	return sendlen;
        }
        *end += r;
        buflen = *end - *start;
        pos = strstr(*start,"\r\n");
	}
	*start = pos+2;
	chunklen = hexstr2int(*start,pos-*start);
	if(chunklen == 0){
		cu->resp->resp_status = R_STATUS_DONE;
		return sendlen;
	}

    cu->resp->chunk_remain = chunklen;
    goto again;
}

//buffer response body for further reading
int http_buffer_resp_body(int fd,ef_curl_response *resp){
    // content-length
    int r;
    char *headerName = "content-length";
    ef_string_t *v = strtab_find(resp->headers,headerName,strlen(headerName));
    if(v){
        resp->content_length = str2int(v->str,v->len);
    }
    while(resp->content_received < MAX_RESP_BUFSIZE){
    	if(resp->content_length > 0 && resp->content_received >= resp->content_length){
    		resp->resp_status = R_STATUS_DONE;
    		break;
    	}
        r = ef_wrap_read(fd,resp->bufend,MAX_RESP_BUFSIZE - (resp->bufend - resp->bufsend));
        if(r < 0){
        	break;
        }
        resp->content_received += r;
        resp->bufend += r;
    }
    return resp->bufend - resp->bufsend;
}

// parse response headers
int http_parse_resp_headers(ef_curl *cu){
	printf("prepare to parse resp headers\n");
    char buf[MAX_HEADER_BUFSIZE];
    char *start,*end;
    int r,res;
    r = ef_wrap_read(cu->fd,buf,MAX_RESP_BUFSIZE);
    printf("read from server:%d\n",r);
    if(r < 0){
    	return r;
    }
    res = http_parse_resp_line(cu->resp,buf);
    if(res < 0){
    	printf("response line is too long\n");
    	return res;
    }
    start = buf + res;
    end = buf + r;
    printf("start-buf=%d,end-buf=%d,end-start:%d,\n",start-buf,end-buf,end-start);
    cu->resp->resp_status = R_STATUS_HEADER;
    while(cu->resp->resp_status == R_STATUS_HEADER){
        res = http_parse_resp_header(cu->resp,start,end);
        printf("parse header:%d\n",res);
        if(res < 0){
        	strncpy(buf,start,end-start);
            r = ef_wrap_read(cu->fd,buf+(end-start),MAX_RESP_BUFSIZE-(end-start));
            start = buf;
            end = buf + (end - start) + r;
            continue;
        }
        start += res;
    }

    // buffer received body
    if(end - start > 0){
    	strncpy(cu->resp->buf,start,end-start);
    	cu->resp->content_received = end-start;
        cu->resp->bufend = cu->resp->buf + (end-start);
    }
    printf("end-start=%d\n",end-start);
    return 0;
}

int http_parse_resp_header(ef_curl_response *resp,char *start,char *bufend){
	    printf("prepare to parse resp header\n");
        char *pos,*end,*header,*val;
        ef_string_t *res;
        if(strncmp(start,"\r\n",2) == 0){
        	printf("parse resp header end.\n");
            resp->resp_status = R_STATUS_BODY;
            return 2;
        }
        end = strstr(start,"\r\n");  
        printf("%d\n",end - start);    
        if(!end){
        	printf("buffer is too small for resp headers\n");
            return -1;
        }

        pos = strstr(start,":");
        if(!pos){
        	printf("bad request header\n");
        	return end - start;
        }
        header = start;
        while(*header == ' '){
        	printf("headr space\n");
        	header++;
        }
        *pos = '\0';
        val = pos+1;
        while(*val == ' '){
            printf("val space\n");
        	val++;
        }
        *end = '\0';
        strtolower(header,strlen(header));
        res = strtab_set(resp->headers,header,pos-header,val,end-val);
        printf("set resp header:%s:%s %d\n",header,val,end - val);
        if(!res){
        	printf("hash_set fail %s:%s\n",header,val);
        }
        return end - start + 2;
}

int http_parse_resp_line(ef_curl_response *resp,char *buf){
	printf("prepare to parse resp line\n");
	char *pos,*end,*start=buf;
	end = strstr(start,"\r\n");
	if(!end){
		return -1;
	}
	pos = strstr(start," ");
	if(!pos){
		return -1;
	}
	while(*pos == ' '){
		pos++;
	}
	start = pos;
	// parse response status
    resp->status = str2int(start,pos-start);
    printf("response status:%d\n",resp->status);
    return end-buf+2;
}

