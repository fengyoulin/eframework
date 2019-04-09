#include "hashtable.h"

hashtable *new_hash_table(size_t cap){
	cap = hash_recap(cap);
	hashtable *ht = (hashtable *)malloc(sizeof(hashtable));
	if(ht == NULL){
		return NULL;
	}
	size_t *data = (size_t *)malloc(sizeof(size_t) * cap + sizeof(block) * cap);
	if(data == NULL){
		free(ht);
		return NULL;
	}
	init_hash_data(data,cap);
    ht->cap = cap;
    ht->used = 0;
    ht->next = 0;
    ht->sizemask = HASH_SIZEMASK(ht);
    ht->arrData = (block *)(data + cap);
    return ht;
}

void init_hash_data(size_t *data,size_t cap){
	int i;
	for(i = 0;i<cap;++i){
		*(data+i) = -1;
	}
	block *arrData = (block *)(data+cap);
    for(i=0;i<cap;++i){
    	(arrData + i)->h = -1;
    	(arrData + i)->key = NULL;
    	(arrData + i)->val = NULL;
    	(arrData + i)->next = -1;
    }
    return;
}
size_t hash(char *key){
	size_t h = strlen(key);
}

int hash_set(hashtable *ht,char *key,char *val){
	if(ht == NULL){
		return -1;
	}
	char *v = (char *)malloc(sizeof(char) * (strlen(val)+1));
	if(v == NULL){
		return -1;
	}
	v = strcpy(v,val);
	if(v == NULL){
		return -1;
	}

	// key already exists
	block_val *bval = hash_get(ht,key);

	if(bval != NULL){
		free(bval->v.str);
		bval->v.str = v;
		return 0;
	}
	char *k = (char *)malloc(sizeof(char) * (strlen(key)+1));
	if(k == NULL){
		return -1;
	}
	k = strcpy(k,key);
	if(k == NULL){
		free(k);
		free(v);
		return -1;
	}
	
    // key not exists
    if(ht->next == ht->cap - 1){
    	//printf("prepare to resize:%d\n",ht->next);
		int res = hash_resize(ht);
		if(res < 0){
			free(k);
			free(v);
			return res;
		}
	}

	bval = (block_val*)malloc(sizeof(block_val));
    if(bval == NULL){
    	return -1;
    }
    bval->type = HASH_VAL_TYPE_STR;
    bval->v.str = v;

	size_t h = hash(key);
	size_t offset = HASH_OFFSET(ht,h);
	size_t *pidx = (size_t *)ht->arrData + offset;
	block *pb;
	if(*pidx == -1){	
		*pidx = ht->next;
		ht->next++;
		ht->used++;
		pb = ht->arrData + *pidx;
		pb->h = h;
		pb->key = k;
		pb->val = bval;
		pb->next = -1;
		//printf("hash_set(%s)idx:%d,h:%d,next:%d\n",pb->key,*pidx,pb->h,pb->next);
		return 0;
	}
    pb = ht->arrData + ht->next;
    pb->next = *pidx;
    *pidx = ht->next;
    ht->used++;
    ht->next++;
    pb->h = h;
    pb->key = k;
    pb->val = bval;
	//printf("hash_set(%s)idx:%d,h:%d,next:%d\n",pb->key,*pidx,pb->h,pb->next);
    return 0;
}

block_val *hash_get(hashtable *ht,char *key){
	if(ht == NULL){
		return NULL;
	}
	uint h = hash(key);
    size_t offset = HASH_OFFSET(ht,h);
    size_t index = *((size_t *)(ht->arrData) + offset);
    if(index == -1){
    	return NULL;
    }
    block *pb = ht->arrData+index;
    
    while(pb->h != -1){
    	if(strcmp(pb->key,key) == 0){
    	    return pb->val;
        }
        if(pb->next == -1){
        	break;
        }else{
        	pb = ht->arrData + pb->next;
        }
        
    }
    return NULL;
}

unsigned short hash_exists(hashtable *ht,char *key){
	if(ht == NULL){
		return 0;
	}
	uint h = hash(key);
	size_t offset = HASH_OFFSET(ht,h);
	size_t index = *((size_t *)(ht->arrData) + offset);
    if(index == -1){
    	return 0;
    }
    block *pb = ht->arrData+index;
    while(pb){
    	if(strcmp(pb->key,key) == 0){
    		return 1;
    	}
    	pb = ht->arrData + pb->next;
    }
    return 0;
}

int hash_remove(hashtable *ht,char *key){
	if(ht == NULL || !hash_exists(ht,key)){
		return 0;
	}
	uint h = hash(key);
	size_t offset = HASH_OFFSET(ht,h);
	size_t *pidx = (size_t*)ht->arrData + offset;
	block *pb = ht->arrData + *pidx;
	block *preb = NULL;
	while(pb){
		if(strcmp(pb->key,key) == 0){
			break;
		}
		preb = pb;
		if(pb->next != -1){
			pb = ht->arrData + pb->next;
		}else{
			return 0;
		}
	}
	if(preb != NULL){
		preb->next = pb->next;
	}else{
		*pidx = pb->next;
	}
	hash_free_block(pb,'1');
	ht->used--;
	return 0;
}

void hash_free_block(block *pb,char freeval){
	if(pb == NULL){
		return;
	}
	pb->h = -1;
	pb->key = NULL;
	if(freeval == '1'){
		free(pb->key);
		hash_free_block_val(pb->val);
	}
	pb->val = NULL;
	pb->next = -1;
	return;
}

void hash_free_block_val(block_val *pval){
	if(pval == NULL){
		return;
	}
	switch(pval->type){
		case HASH_VAL_TYPE_STR:
		    free(pval->v.str);
		    break;
	    case HASH_VAL_TYPE_LONG:
	        break;
	    case HASH_VAL_TYPE_ARR:
	        hash_free(pval->v.arr);
            break;
        default:
            printf("unknown block value type.\n");
            break;
	}
	free(pval);
	return;
}

void hash_free(hashtable *ht){
	int idx;
	for(idx=0;idx < ht->next;++idx){
		hash_free_block(ht->arrData+idx,'1');
	}
	free(HASH_DATA_START(ht));
	free(ht);
}

// 计算大于等于cap的最小2^n
size_t hash_recap(size_t cap){
	if(cap <= HASH_TABLE_INIT_CAP){
		return HASH_TABLE_INIT_CAP;
	}
    
    int count=0,last=0;
    while(cap > 0){
    	last = cap;
    	cap = cap & (cap-1);
    	count++;
    }
    
	return count == 1 ? last : last<<1;
}

int hash_resize(hashtable *ht){
    if(ht == NULL){
    	*ht = *new_hash_table(HASH_TABLE_INIT_CAP);
    	return 0;
    }
    size_t cap = ht->cap << 1;
    //printf("new cap:%d\n",cap);
    size_t *data = (size_t *)malloc(sizeof(size_t)*cap + sizeof(block) * cap);
    if(data == NULL){
    	return -1;
    }
    size_t *origdata = HASH_DATA_START(ht);
    init_hash_data(data,cap);
    hash_copy_block((block *)(data+cap),ht->arrData,ht->next);
    ht->cap = cap;
    ht->sizemask = HASH_SIZEMASK(ht);
    ht->arrData = (block *)(data+cap);
    hash_rehash(ht);
    free(origdata);
    return 0;
}

void hash_copy_block(block* dest,block* src,size_t count){
	if(count <= 0){
		return;
	}
	int idx;
	for (idx=0; idx<count;++idx){
		if((src+idx)->h == -1){
			continue;
		}
        *(dest+idx) = *(src+idx);
	}
	return;
}

void hash_rehash(hashtable *ht){
    int idx,idxused;
    block *pb,*pbused;
    for(idx = 0; idx < ht->next; ++idx){
    	pb = ht->arrData+idx;
    	if(pb->h == -1){
    		// find next used block to fill the hole
    		pbused = NULL;
    		idxused = idx+1;
            while(idxused < ht->next){
            	if((ht->arrData+idxused)->h != -1){
            		pbused = ht->arrData+idxused;
            		break;
            	}
            	idxused++; 
            }
            if(pbused == NULL){
            	ht->next = idx;
            	break;
            }
            *pb = *pbused;
            hash_free_block(pbused,'0');
    	}
    	size_t offset = HASH_OFFSET(ht,pb->h);
    	size_t * pidx = (size_t*)ht->arrData + offset;
    	if(*pidx == -1){
    		*pidx = idx;
    		continue;
    	} 
    	pb->next = *pidx;
    	*pidx = idx;
    }
}
